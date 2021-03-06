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

#include <iostream>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <functional>
#include <display/screen.h>
#include <display/manager.h>
#include <utils/utils.h>
#include <core/log.h>
#include <core/events.h>

#include <ui/window_hub.h>
#include <ui/window_privatemessage.h>

namespace display {

Manager::Manager():
    m_windows(new Windows()),
    m_current(m_windows->end()),
    m_statusbar(display::StatusBar::create()),
    m_altPressed(false)
{
    events::add_listener("key pressed",
            std::bind(&display::Window::handle,
                std::bind(&display::Manager::get_current_window, this),
                std::bind(&events::arg<wint_t>, 1)));

    events::add_listener_first("key pressed",
            std::bind(&display::Manager::handle_key, this));

    events::add_listener_last("window closed",
            std::bind(&display::Manager::window_closed, this));

    m_inputWindow.set_input(&display::Window::m_input);
}

void Manager::push_back(display::Window *window)
{
    m_windows->push_back(window);
	if (m_windows->size() == 1) {
		m_current = m_windows->begin();
	}

    window->resize();
}

void Manager::set_active_window(unsigned int n) {
	if (n >= m_windows->size())
		return;
    set_current(m_windows->begin()+n);
}

size_t Manager::size() const {
	return m_windows->size(); 
}

void Manager::set_current(Windows::iterator current)
{
	if (current == m_current) {
		return;
	}

	set_current_impl(current, *m_current);
}

void Manager::set_current_impl(Windows::iterator newCur, Window* oldCur) {
	(*m_current)->set_state(STATE_NO_ACTIVITY);
	(*m_current)->erase();
	m_current = newCur;
	(*m_current)->set_state(STATE_IS_ACTIVE);
	(*m_current)->refresh();
	(*m_current)->redraw();

	events::emit("window status updated", *newCur, STATE_IS_ACTIVE);
	events::emit("window updated", *newCur);
	events::emit("window changed", oldCur, *newCur);
}

void Manager::handle_key()
{
    wint_t key = events::arg<wint_t>(1);
    if(key == 0x1B) {
        m_altPressed = true;
        events::stop();
        return;
    }

    if(m_altPressed == false)
        return;

    m_altPressed = false;
    if(key >= '1' && key <= '9') {
		set_active_window(key - '1');
        events::stop();
    } else if(key == KEY_LEFT) {
		prev();
    } else if(key == KEY_RIGHT) {
		next();
    }
}

void Manager::redraw()
{
    if(display::Screen::is_resized()) {
        resize();
    }

    (*m_current)->draw();
    m_statusbar->redraw();

	if ((*m_current)->draw_prompt()) {
		m_inputWindow.set_prompt((*m_current)->get_prompt());
		m_inputWindow.redraw();
	} else {
		m_inputWindow.erase();
		m_inputWindow.refresh();
	}

    if((*m_current)->insert_mode()) {
        curs_set(1);
    } else {
        curs_set(0);
    }
    display::Screen::do_update();
}

void Manager::resize()
{
    int h = display::Screen::get_height();
    int w = display::Screen::get_width();
    m_statusbar->resize(0, h-2 < 1 ? 1 : h-2, w, 1);
    m_inputWindow.resize(0, h-1 < 1 ? 1 : h-1, w, 1);

    std::for_each(m_windows->begin(), m_windows->end(),
            std::mem_fun(&Window::resize));
}

void Manager::cmdMessage(const std::string& aLine) {
	auto msg = utils::escape(aLine);
	auto it = get_current();
	if ((*it)->get_type() == display::TYPE_HUBWINDOW) {
		static_cast<ui::WindowHub*>(*it)->add_line(display::LineEntry(msg));
	} else if ((*it)->get_type() == display::TYPE_PRIVMSG) {
		static_cast<ui::WindowPrivateMessage*>(*it)->add_line(display::LineEntry(msg));
	} else {
		core::Log::get()->log(msg);
	}
}

void Manager::remove(display::Window *window)
{
    /* it would crash if all windows were closed */
    if(m_windows->at(0) == window) {
        //core::Log::get()->log("Don't close log window", core::MT_DEBUG);
        return;
    }

	auto cur = *m_current;
	auto newWin = m_windows->erase(std::find(m_windows->begin(), m_windows->end(), window));

	if (cur == window) {
		set_current_impl(newWin == m_windows->end() ? m_windows->end()-1 : newWin, window);
	}

	events::emit("window closed", window);
}

void Manager::next() {
	auto newCur = m_current == m_windows->end() - 1 ? m_windows->begin() : m_current + 1;
	set_current(newCur);
}

void Manager::prev() {
	auto newCur = m_current == m_windows->begin() ? m_windows->end() - 1 : m_current - 1;
	set_current(newCur);
}

Windows::iterator Manager::find(display::Type aType, const std::string& aID)
{
	return std::find_if(m_windows->begin(), m_windows->end(), [&](const display::Window* aWindow) { return (aID.empty() || aWindow->getID() == aID) && aWindow->get_type() == aType; });
}

void Manager::window_closed()
{
    display::Window *window = events::arg<display::Window*>(0);
    delete window;
}

Manager::~Manager()
{
    std::for_each(m_windows->begin(), m_windows->end(),
            utils::delete_functor<Window>());
    m_windows->clear();

    display::InputWindow::destroy();
    display::StatusBar::destroy();
}

} // namespace display

