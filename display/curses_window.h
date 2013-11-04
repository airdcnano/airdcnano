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

#ifndef _CURSESWINDOW_H_
#define _CURSESWINDOW_H_

#include <utils/ncurses.h>

#include <string>

namespace display {

/** Wrapper class for ncurses window drawing operations.
 * User may not directly print to screen but he must lock
 * the screen using lock() method of this class. Unlocking
 * is made using the unlock() method.*/
class CursesWindow {
public:
    /** Constructor. */
    CursesWindow(int x = 0, int y = 0, int width = 0, int height = 0);

    /** Resize the window.
     * @param x The new x-pos.
     * @param y The new y-pos.
     * @param w The new width. 
     * @param h The new height. */
    void resize(int x, int y, int w, int h);

    /** Refresh the window. */
    void refresh() { wnoutrefresh(m_window); }

    /** Erase the contents of the window. */
    void erase() { werase(m_window); }

    /** Clear the window. */
    void clear() { wclear(m_window); }

	std::pair<unsigned int, unsigned int> get_xymax() const { int x, y; getmaxyx(m_window, y, x); return{ x, y }; }

    /** Get the width of the window. */
	unsigned int get_width() const { return get_xymax().first; }

    /** Get the height of the window. */
	unsigned int get_height() const { return get_xymax().second; }

	
	std::pair<unsigned int, unsigned int> get_xy() const { int x, y; getyx(m_window, y, x); return{ x, y }; }
    /** Get the x-position of the window. */
	int get_x() const { return get_xy().first; }

    /** Get the y-position of the window. */
	int get_y() const { return get_xy().second; }

    /**
        \brief Print text to the window.

        You can format the printed text with %xx syntax.
        You can print "%" using syntax "%%".

        - 01 blue
        - 02 green
        - 03 cyan
        - 04 red
        - 05 magenta
        - 06 yellow
        - 07 white
        - 08 black
        - 09-15 = 01-07 with blue background
        - 21 bold
        - 22 underline
        - 23 reverse colors

        Example: "%21bold and %02green%02%21 text"

        @param str Formatted text to print.
        @param x The x-coordinate of the window
        @param y The y-coordinate of the window
        @param colors Whether to use the %xx syntax for printing colors
    */
    void print(const std::string &str, int x=0, int y=0, bool colors=true);

    /** Set the background color of the window. */
    void set_background(int color) { wbkgdset(m_window, ' ' | COLOR_PAIR(color)); }

    /** Move the cursor. */
    void move(int x, int y) { wmove(m_window, y, x); }

    /** Get the pointer to the internal ncurses WINDOW-structure. */
    WINDOW *get_window() const { return m_window; }

    /** A wrapper for mvwchgat. */
    void set_attr(int x, int y, int n, int attr, int color) { mvwchgat(m_window, y, x, n, attr, color, NULL); }

    /** Load formatting flags from string. */
    void set_flags(const std::string &flags);

    /** Clear printing state flags. */
    void clear_flags() { m_flags = 0; }

    /** Destructor. */
    virtual ~CursesWindow();
private:
    CursesWindow(const CursesWindow &); //!< Forbidden
    CursesWindow& operator=(const CursesWindow &); //!< Forbidden
    WINDOW *m_window; //!< The ncurses WINDOW pointer
    int m_flags; //!< Used to save formatting flags between print() calls
};

} // namespace display

#endif // _CURSESWINDOW_H_
