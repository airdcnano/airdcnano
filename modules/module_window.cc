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

#include <utils/utils.h>
#include <core/events.h>
#include <core/log.h>
#include <core/argparser.h>
#include <display/manager.h>
#include <input/help_handler.h>

#include <ui/window_favorites.h>
#include <ui/window_log.h>
#include <ui/window_publichubs.h>
#include <ui/window_transfers.h>

#include <client/Util.h>

using namespace dcpp;

namespace modules {

class Window
{
public:
	template <class T>
	static void handleOpenTab(display::Type aType) {
		auto mger = display::Manager::get();
		auto it = mger->find(aType);
		if (it == mger->end()) {
			auto w = new T();
			mger->push_back(w);
		}

		mger->set_current(it);
	}

	HelpHandler::CommandList commands = {
		{ "clear", std::bind(&Window::handleClear, this), nullptr },
		{ "window", std::bind(&Window::window_callback, this), nullptr },
		{ "wc", std::bind(&Window::close, this), nullptr },
		{ "favorites", [this] { handleOpenTab<ui::WindowFavorites>(display::TYPE_FAVORITES); }, nullptr },
		{ "syslog", [this] { handleOpenTab<ui::WindowLog>(display::TYPE_LOGWND); }, nullptr },
		{ "hubs", [this] { handleOpenTab<ui::WindowHubs>(display::TYPE_HUBLIST); }, nullptr },
		{ "transfers", [this] { handleOpenTab<ui::WindowTransfers>(display::TYPE_TRANSFERS); }, nullptr }
	};

	HelpHandler help;

    Window() : help(&commands, "Window") {
        /*events::add_listener("command window",
            std::bind(&Window::window_callback, this));

        events::add_listener("command wc",
			std::bind(&Window::close, this));*/
    }

	void handleClear() {
		auto cur = *display::Manager::get()->get_current();
		if (cur->get_type() == display::TYPE_LOGWND || cur->get_type() == display::TYPE_HUBWINDOW || cur->get_type() == display::TYPE_PRIVMSG)
			static_cast<display::ScrolledWindow*>(cur)->clear();
	}

    void window_callback() {
		if (events::args() == 0) {
			display::Manager::get()->cmdMessage("Usage: /window <list|close|move|prev|next|[0-X]>");
			return;
		}

		core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
		parser.parse();
		auto command = parser.arg(0);
        auto mger = display::Manager::get();
        auto current = mger->get_current();

        if(command == "close") {
			close();
        } else if (command == "move") {
			if (parser.args() < 2) {
				mger->cmdMessage("Usage: /window move <new pos>");
				return;
			}

			auto newPos = Util::toInt(parser.arg(1));
			if (newPos < 1 || newPos > static_cast<int>(mger->size())) {
				mger->cmdMessage("Invalid position");
				return;
			}

			auto p = mger->begin() + newPos - 1;
			if (p < current) {
				std::rotate(p, current, current+1);
			} else if (current < p) {
				std::rotate(current, current+1, p + 1);
			}
			mger->set_current(mger->begin() + newPos - 1);
		} else if (command == "next") {
			mger->next();
		} else if(command == "prev") {
			mger->prev();
		} else if (command == "list") {
			mger->cmdMessage("Open windows:");
			int pos = 1;
			for (auto i = mger->begin(); i != mger->end(); ++i, ++pos) {
				mger->cmdMessage(Util::toString(pos) + ": " + (*i)->get_name());
			}
		} else if (!command.empty() && (Util::toInt(command) >= 0 && Util::toInt(command) <= static_cast<int>(mger->size()))) {
			auto pos = Util::toInt(command) - 1;
			if (pos <= static_cast<int>(mger->size())) {
				auto newCurrent = mger->begin() + pos;
				mger->set_current(newCurrent);
			}
		}
    }

	void close() {
		auto it = display::Manager::get()->get_current();
		display::Manager::get()->remove(*it);
	}

   /* void print_help(const Parameter &param) {
        if(param.get_command() == "window") {
            core::Log::get()->log("Usage: /window n|move|close");
        }
    }

    std::vector<std::string> complete(int n, const Parameter &param)
    {
        if(param.get_command() == "window") {
            if(n == 0) {
                return utils::make_vector(4, "move", "close", "next", "prev");
            }
            else if(n == 1) {
                if(param.get_param(0) == "move") {
                    return utils::make_vector(4, "next", "prev", "first", "last");
                }
            }
        }
        return std::vector<std::string>();
    }*/
//#endif
};
} // namespace modules

static modules::Window initialize;


