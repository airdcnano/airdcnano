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
 *  Jussi Peltola <pelzi@pelzi.net>
 */

#include <core/events.h>
#include <display/scrolled_window.h>
#include <input/manager.h>
#include <utils/utils.h>
#include <utils/strings.h>
#include <utils/lock.h>
#include <display/manager.h>

namespace display {

ScrolledWindow::ScrolledWindow(const std::string& aID, display::Type aType) :
    m_scrollPosition(0),
    m_lastlogSize(200),
    m_timestamp("[%H:%M:%S] "),
	Window(aID, aType)
{
    m_bindings[KEY_PPAGE] = std::bind(&ScrolledWindow::scroll_window, this, -10);
    m_bindings[KEY_NPAGE] = std::bind(&ScrolledWindow::scroll_window, this, 10);

    update_config();
    m_lines.reserve(m_lastlogSize);
    m_state = STATE_NO_ACTIVITY;

}

void ScrolledWindow::clear() {
	m_lineLock.lock();
	m_lines.clear();
	m_scrollPosition = 0;
	m_lineLock.unlock();
}

void ScrolledWindow::update_config()
{
    m_lastlogSize = core::Settings::get()->find_int("lastlog_size", 200);
    m_timestamp = core::Settings::get()->find("timestamp_format", "[%H:%M:%S] ");
}

void ScrolledWindow::handle(wint_t key)
{
    if(m_bindings.find(key) != m_bindings.end()) {
        m_bindings[key]();
    }
    else if(key >= 0x20 && key < 0xFF) {
        m_input.key_insert(key);
        events::emit("window updated", static_cast<display::Window*>(this));
    }
    else {
        /* backspace, arrow keys.. */
        m_input.pressed(key);
        events::emit("window updated", static_cast<display::Window*>(this));
    }
}

void ScrolledWindow::add_line(const display::LineEntry &line_,
        bool redraw_screen /* = true */)
{
    m_lineLock.lock();
    if(m_lines.size() >= m_lastlogSize)
        m_lines.erase(m_lines.begin());

    display::LineEntry line = koskenkorva_viina(line_);

    /* scroll the window if we are at the bottom */
    if(m_scrollPosition == m_lines.size()) {
        m_scrollPosition++;
    }

    m_lines.push_back(line);
    m_lineLock.unlock();

    if(redraw_screen && m_state == STATE_IS_ACTIVE) {
        events::emit("window updated", this);
    }
    else if(m_state != STATE_IS_ACTIVE) {
        if(line.m_type == LineEntry::HIGHLIGHT || line.m_type == LineEntry::ACTIVITY_ERROR) {
            m_state = STATE_HIGHLIGHT;
        }
        else if(line.m_type == LineEntry::MESSAGE && m_state != STATE_HIGHLIGHT) {
            m_state = STATE_MESSAGES;
        }
        else if(line.m_type == LineEntry::ACTIVITY && m_state != STATE_HIGHLIGHT
                && m_state != STATE_MESSAGES)
        {
            m_state = STATE_ACTIVITY;
        }
        events::emit("window status updated", static_cast<Window*>(this), m_state);
    }
}

LineEntry ScrolledWindow::koskenkorva_viina(const display::LineEntry &line_)
{
    auto line = line_;
    std::string text;
 
    text = utils::time_to_string(m_timestamp, line.m_time);
    line.m_indent += text.length();

	if (line.m_type == LineEntry::ACTIVITY_ERROR) {
		line.m_text = "%04" + line.m_text + "%04";
	}

	if (line.m_type == LineEntry::ACTIVITY || line.m_type == LineEntry::ACTIVITY_ERROR)
        text += "%21%01-%01!%01-%01%21 ";

    line.m_text = text + line.str();
    return line;
}

void ScrolledWindow::scroll_window(int lines)
{
    if(static_cast<int>(m_scrollPosition)+lines < 0)
        m_scrollPosition = 0;
    else
        m_scrollPosition += lines;

    if(m_scrollPosition > m_lines.size())
        m_scrollPosition = m_lines.size();

    events::emit("window updated", this);
}

void ScrolledWindow::redraw()
{
    resize();
    unsigned int window_height = get_height();
    unsigned int window_width = get_width();

    /* Find out the first message to print */
    int first = m_scrollPosition;
    int line = std::min<size_t>(get_height(), m_lines.size());

    while(first > 0 && line > 0) {
        LineEntry lineEntry;
        try {
            lineEntry = m_lines.at(first-1);
        } catch(std::exception &e) {
            set_title("bUG in ScrolledWindow::redraw" +
                    std::string(e.what() + utils::to_string(first)));
        }
        line -= Window::calculate_height(lineEntry.str(), window_width,
                    lineEntry.m_indent);
        first--;
    }

    /* Iterator to the first message to print */
    auto it = m_lines.end() - (m_lines.size()-first);

    Window::erase();

    for(size_t height=0; it != m_lines.end() && height < window_height; ++it) {
        std::string message = it->str();

        std::string::size_type i = 0;
        while(i < message.length()) {
            std::string indentation;
            std::string::size_type tmp = i;
            if(i == 0) {
                i = Window::find_line_end(message, i, window_width);
            }
            else {
                i = Window::find_line_end(message, i, window_width-it->m_indent);
                indentation = std::string(it->m_indent, ' ');
            }

            /* line is < 0 if the first message doesn't
             * fit completely on the screen */
            if(++line > 0) {
                print(indentation + message.substr(tmp, i-tmp+1), 0, height++);
                i++;
            }

            for(; i < message.length() && isspace(message[i]); i++);
        }

        Window::clear_flags();
    }

    Window::refresh();
}

} // namespace display

