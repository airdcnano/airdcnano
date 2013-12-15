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
#include <core/events.h>
#include <ui/status_windowinfo.h>

#include <client/Util.h>

namespace ui {

StatusWindowInfo::StatusWindowInfo() : StatusItem("window")
{
    events::add_listener("window changed",
        std::bind(&StatusWindowInfo::update, this));

    update();
}

void StatusWindowInfo::update()
{
    display::Manager *dm = display::Manager::get();
    display::Window *window = dm->get_current_window();
    unsigned int number = std::distance(dm->begin(), dm->get_current()) + 1;
    m_text = dcpp::Util::toString(number) + ":" + window->get_name();
    events::emit("statusbar updated");
}

} // namespace ui
