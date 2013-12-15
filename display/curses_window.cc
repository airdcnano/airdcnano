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

#include <iostream>
#include <stdexcept>
#include <display/curses_window.h>

#include <client/stdinc.h>
#include <client/Text.h>
#include <client/Util.h>

namespace display {

CursesWindow::CursesWindow(int x, int y, int width, int height):
    m_window(newwin(height, width, y, x)),
    m_flags(0)
{
}

void CursesWindow::set_flags(const std::string &flags)
{
    int code = dcpp::Util::toInt(flags);
    switch(code) {
        case 21:
            m_flags ^= A_BOLD;
            break;
        case 22:
            m_flags ^= A_UNDERLINE;
            break;
        case 23:
            m_flags ^= A_REVERSE;
            break;
        default:
            if(code > 0 && code < COLOR_PAIRS-1) {
                m_flags ^= COLOR_PAIR(code);
            }
    }
}

void CursesWindow::print(const std::string &text, int x, int y, bool colors)
{
    wmove(m_window, y, x);

    //std::string str = dcpp::Text::fromUtf8(text);
    int width = get_width();
	for (unsigned int i = 0; i < text.length() && m_window && getcurx(m_window) < width; ++i)
    {
		unsigned char c = text.at(i);
        if(colors && c == '%') {
			if (text.length() > ++i && text.at(i) != '%') {
				set_flags(text.substr(i, 2));
                ++i;
                continue;
            }
        }
        waddch(m_window, c | m_flags);
    }
}

void CursesWindow::resize(int x, int y, int w, int h) { 
    //if (h < 0 || w < 0)
    //    throw std::logic_error("CursesWindow::resize(): bad h or w position");

   // if (x < 0 || y < 0)
    //    throw std::logic_error("CursesWindow::resize(): bad x or y position");
#if 0
    wmove(m_window, y, x);
    wresize(m_window, h, w);
#else
    delwin(m_window);
    m_window = newwin(h, w, y, x);
#endif
}

CursesWindow::~CursesWindow()
{
    delwin(m_window);
}

} // namespace display

