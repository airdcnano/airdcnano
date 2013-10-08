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

#ifndef _TEXTINPUT_H_
#define _TEXTINPUT_H_

#include <deque>
#include <map>
#include <string>
#include <stdexcept>
#include <functional>
#include <utils/instance.h>

namespace input {

const unsigned int MAX_HISTORY = 50;

typedef std::string String;

class TextInput:
    public std::string
{
public:
    TextInput();

    void update_config();

    void enter();
    void key_insert(wint_t key);
    void text_insert(const std::string &str);
    void pressed(int key);

    void set_pos(size_type pos) { m_pos = pos; }
    size_t get_pos() const;

    std::string& str() { return *this; }
    std::string str() const { return *this; }
    size_t length() const;
	void clear_text();
	void setText(const std::string& aText);
private:
	bool customTextSet = false;
    bool m_utf8;
    unsigned int m_pos;
    typedef std::deque<String> List;
    static List m_history;
    static List::const_iterator m_historyPos;
    std::map<wint_t, std::function<void ()> > m_bindings;
};

} // namespace input

#endif // _TEXTINPUT_H_

