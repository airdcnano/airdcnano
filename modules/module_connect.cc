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

#include <core/log.h>
#include <core/events.h>
#include <core/argparser.h>
#include <ui/window_hub.h>
#include <display/manager.h>
#include <input/help_handler.h>

#include <client/FavoriteManager.h>
#include <client/ShareManager.h>

namespace modules {


class Connect
{
public: 
	HelpHandler::CommandList commands = {
		{ "connect", std::bind(&Connect::connect_callback, this), COMPLETION(Connect::handleSuggest) }
	};

	HelpHandler help; 

    Connect() : help(&commands, "Connect") {

    }

    void connect_callback()
    {
		if (events::args() == 0) {
			display::Manager::get()->cmdMessage("Usage: /connect <address> [<profile>]");
			return;
		}

		core::ArgParser parser(events::arg<std::string>(0));
		parser.parse();

		auto args = parser.args();

		connect(parser.arg(0),
			args > 1 ? parser.arg(1) : "", // share profile
            true); // activate
    }

    /** Connect to a hub */
	void connect(const std::string &address, const std::string &shareProfile, bool activate)
    {
		auto mger = display::Manager::get();

		auto profileToken = ShareManager::getInstance()->getProfileByName(shareProfile);
		if (profileToken) {
			if (!shareProfile.empty() && !AirUtil::isAdcHub(address) && *profileToken != SETTING(DEFAULT_SP)) {
				display::Manager::get()->cmdMessage("Custom share profiles can't be used in NMDC hubs");
				return;
			}

			ui::WindowHub::openWindow(address, *profileToken, activate);
		} else {
			mger->cmdMessage("Profile not found");
		}
    }

	void handleSuggest(const StringList& aArgs, int pos, StringList& suggest_) {
		if (pos == 1) {
			auto hubs = FavoriteManager::getInstance()->getRecentHubs();
			for (const auto& h : hubs) {
				suggest_.push_back(h->getServer());
			}
		} else if (pos == 2 && AirUtil::isAdcHub(aArgs[1])) {
			auto& profiles = ShareManager::getInstance()->getProfiles();
			for (const auto& p : profiles) {
				suggest_.push_back(p->getPlainName());
			}
		}
	}
};

} // namespace modules

static modules::Connect initialize;

