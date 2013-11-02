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

#include <sstream>

#include <core/settings.h>
#include <core/events.h>
#include <core/log.h>

#include <display/manager.h>

#include <utils/ncurses.h>
#include <utils/utils.h>

#include <input/text_input.h>
#include <input/manager.h>

namespace input {

Manager::Manager():
    m_running(true)
{
    /* "in raw mode, the interrupt, quit, suspend, and flow control characters
     * are all passed  through  uninterpreted, instead of generating a signal"
     */
    raw();

    /* don't echo typed characters */
    noecho();

    /* TRUE = function keys are interpreted as single values eg. KEY_LEFT */
    keypad(stdscr, TRUE);

    /* XXX: this is not probably the best solution
     * make get_wch non-blocking so it doesn't block on shutdown.
     * otherwise there's no need for it to be non-blocing */
    halfdelay(5);
}

void Manager::main_loop()
{
    //core::Log::get()->log("input::Manager: " + utils::to_string(utils::gettid()));

	wint_t code = 0;
	int r;
	bool lastEsc = false;

    while(m_running)
    {
		while ((r = get_wch(&code)) != ERR) {
			// we use SIGWINCH, so KEY_RESIZE can be ignored
			if (r == KEY_CODE_YES && code == KEY_RESIZE)
				continue;
			lastEsc = code == 27;
			handle_char(code);
		}

		if (lastEsc) {
			handle_char(KEY_ESCAPE);
			lastEsc = false;
		}
    }
}

void Manager::handle_char(wint_t ch)
{
    char str[20];

    /* Control char */
    if(ch < 32) {
        str[0] = '^';
        str[1] = ch + '@';
        2[str] = 0;
    }
    else if(ch == 127) {
        str[0] = '^';
        str[1] = '?';
        2[str] = 0;
    }
    else {
        utils::utf16_char_to_utf8(ch, str)[str] = 0;
    }

    if(strcmp(str, "^") == 0) {
        str[1] = '^';
        str[2] = 0;
    }

    events::emit("key pressed", std::string(str), ch);
}

Manager::~Manager()
{
}

} // namespace input
