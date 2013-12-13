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

#include <stdexcept>
#include <iostream>
#include <functional>
#include <display/manager.h>
#include <display/window.h>
#include <display/screen.h>
#include <input/manager.h>
#include <utils/utils.h>
#include <core/log.h>
#include <client/Text.h>

namespace display {

input::TextInput Window::m_input;

Window::Window(const std::string& aID, Type aType, bool aAllowCommands) :
	id(aID),
    m_name("window"),
    m_title("title"),
    m_titleWindow(),
	m_type(aType),
    m_state(STATE_NO_ACTIVITY),
    m_bindings(),
    m_insertMode(true),
    m_drawTitle(true),
	allowCommands(aAllowCommands),
	asyncConn(events::add_listener("async" + id, std::bind(&Window::handleAsync, this)))
{
    // ^X
    m_bindings['X' - '@'] = std::bind(&display::Manager::remove,
                                    std::bind(&display::Manager::get),
                                    this);
	m_bindings[KEY_ESCAPE] = std::bind(&Window::handleEscape, this);
    set_prompt();
    resize();
}

static const char *default_prompt = "[airdcnano]";
void Window::set_prompt() {
	m_prompt = default_prompt; 
}

void Window::set_prompt(const std::string &prompt) {
	m_prompt = prompt; 
}

void Window::resize()
{
    int w = Screen::get_width();
    int h = Screen::get_height();

    if(m_drawTitle) {
        m_titleWindow.resize(0, 0, w, 1);
        m_titleWindow.set_background(10); // blue
    }

    CursesWindow::resize(0, m_drawTitle ? 1 : 0, w, h-3 > 0 ? h-3 : 1);
}

void Window::draw()
{
    if(m_drawTitle) {
        m_titleWindow.erase();
        m_titleWindow.print("%15 " + m_title + "%15");
        m_titleWindow.refresh();
    }

    redraw();
}

unsigned int Window::calculate_height(const std::string& message,
        unsigned int screen_width, unsigned int indent)
{
    const gchar *message_start = (gchar *) message.data();
    const gchar *message_end = (gchar *) message.data() + (message.length()-1);
    return calculate_height(message_start, message_end, screen_width, indent);
}

unsigned int Window::calculate_height(const gchar *message_start,
        const gchar *message_end, unsigned int screen_width,
        unsigned int indent)
{
    if(screen_width < 1)
        return 0;
    if(screen_width - indent < 4)
        indent = 0;

    const gchar *i = message_start;         // Current character
    unsigned int lines = 0;                 // Line count

    while(i <= message_end)
    {
		i = find_line_end(i, message_end, screen_width - (i == message_start ? 0 : indent));
        lines++;

        /* Move to the beginning of the next line. find_line_end returns the
         * last byte of the UTF-8 character, so this is safe.
         */
        i++;

        // Skip whitespace
        while(i <= message_end && g_unichar_isspace(g_utf8_get_char(i)))
            i = g_utf8_next_char(i);
    }
    return lines;
}

std::string::size_type Window::find_line_end(const std::string& message,
        std::string::size_type start, unsigned int screen_width)
{
    const gchar *line_start = message.data() + start;
    const gchar *message_end = message.data() + (message.length() - 1);
    return find_line_end(line_start, message_end, screen_width)
        - message.data();
}

const gchar *Window::find_line_end(const gchar *line_start,
        const gchar *message_end, unsigned int screen_width)
{
    //if(line_start > message_end)
    //    throw std::invalid_argument("line_start > message_end");
    //if(screen_width == 0)
     //   throw std::invalid_argument("screen_width == 0");

    /* The maximum amount of visible characters is the number of bytes.
     * If the number of remaining bytes is less than or equal to the screen
     * width it will always fit. The cast is to silence warning about
     * comparison between signed and unsigned types, since line_start is
     * guaranteed to be less than or equal to message_end.
     */
    //if(message_end - line_start + 1 <= (int) screen_width)
    //    return message_end;

    const unsigned int overlong_word_length = screen_width / 2;

    const gchar *last_word_end = 0;
    const gchar *i = line_start;        // current character
    unsigned int j = screen_width;      // remaining space
    unsigned int current_word_length = 0;
    while (i <= message_end && g_utf8_next_char(i) <= message_end)
    {
        /* Handle formatting sequences.
         * Invalid sequences are not handled - this is intended!
         */
        if(*i == '%')
        {
            if(*(i+1) == '%')
            {
                i += 2;
                current_word_length++;
                j--;                    // "%%" is displayed as "%"
			} else {
				i += 3;
			}
            continue;
        }
        if(j < 2) // Must be done here - otherwise formatting will be damaged.
            break;
        if(g_unichar_isspace(g_utf8_get_char(i)))
            current_word_length = 0;
        else if(g_unichar_isspace(g_utf8_get_char(g_utf8_next_char(i))))
            last_word_end = g_utf8_next_char(i) - 1;

        i = g_utf8_next_char(i);
        current_word_length++;
        j--;
    }
    if(i > message_end)
        return message_end;
    if(!(g_utf8_next_char(i) <= message_end))
        return message_end;
    if(j < 2)
    {
        // Check if a word boundary follows
        if(g_utf8_next_char(i) <= message_end
                && g_unichar_isspace(g_utf8_get_char(g_utf8_next_char(i))))
            return g_utf8_next_char(i) - 1; // Last byte of last character
        if(current_word_length <= overlong_word_length)
            return last_word_end == (gchar *)0 ? i : last_word_end;
    }
    return g_utf8_next_char(i) - 1;
}

void Window::setInsertMode(bool enable) {
	m_insertMode = enable;
}

void Window::callAsync(std::function<void()> aF) {
	events::emit("async" + id, aF);
}

void Window::handleAsync() {
	events::arg<std::function<void()>>(0)();
}

Window::~Window()
{
}

void Window::set_current() {

}

} // namespace display

