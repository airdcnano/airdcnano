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

#ifndef _STATUSBAR_H_
#define _STATUSBAR_H_

#include <string>
#include <deque>
#include <client/stdinc.h>
#include <client/DCPlusPlus.h>
#include <client/TimerManager.h>

#include <display/curses_window.h>
#include <display/manager.h>
#include <display/status_item.h>
#include <utils/instance.h>

namespace display {

class StatusBar:
    public utils::Instance<display::StatusBar>,
    public display::CursesWindow
{
public:
    /** Constructor. */
    StatusBar();

    /** Redraws the status bar. */
    virtual void redraw();

    /** Called when the config file is changed. */
    void update_config();

    /** Add an item to the status bar.
     * @param item The item.
     * @param pos The position of the item.
     * @throw std::runtime_error if item with same name already exists */
    void add_item(display::StatusItem *item, int pos = -1);

    /** Remove an item from the status bar. Doesn't free the memory of the item.
     * @param name The name of the item. 
     * @throw std::runtime_error if item is not found.
     * @return Returns the item. */
    StatusItem *remove_item(const std::string &name);

    /** Remove an item from the status bar and free the memory.
     * @param name The name of the item.
     * @throw std::runtime_error if item is not found. */
    void free_item(const std::string &name);

    /** Update an item.
     * @param name Items name.
     * @throw std::runtime_error if the item doesn't exist*/
    void update_item(const std::string &name);

    /** Destructor. Frees the memory of all items. */
    ~StatusBar();

    typedef std::deque<display::StatusItem*> StatusItems;
private:
    std::string m_statusline;
    StatusItems m_items;
};

} // namespace display

#endif // _STATUSBAR_H_
