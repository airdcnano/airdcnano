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

#include <numeric>
#include <sstream>
#include <functional>
#include <ui/status_windowlist.h>
#include <core/events.h>
#include <core/log.h>

namespace ui {

StatusWindowList::StatusWindowList()
{
    set_name("windows");
    events::add_listener("window status updated",
            std::bind(&StatusWindowList::window_status_updated, this));

    events::add_listener("window closed",
            std::bind(&StatusWindowList::window_closed, this));
}

void StatusWindowList::window_closed()
{
    display::Window *window = events::arg<display::Window*>(0);
    if(m_list.find(window) != m_list.end()) {
        if(m_list.erase(window) != 1)
            throw std::logic_error("m_list.erase(window) != 1");
    }
}

void StatusWindowList::window_status_updated()
{
    display::Window *window = events::arg<display::Window*>(0);
    display::State state = window->get_state();

    /* don't list the window if it's active */
    if(state == display::STATE_IS_ACTIVE &&
            m_list.find(window) != m_list.end())
    {
        if(m_list.erase(window) != 1)
            throw std::logic_error("m_list.erase(window) != 1");
    }
    else {
        m_list[window] = state;
    }
    update();
    events::emit("statusbar updated");
}

void StatusWindowList::update()
{
    std::stringstream activity;
    display::Manager *mger = display::Manager::get();

    for(const auto& w: m_list)
    {
        int i = std::distance(mger->begin(), mger->find(w.first->get_type(), w.first->getID()))+1;
        switch(w.second) {
            case display::STATE_ACTIVITY:
                activity << "%11" << i << "%11"; // cyan on blue
                break;
            case display::STATE_MESSAGES:
                activity << "%21%15" << i << "%21%15"; // white on blue
                break;
            case display::STATE_HIGHLIGHT:
                activity << "%21%13" << i << "%21%13"; // magneta on blue
                break;
            default:
                continue;
        }

        activity << "%11,%11";
    }

    set_text(activity.str());
    if(get_text().length() >= 7)
        set_text(get_text().substr(0, get_text().length()-7));
    set_text("%15" + get_text() + "%15");
}

} // namespace ui
