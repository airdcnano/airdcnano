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

#include <display/input_window.h>
#include <display/screen.h>
#include <utils/utils.h>
#include <core/log.h>

#include <glib.h>

namespace display {

InputWindow::InputWindow():
    CursesWindow(0, Screen::get_height()-1, Screen::get_width(), 1),
    m_prompt("[airdcnano] "),
    m_input(0)
{
    
}

void InputWindow::redraw() {
    int len = m_prompt.length()+1;
    auto line = m_prompt + " ";
    auto width = get_width();
    if(m_input) {
        auto pos = m_input->get_pos();
        if (scroll_position > pos) {
            scroll_position = std::min(scroll_position < 5 ? 0 : scroll_position-5, pos);
        } else if ((pos-scroll_position) + len > width-5) {
            scroll_position += 5;
        }

        const gchar *message_start = (gchar *) m_input->str().data();
        if (scroll_position > 0) {
            const auto start = g_utf8_offset_to_pointer(message_start, scroll_position);
            auto realPos = start-message_start;
            line += m_input->str().substr(realPos, m_input->str().length() - realPos);
        } else {
            line += m_input->str();
        }

        if (line.length() > width) {
            line.substr(0, width);
        }


        len += m_input->get_pos()-scroll_position;
    }

    erase();
    print(line, 0, 0, false);
    move(len, 0);
    refresh();
}

const std::string& InputWindow::getInputStr() const {
	return m_input->str();
}

void InputWindow::setInputStr(const std::string& aText) {
	m_input->assign(aText);
	m_input->set_pos(aText.length());
}

} // namespace display

