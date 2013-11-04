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

#ifndef _DISPLAYMANAGER_H_
#define _DISPLAYMANAGER_H_

#include <display/status_bar.h>
#include <display/window.h>
#include <display/input_window.h>
#include <utils/instance.h>
#include <utils/mutex.h>
#include <utils/lock.h>

#include <client/stdinc.h>
#include <client/DCPlusPlus.h>
#include <client/TimerManager.h>
#include <deque>

namespace display {

/** Helper typedef */
typedef std::deque<display::Window*> Windows;

class StatusBar;

class Manager:
    public utils::Instance<display::Manager>
{
public:
    Manager();

    Windows::iterator begin() { return m_windows->begin(); }
    Windows::iterator end() { return m_windows->end(); }

	/** Next/Prev. */
	void next();
	void prev();

    /** Redraw the screen. */
    void redraw();

    /** Handle key combos for changing window. */
    void handle_key();

    /** Remove the window and free memory used by it.
        @param window Window to remove */
    void remove(display::Window *window);

    /** Resize all windows if screen size is changed. */
    void resize();

    /** Insert a new window. */
    void push_back(display::Window *window);

    /** Get the active window
        @param current Iterator to the window which is currently active */
    Windows::iterator get_current() const { return m_current; }

    /** Get the active window
        @param current Pointer to the window which is currently active */
    Window* get_current_window() const { return *m_current; }

    /** Set the current active window
        @param current Iterator to the window */
    void set_current(Windows::iterator current);

    /** Set the current active window
        @param current Number of the window */
    void set_active_window(unsigned int n);

    /** Find window by its name.
        @return Iterator to window or end() if not found. */
	Windows::iterator find(display::Type aType, const std::string &aID = "");

    /** Passes the pressed key to active window. */
    void key_pressed();

    /** Destructor. Frees memory used by windows. */
    ~Manager();

	size_t size() const;
	//const std::string& getInputStr() const;
	void cmdMessage(const std::string& aLine);

	display::InputWindow m_inputWindow; //!< The input window
private:
	void set_current_impl(Windows::iterator newCur, Window* oldCur);
    /** Event handler for "window closed". Frees the memory used by the window. */
    void window_closed();

    Windows *m_windows; //!< List of the windows
    Windows::iterator m_current; //!< The window currently on the screen
    display::StatusBar *m_statusbar; //!< The status bar
    bool m_altPressed; //!< Used by key_pressed to check if alt was pressed
};

} // namespace display

#endif // _DISPLAYMANAGER_H_
