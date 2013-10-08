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

#include <glib.h>
#include <ncursesw/ncurses.h>
#include <core/settings.h>
#include <core/log.h>
#include <utils/utils.h>
#include <functional>
#include <input/text_input.h>

namespace input {

TextInput::List TextInput::m_history;
TextInput::List::const_iterator TextInput::m_historyPos = m_history.begin();

TextInput::TextInput():
    m_utf8(false),
    m_pos(0)
{
    m_bindings[KEY_HOME] = std::bind(&TextInput::set_pos, this, 0);
    m_bindings[KEY_END] = std::bind(&TextInput::set_pos, this,
                                std::bind(&TextInput::length, this));
}

void TextInput::update_config()
{
    m_utf8 = core::Settings::get()->find_bool("utf8_input", false);
}

size_t TextInput::length() const
{
    return g_utf8_strlen(c_str(), -1);
}

void TextInput::clear_text() {
	clear();
	m_pos = 0;
}

size_t TextInput::get_pos() const
{
    return g_utf8_pointer_to_offset(c_str(), c_str()+m_pos);
}

void TextInput::enter()
{
    if(m_history.size() > MAX_HISTORY)
        m_history.pop_back();

    if(!this->empty())
        m_history.push_front(*this);
    m_historyPos = m_history.begin();

	clear_text();
}

void TextInput::text_insert(const std::string &ch)
{
    try {
        if(!g_utf8_validate(ch.c_str(), -1, 0))
            throw std::runtime_error("g_utf8_validate failed");

        String::insert(m_pos, ch);
        m_pos += ch.length();
    } catch(std::exception &e) {
        core::Log::get()->log(std::string("Bug in TextInput::text_insert? ") + e.what());
        return;
    }
}

void TextInput::key_insert(wint_t key)
{
    char str[20];
    utils::utf16_char_to_utf8(key, str)[str] = 0;
    text_insert(std::string(str));
}

void TextInput::setText(const std::string& aText, bool aNoClear /*true*/) {
	assign(aText);
	set_pos(aText.length());
	noClear = aNoClear;
}

bool TextInput::hasBinding(wint_t key) const {
	return m_bindings.find(key) != m_bindings.end(); 
}

void TextInput::pressed(int key) 
{
    if(hasBinding(key)) {
        m_bindings[key]();
        return;
    }
    switch (key) {
        // ^W
        case 0x17:
        {
            break;
        }
        case 0x7F:
        case 0x08:
        case KEY_BACKSPACE:
            if (m_pos != 0) {
                int prev = g_utf8_prev_char(c_str()+m_pos)-c_str();
                erase(prev, m_pos-prev);
                m_pos = prev;
            }
            break;
        case KEY_DC:
            if (m_pos != size()) {
                int next = g_utf8_find_next_char(c_str()+m_pos, 0)-c_str();
                erase(m_pos, (next-m_pos));
            }
            break;
        case KEY_LEFT:
            if (m_pos != 0) {
                m_pos = g_utf8_prev_char(c_str()+m_pos)-c_str();
            }
            break;
        case KEY_RIGHT:
            if (m_pos != size()) {
                m_pos = g_utf8_find_next_char(c_str()+m_pos, 0)-c_str();
            }
            break;
        case KEY_UP:
            if(m_history.size() > 0) {
                if((m_historyPos == m_history.begin()) && (this->empty())) {
                    assign(*m_historyPos);
                }
                else if(m_historyPos != m_history.end()-1) {
                    assign(*(++m_historyPos));
                }
                else {
                    m_historyPos = m_history.begin();
                    clear();
                }
                m_pos = size();
            }
            break;
        case KEY_DOWN:
            if(m_historyPos != m_history.begin()) {
                assign(*(--m_historyPos));
            }
            else {
                // save the written line even if enter 
                // is not pressed (just like in Irssi)
                if(m_history.empty() || (!empty() && m_history.at(0) != str())) {
                    m_history.push_front(*this);
                    m_historyPos = m_history.begin();
                }
                clear();
            }
            m_pos = length();
            break;
        case 0x0A:
			if (!noClear)
				enter();
			else
				noClear = false;
            break;
        default:
            return;
    }
}

} // namespace input
