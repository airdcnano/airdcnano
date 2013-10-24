/* vim:set ts=4 sw=4 sts=4 et cindent: */
/*
 * nanodc - The ncurses DC++ client
 * Copyright © 2005-2006 Markus Lindqvist <nanodc.developer@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contributor(s):
 *  
 */

#include <thread>
#include <functional>
#include <pthread.h>
#include <client/stdinc.h>
#include <client/DCPlusPlus.h>
#include <client/Util.h>
#include <client/StringTokenizer.h>

#include <ui/window_hub.h>
#include <input/manager.h>
#include <display/status_item.h>
#include <display/manager.h>
#include <core/manager.h>
#include <core/settings.h>
#include <utils/utils.h>
#include <ui/manager.h>
#include <core/events.h>
#include <core/log.h>

using namespace dcpp;

namespace core {

Manager::Manager()
{
}

struct Prompter {
public:
	enum Type {
		TYPE_ANY,
		TYPE_BOOL,
		TYPE_INT,
		TYPE_STRING,
		TYPE_QUESTION,
		TYPE_DOUBLE
	};

	void show(const string& aMsg, Type aType, optional<pair<double, double>> aNumRange = nullptr) {
		numRange = aNumRange;
		type = aType;

		string msg = aMsg;
		if (aType == TYPE_QUESTION) {
			msg += " [Y/N]\r\n";
		} else if (aType == TYPE_ANY) {
			msg += " [Enter]\r\n";
		} else if (numRange) {
			auto min = (*numRange).first;
			auto max = (*numRange).second;
			if (aType == TYPE_INT) {
				msg += " (" + Util::toString(static_cast<int>(min)) + "-" + Util::toString(static_cast<int>(max)) + ")";
			} else
				msg += " (" + Util::toString(min) + "-" + Util::toString(max) + ")";
		}

		while (true) {
			bool ready = false;
			auto conn = events::add_listener_first("key pressed", [&] {
				// detect enter
				wint_t key = events::arg<wint_t>(1);
				if (key == 0xA) {
					input = display::Manager::get()->m_inputWindow.getInputStr();
					ready = true;
				}
			});

			StringTokenizer<string> lines(msg, "\r\n");
			for (const auto& l : lines.getTokens()) {
				if (!l.empty())
					core::Log::get()->log(l);
			}

			while (!ready) {
				ui::Manager::get()->redraw_screen();
				sleep(1);
			}

			conn.disconnect();
			if (aType == TYPE_ANY) {
				break;
			}

			if (aType == TYPE_QUESTION && toQuestion()) {
				break;
			}

			if (aType == TYPE_BOOL && toBool()) {
				break;
			}

			if (aType == TYPE_INT && toInt()) {
				break;
			}

			if (aType == TYPE_DOUBLE && toDouble()) {
				break;
			}

			if (aType == TYPE_STRING && !input.empty()) {
				break;
			}

			core::Log::get()->log("Invalid input");
		}
	}

	bool getBool() const {
		auto t = type == TYPE_QUESTION ? toQuestion() : toBool();
		return *t;
	}

	const string& getString() const {
		return input;
	}

	int getInt() const {
		auto t = toInt();
		return *t;
	}

	int getDouble() const {
		auto t = toDouble();
		return *t;
	}
private:
	optional<int> toDouble() const {
		auto d = Util::toDouble(input);
		if (!numRange || (d >= (*numRange).first && d <= (*numRange).second)) {
			return d;
		}

		return nullptr;
	}

	optional<int> toInt() const {
		auto n = Util::toInt(input);
		if (!numRange || (n >= (*numRange).first && n <= (*numRange).second)) {
			return n;
		}

		return nullptr;
	}

	optional<bool> toQuestion() const {
		if (Util::stricmp(input, "yes") == 0 || Util::stricmp(input, "y") == 0) {
			return true;
		}

		if (Util::stricmp(input, "no") == 0 || Util::stricmp(input, "n") == 0) {
			return false;
		}

		return nullptr;
	}

	optional<bool> toBool() const {
		if (Util::stricmp(input, "1") == 0 || Util::stricmp(input, "true") == 0 || Util::stricmp(input, "enabled") == 0) {
			return true;
		}

		if (Util::stricmp(input, "0") == 0 || Util::stricmp(input, "false") == 0 || Util::stricmp(input, "disabled") == 0) {
			return false;
		}

		return nullptr;
	}

	Type type;
	optional<pair<double, double>> numRange;
	string input;
};

void Manager::start_client()
{
    //core::Log::get()->log("start_client: " + utils::to_string(utils::gettid()));
    const char *charset;
    bool utf8 = g_get_charset(&charset);
    if(utf8) {
        if(!core::Settings::get()->exists("utf8_input"))
            core::Settings::get()->set("utf8_input", true);
        if(!core::Settings::get()->exists("nmdc_charset"))
            core::Settings::get()->set("nmdc_charset", "ISO-8859-15");
    }

    if(!core::Settings::get()->exists("command_char"))
        core::Settings::get()->set("command_char", "/");

    core::Log::get()->log("Starting the client...");

	try {
		auto pu = unique_ptr<display::ProgressUpdater>(new display::ProgressUpdater);
		startup(
			[&](const string& aMsg) { core::Log::get()->log("Loading " + aMsg + "..."); },
			[](const string& aMsg, bool isQuestion, bool isError) {
				// message
				string msg(isError ? "ERROR: " : "");

				Prompter p;
				p.show(aMsg, isQuestion ? Prompter::TYPE_QUESTION : Prompter::TYPE_ANY);
				if (isQuestion)
					return p.getBool();
				return true;
			},
			[] {
				auto setInput = [](const string& text) {
					core::Log::get()->log("");
					display::Manager::get()->m_inputWindow.setInputStr(text);
				};

				// wizard
				Prompter p;
				p.show("Hello you new AirDC++ nano user! Let's start by going through a few general questions that help with configuring the optimal initial settings. Press enter when you feel like starting.", Prompter::TYPE_ANY);

				//if (SettingsManager::getInstance()->isDefault(SettingsManager::))
				setInput(SettingsManager::getInstance()->get(SettingsManager::NICK, false));
				p.show("Enter your nick", Prompter::TYPE_STRING);
				SettingsManager::getInstance()->set(SettingsManager::NICK, p.getString());

				auto curP = SettingsManager::getInstance()->get(SettingsManager::SETTINGS_PROFILE, false);
				setInput(SettingsManager::getInstance()->isDefault(SettingsManager::SETTINGS_PROFILE) ? "" : Util::toString(curP));
				core::Log::get()->log(STRING(NORMAL) + " (0): " + STRING(PROFILE_NORMAL_DESC));
				core::Log::get()->log(STRING(RAR_HUBS) + " (1): " + STRING(PROFILE_RAR_DESC));
				core::Log::get()->log(STRING(LAN_HUBS) + " (2): " + STRING(PROFILE_LAN_DESC));
				p.show("Select your settings profile", Prompter::TYPE_INT, make_pair(static_cast<double>(0), static_cast<double>(2)));
				SettingsManager::getInstance()->set(SettingsManager::SETTINGS_PROFILE, p.getInt());

				setInput(SettingsManager::getInstance()->get(SettingsManager::DOWNLOAD_SPEED, false));
				p.show("Enter your download speed (Mbit/s)", Prompter::TYPE_DOUBLE, make_pair(0.1, static_cast<double>(10000)));
				SettingsManager::getInstance()->set(SettingsManager::DOWNLOAD_SPEED, Util::toString(p.getDouble()));

				setInput(SettingsManager::getInstance()->get(SettingsManager::UPLOAD_SPEED, false));
				p.show("Enter your upload speed (Mbit/s)", Prompter::TYPE_DOUBLE, make_pair(0.1, static_cast<double>(10000)));
				SettingsManager::getInstance()->set(SettingsManager::UPLOAD_SPEED, Util::toString(p.getDouble()));

				p.show("We are all done here. Type /help to see all settings after the client has been loaded. For help and more information, visit www.airdcpp.net", Prompter::TYPE_ANY);
			},
			[&](double percent) {
				pu->updateStatus(percent);
			}
			);
	} catch (...) {
		events::emit("command quit");
		return;
	}

	initialized = true;

	SettingsManager::getInstance()->setDefault(SettingsManager::USE_UPLOAD_BUNDLES, false);
	SettingsManager::getInstance()->setDefault(SettingsManager::AUTO_REFRESH_TIME, 60);
	SettingsManager::getInstance()->setDefault(SettingsManager::AWAY_IDLE_TIME, 15);

	SettingsManager::getInstance()->setDefault(SettingsManager::SHOW_TRANSFERVIEW, true);
	SettingsManager::getInstance()->setDefault(SettingsManager::OPEN_FAVORITE_HUBS, true);
	SettingsManager::getInstance()->setDefault(SettingsManager::OPEN_PUBLIC, true);

    events::emit("command motd", std::string());

    // set random default nick
    if(SETTING(NICK).empty()) {
        auto nick = "airdcnano" + utils::to_string(Util::rand(1000, 50000));
        SettingsManager::getInstance()->set(SettingsManager::NICK, nick);
        core::Log::get()->log("Note: Using nick " + nick);
    }

    /*if(SETTING(DESCRIPTION).empty()) {
        SettingsManager::getInstance()->set(SettingsManager::DESCRIPTION,
                "nanodc user (http//nanodc.sf.net)");
    }*/

    const char *http_proxy = getenv("http_proxy");
    if(http_proxy) {
        SettingsManager::getInstance()->setDefault(SettingsManager::HTTP_PROXY, http_proxy);
        core::Log::get()->log("Note: Using HTTP proxy " + std::string(http_proxy));
    }

	TimerManager::getInstance()->start();
	events::emit("client created");
}

int Manager::run()
{
    auto nanodcrc = Util::getPath(Util::PATH_USER_CONFIG) + ".nanodcrc";
    core::Settings::create()->read(nanodcrc);

    core::Log::create();

    //events::create("client created");

    ui::Manager::create()->init();

    /* /q is alias for /quit */
	events::add_listener("command q", [] { events::emit("command quit"); });

    events::add_listener_last("command quit",
            std::bind(&events::Manager::quit, events::Manager::get()));

    events::add_listener("command quit",
            std::bind(&input::Manager::quit, input::Manager::get()));
    events::add_listener_first("command quit",
            std::bind(&core::Log::log, core::Log::get(),
                "Shutting down airdcnano...", core::MT_MSG));

	events::add_listener("command help", [] { display::Manager::get()->cmdMessage("/quit /q"); });

    std::thread input_thread(
                std::bind(&input::Manager::main_loop,
                    input::Manager::get()));

    std::thread client_starter(
                std::bind(&Manager::start_client, this));

    //core::Log::get()->log("Event loop: " + utils::to_string(utils::gettid()));
    events::Manager::get()->main_loop();

    input_thread.join(); 
    client_starter.join();
    return 0;
}

void Manager::shutdown()
{
    display::Manager::destroy();
    input::Manager::destroy();
    core::Log::destroy();
    core::Settings::destroy();

	if (initialized)
		dcpp::shutdown(nullptr, nullptr);
}

Manager::~Manager()
{
    shutdown();
}

} // namespace core
