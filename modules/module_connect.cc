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
		{ "connect", std::bind(&Connect::connect_callback, this), nullptr }
	};

	HelpHandler help; 

    Connect() : help(&commands, "Connect") {
		//init(commands, "Connect");
        /*events::add_listener("command connect",
                std::bind(&Connect::connect_callback, this));

        events::add_listener("command disconnect",
                std::bind(&Connect::disconnect, this,
                    std::bind(&events::arg<std::string>, 0)));*/

        //events::add_listener("command reconnect",
       //         std::bind(&Connect::reconnect_callback, this));
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

    void reconnect_callback()
    {
        auto address = events::arg<std::string>(0);

		auto mger = display::Manager::get();
		auto it = mger->find(display::TYPE_HUBWINDOW, address);
		if (it != mger->end()) {
			static_cast<ui::WindowHub*>(*it)->reconnect();
		}

        //disconnect(address);
        //connect(address, "", "", "", false);
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

		/*auto it = mger->find(display::TYPE_HUBWINDOW, address);

        if(it != mger->end()) {
            auto hub = static_cast<ui::WindowHub*>(*it);
            if(!hub->get_client()->isConnected())
                hub->connect();
            mger->set_current(it);
        } else {
            ui::WindowHub *hub = new ui::WindowHub(address);
			hub->connect(address, nick, password, description, *getProfileToken(shareProfile));
            mger->push_back(hub);
            if(activate)
                mger->set_current(it);
        }*/
    }

    /** Disconnects a hub.
     * @param hub Hub url to disconnect */
    void disconnect(const std::string &hub) {
        display::Manager *mger = display::Manager::get();
        auto it = mger->get_current();

        if(!hub.empty()) {
			it = mger->find(display::TYPE_HUBWINDOW, hub);
        }

        if(it != mger->end() && (*it)->get_type() == display::TYPE_HUBWINDOW) {
            auto hub = static_cast<ui::WindowHub*>(*it);
            if(hub->get_client()->isConnected())
                hub->get_client()->disconnect(false);
        }
    }

    std::vector<std::string> complete()
    {
        //int nth = events::arg<int>(0);
        std::string command = events::arg<std::string>(1);

        StringList ret;
        if(command == "connect") {
            auto list = FavoriteManager::getInstance()->getFavoriteHubs();
            for(const auto& f: list) {
                ret.push_back(f->getServers()[0].first);
            }
        }
        return ret;
    }
};

} // namespace modules

static modules::Connect initialize;

