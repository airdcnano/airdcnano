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

#include <display/screen.h>
#include <utils/utils.h>

namespace display {

void Screen::initialize()
    throw (std::runtime_error)
{
    if(initscr() == 0)
        throw std::runtime_error("initscr() failed");

    curs_set(1); // use cursor

    if(has_colors()) {
        start_color();
        use_default_colors();

        char ansi_tab[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
        for(int num = 1; num < COLOR_PAIRS; num++)
            init_pair(num, ansi_tab[num & 7], num < 9 ? -1 : ansi_tab[num >> 3]);
        init_pair(63, 0, -1);
    }
}

bool Screen::is_resized() {
    static unsigned int x = 0;
    static unsigned int y = 0;

    bool resized = display::Screen::get_width()  != x ||
                   display::Screen::get_height() != y;

    x = display::Screen::get_width();
    y = display::Screen::get_height();

    return resized;
}

} // namespace display

