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
#include <ui/window_search.h>
#include <display/manager.h>
#include <client/FavoriteManager.h>

#include <input/help_handler.h>

namespace modules {

class Search
{
public:
	HelpHandler::CommandList commands = {
		{ "search", std::bind(&Search::search_callback, this), nullptr }
	};

	HelpHandler help;

    Search() : help(&commands, "Search") {
        //events::add_listener("command search",
         //       std::bind(&Search::search_callback, this));
    }

    /** /search string */
    void search_callback()
    {
        core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
        parser.parse();

        if(parser.args() < 1) {
			display::Manager::get()->cmdMessage("Usage: /search <search string>");
            return;
        }

        /* search string */
        auto str = parser.get_text(0);

        auto dm = display::Manager::get();
		auto it = dm->find(display::TYPE_SEARCHWINDOW, str);

        if(it != dm->end()) {
            dm->set_current(it);
            return;
        }
        /* don't allow this search string if a window with this name already exists */
        else if(it != dm->end()) {
            core::Log::get()->log("Search::search_callback(): Invalid search string");
            return;
        }

        auto wnd = new ui::WindowSearch(str);
        dm->push_back(wnd);
        dm->set_current(it);
    }
};

} // namespace modules

static modules::Search initialize;

