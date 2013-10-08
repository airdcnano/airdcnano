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

#ifndef _STATUSWINDOWLIST_H_
#define _STATUSWINDOWLIST_H_

#include <display/status_bar.h>
#include <display/status_item.h>

namespace ui {

class StatusWindowList:
    public display::StatusItem
{
public:
    /** Constuctor. Registers event handlers. */
    StatusWindowList();
    virtual void update();
private:
    /** Handle "window closed" event and remove the window from m_list if it's there. */
    void window_closed();

    /** Handle window status updates. */
    void window_status_updated();

    /** Helper typedef. */
    typedef std::map<display::Window*, display::State> WindowList;
    WindowList m_list;
};

} // namespace ui

#endif // _STATUSWINDOWLIST_H_
