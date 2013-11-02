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

#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <stdexcept>
#include <string>

#include <utils/ncurses.h>
#include <utils/mutex.h>
#include <utils/lock.h>

namespace display {

/** Wrapper for general ncurses functions. */
class Screen {
public:
    /** Initialize the curses-stuff.
     * @throw std::runtime_error if something fails. */
    static void initialize() throw(std::runtime_error);

    /** Returns the width of the screen. */
	static unsigned int get_width() { return get_xymax().first; }

    /** Returns the height of the screen. */
	static unsigned int get_height() { return get_xymax().second; }

    /** Check if screen has been resized since last call.
     * @return True if screen has been resized, false otherwise. */
    static bool is_resized();

    /** Copy the buffer to the screen. */
    static void do_update() { utils::Lock lock(m_mutex); doupdate(); }

    /** Locks the screen mutex. */
    static void lock() { m_mutex.lock(); }

    /** Unlocks the screen mutex. */
    static void unlock() { m_mutex.unlock(); }
private:
	static std::pair<unsigned int, unsigned int> get_xymax() { int x, y; getmaxyx(stdscr, y, x); return{ x, y }; }

    Screen(); //!< Forbidden
    Screen(const Screen &); //!< Forbidden
    Screen& operator=(const Screen &); //!< Forbidden
    static utils::Mutex m_mutex; //!< The screen lock
};

} // namespace display

#endif // _SCREEN_H_
