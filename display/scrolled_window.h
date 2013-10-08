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

#ifndef _SCROLLEDWINDOW_H_
#define _SCROLLEDWINDOW_H_

#include <string>
#include <vector>
#include <core/settings.h>
#include <display/window.h>
#include <utils/mutex.h>

#include <client/stdinc.h>
#include <client/TimerManager.h>

using namespace dcpp;

namespace display {

class LineEntry {
public:
    enum Type {
        ACTIVITY = 10001,
		ACTIVITY_ERROR,
        MESSAGE,
        HIGHLIGHT
    };

    LineEntry(const std::string &text, unsigned int indent = 0, time_t time_ = -1, Type type = ACTIVITY):
        m_text(text),
        m_indent(indent),
        m_time(time_),
        m_type(type)
    {
        if(m_time == -1)
            m_time = GET_TIME();
    }

    LineEntry() { }

    const std::string& str() { return m_text; }
private:
    std::string m_text;
    unsigned int m_indent;
    time_t m_time;
    Type m_type;
    friend class ScrolledWindow;
};

class ScrolledWindow:
    public display::Window
{
public:
    ScrolledWindow(const std::string& aID, display::Type aType);

    virtual void update_config();
    virtual void redraw();
    virtual void handle(wint_t key);

    /** Insert a new line.
     * @param line The line to add 
     * @param redraw Whether to redraw the screen immediately after adding the line */
    void add_line(const display::LineEntry &line, bool redraw = true);

    /** Scroll the window. */
    void scroll_window(int lines);
private:
    void set_activity(display::LineEntry::Type type);
    display::LineEntry koskenkorva_viina(const display::LineEntry &line);

    utils::Mutex m_messageLock;
    utils::Mutex m_lineLock;
    std::vector<display::LineEntry> m_lines;
    unsigned int m_scrollPosition; //!< Number of the first line to show on the screen
    unsigned int m_lastlogSize;
    std::string m_timestamp;
};

} // namespace display

#endif // _SCROLLEDWINDOW_H_

