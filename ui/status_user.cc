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

#include <functional>
#include <ui/status_user.h>
#include <core/events.h>

namespace ui {

StatusUser::StatusUser() : StatusItem("user")
{
    events::add_listener("window changed",
            std::bind(&StatusUser::update, this));
	events::add_listener("nick changed",
		std::bind(&StatusUser::update, this));
    update();
}

void StatusUser::update()
{
    display::Window *window = *display::Manager::get()->get_current();
    switch(window->get_type()) {
        case display::TYPE_HUBWINDOW:
			m_text = static_cast<ui::WindowHub*>(window)->get_nick();
            break;
        case display::TYPE_PRIVMSG:
            m_text = static_cast<ui::WindowPrivateMessage*>(window)->get_nick();
            break;
        default:
			m_text = SETTING(NICK);
    }
}

} // namespace ui
