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

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <glib.h>
#include <map>
#include <functional>
#include <display/curses_window.h>
#include <display/input_window.h>
#include <client/GetSet.h>
#include <core/events.h>

static const char *default_prompt = "[airdcnano]";

namespace display {

/** Something happened in the window? */
enum State {
    STATE_NO_ACTIVITY = 1,
    STATE_IS_ACTIVE,
    STATE_ACTIVITY,
    STATE_MESSAGES,
    STATE_HIGHLIGHT
};

/** Window types */
enum Type {
    TYPE_HUBWINDOW,
    TYPE_PRIVMSG,
    TYPE_LOGWND,
    TYPE_SEARCHWINDOW,
    TYPE_LISTBROWSER,
	TYPE_TRANSFERS,
	TYPE_HUBLIST,
	TYPE_FAVORITES,
    TYPE_UNKNOWN
};

/** Window. */
class Window:
    public display::CursesWindow
{
public:
    /** Constructor. */
    Window(const std::string& aID, Type aType, bool allowCommands);

    /** Draws the title window and calls \ref redraw. */
    void draw();

    /** Reimplement. */
    virtual void redraw() = 0;

    /** Resize the window. */
    virtual void resize();

    /** Called when a key is pressed. */
    virtual void handle(wint_t key) { }

    /** Called when enter is pressed. */ 
    virtual void handle_line(const std::string &line) { }

    /** Set the name of the window. */
    void set_name(std::string name) { m_name = name; }

    /** Get the name of the window.  */
    const std::string& get_name() const { return m_name; }

    /** Set window title. The title of the active window 
     * appears in top of the screen. */
    void set_title(std::string title) { m_title = title; }

    /** Get the title */
    std::string get_title() const { return m_title; }

    void set_state(State state) { m_state = state; }

    State get_state() const { return m_state; }

    Type get_type() const { return m_type; }

    virtual bool insert_mode() const { return m_insertMode; }

    std::string get_prompt() { return m_prompt; }

    void set_prompt() { m_prompt = default_prompt; }
    void set_prompt(const std::string &prompt) { m_prompt = prompt; }

    void set_draw_title(bool draw) { m_drawTitle = draw; }

    /** Destructor. */
    virtual ~Window();

    /* Thanks pelzi_ */
    /** Calculates how many strings word wrapping a string would return.
     * @param message String to be wrapped.
     * @param screen_width The width to wrap at.
     * @param indent Number of spaces to indent lines after the first.
     * @return The number of lines wrapping would return.
     */
    static unsigned int calculate_height(const std::string& message,
            unsigned int screen_width, unsigned int indent);

    /**
     * Word wraps lines.
     * @param message String containing line to be wrapped.
     * @param start Beginning of line.
     * @return End of line.
     */
    static std::string::size_type find_line_end(const std::string& message,
            std::string::size_type start, unsigned int screen_width);

    static input::TextInput m_input;
	const std::string& getID() const { return id; }

	void callAsync(std::function<void()> aF);
	void handleAsync();

	virtual void complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_) { }

	// does this window have an input line for commands
	const bool allowCommands;
protected:
	const std::string id;
    std::string m_prompt;
    std::string m_name;
    std::string m_title;
    display::CursesWindow m_titleWindow;
    const Type m_type;
    State m_state;
    std::map<wint_t, std::function<void ()> > m_bindings;
    bool m_drawTitle;
	virtual void setInsertMode(bool enable);
	bool getInsertMode() const { return m_insertMode; }
private:
	friend class ListView;
    /* find_line_end implementation. Subject to change, so clients should
     * use the public interface instead.
     */
    static const gchar *find_line_end(const gchar *line_start,
            const gchar *message_end, unsigned int screen_width);
    static unsigned int calculate_height(const gchar *message_start,
            const gchar *message_end, unsigned int screen_width,
            unsigned int indent);

	bool m_insertMode;

	boost::signals2::connection asyncConn;
};

} // namespace display

#endif // _WINDOW_H_
