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
#include <ui/window_privatemessage.h>
#include <display/manager.h>

#include <client/ClientManager.h>
#include <client/FavoriteManager.h>
#include <client/forward.h>
#include <client/HintedUser.h>
#include <client/User.h>
#include <client/OnlineUser.h>

using namespace dcpp;

namespace modules {

class Msg
{
public:
    Msg() {
        //events::add_listener("command msg",
        //        std::bind(&Msg::msg_callback, this));
    }

    /** /msg nick message */
    void msg_callback()
    {
        core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
        parser.parse();

        if(parser.args() < 2) {
			display::Manager::get()->cmdMessage("Not enough parameters given");
            return;
        }

        auto nick = parser.arg(0);

        auto dm = display::Manager::get();
        auto it = dm->get_current();
        if((*it)->get_type() != display::TYPE_HUBWINDOW) {
			display::Manager::get()->cmdMessage("Not connected to a hub");
            return;
        }

        auto hub = static_cast<ui::WindowHub*>(*it);

		try {
			auto user = hub->get_user(nick);
			it = dm->find(display::TYPE_PRIVMSG, user->getUser()->getCID().toBase32());
			if (it == dm->end()) {
				auto pm = new ui::WindowPrivateMessage(HintedUser(user->getUser(), user->getHubUrl()), hub->get_client()->getMyNick());
				dm->push_back(pm);
				dm->set_current(it);
			}
			/* all text after first argument (nick) */
			auto line = parser.get_text(1);
			(*it)->handle_line(line);
		} catch (...) {
			display::Manager::get()->cmdMessage("User " + nick + " not found");
		}
    }
};

} // namespace modules

static modules::Msg initialize;

