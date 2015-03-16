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

#include <sstream>
#include <stdexcept>
#include <functional>
#include <display/window.h>
#include <display/manager.h>
#include <core/events.h>
#include <core/log.h>

#include <boost/algorithm/string/trim.hpp>

namespace modules {

class LineHandler
{
public:
    LineHandler():
        m_commandChar('/')
    {
        events::add_listener_first("key pressed",
                std::bind(&LineHandler::key_pressed, this));
    }

    void key_pressed()
    {
        auto key = events::arg<wint_t>(1);
        /* enter */
        if(key != 0xA)
            return;

	auto wnd = display::Manager::get()->get_current_window();
	auto line = display::Window::m_input.str();
	boost::trim_right(line);

        if(!line.empty() && line[0] == m_commandChar && wnd->allowCommands) {
            auto space = line.find(' ');
            auto command = space != std::string::npos ? line.substr(1, space-1) : line.substr(1);
			auto params = space != std::string::npos ? line.substr(space + 1) : "";
			if (params.empty()) {
				events::emit("command " + command);
				events::emit("command", command);
			} else {
				events::emit("command " + command, params);
				events::emit("command", command, params);
			}
        } else {
            wnd->handle_line(line);
        }
    }
private:
    char m_commandChar;
};

} // namespace modules

static modules::LineHandler initialize;

