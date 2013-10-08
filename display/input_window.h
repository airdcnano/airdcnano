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

#ifndef _INPUTWINDOW_H_
#define _INPUTWINDOW_H_

#include <string>
#include <display/curses_window.h>
#include <input/text_input.h>
#include <utils/instance.h>

namespace display {

class InputWindow:
    public utils::Instance<display::InputWindow>,
    public display::CursesWindow
{
public:
    InputWindow();
    void set_input(input::TextInput *input) { m_input = input; }
    void set_prompt(const std::string &prompt) { m_prompt = prompt; }
    void redraw();

	const std::string& getInputStr() const;
	void setInputStr(const std::string& aText);
private:
    std::string m_prompt;
    input::TextInput *m_input;
};

} // namespace display

#endif // _INPUTWINDOW_H_

