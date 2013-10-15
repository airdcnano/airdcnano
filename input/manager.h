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

#ifndef _INPUTMANAGER_H_
#define _INPUTMANAGER_H_

#include <map>
#include <string>
#include <ncursesw/ncurses.h>
#include <input/text_input.h>
#include <input/completion.h>
#include <utils/instance.h>

#define KEY_ESCAPE (KEY_MAX+1)

namespace input {

/* Reads input from stdin. After reading the character, it
 * is translated to UTF-8 string and event "key pressed"
 * is emitted with first parameter being UTF-8 std::string
 * and the second being the original UTF-16 wint_t. */
class Manager: 
    public utils::Instance<input::Manager>
{
public:
    /* Constructor. */
    Manager();

    /* Main loop. Read characters until 
     * get_wch returns ERR or quit() is called. */
    void main_loop();

    /* Stop input loop. */
    void quit() { m_running = false; }

    /** Destructor. */
    ~Manager();
private:
    bool m_running;

    /** Helper function. Translates ch to UTF-8
     * and emits the event. */
    void handle_char(wint_t ch);
};

}

#endif // _INPUTMANAGER_H_
