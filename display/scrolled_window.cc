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

#include <core/events.h>
#include <display/scrolled_window.h>
#include <input/manager.h>
#include <utils/utils.h>
#include <utils/strings.h>
#include <utils/lock.h>
#include <display/manager.h>
#include <client/stdinc.h>
#include <core/log.h>
#include <client/Util.h>

using namespace dcpp;

namespace display {

ScrolledWindow::ScrolledWindow(const std::string& aID, display::Type aType) :
    m_scrollPosition(-1),
    m_lastlogSize(200),
    m_timestamp("[%H:%M:%S] "),
	Window(aID, aType)
{
    m_bindings[KEY_PPAGE] = std::bind(&ScrolledWindow::scroll_window, this, -10);
    m_bindings[KEY_NPAGE] = std::bind(&ScrolledWindow::scroll_window, this, 10);

    update_config();
    m_lines.reserve(m_lastlogSize);
    m_state = STATE_NO_ACTIVITY;

}

void ScrolledWindow::clear() {
	m_lineLock.lock();
	m_lines.clear();
	m_scrollPosition = -1;
	m_lineLock.unlock();
}

void ScrolledWindow::update_config()
{
    m_lastlogSize = core::Settings::get()->find_int("lastlog_size", 200);
    m_timestamp = core::Settings::get()->find("timestamp_format", "[%H:%M:%S] ");
}

void ScrolledWindow::handle(wint_t key)
{
    if(m_bindings.find(key) != m_bindings.end()) {
        m_bindings[key]();
    }
    else if(key >= 0x20 && key < 0xFF) {
        m_input.key_insert(key);
        events::emit("window updated", static_cast<display::Window*>(this));
    }
    else {
        /* backspace, arrow keys.. */
        m_input.pressed(key);
        events::emit("window updated", static_cast<display::Window*>(this));
    }
}

void ScrolledWindow::add_line(const display::LineEntry &line_,
        bool redraw_screen /* = true */)
{
    m_lineLock.lock();
    if(m_lines.size() >= m_lastlogSize)
        m_lines.erase(m_lines.begin());

    display::LineEntry line = koskenkorva_viina(line_);
	m_lines.push_back(line);

    m_lineLock.unlock();

    if(redraw_screen && m_state == STATE_IS_ACTIVE) {
        events::emit("window updated", this);
    }
    else if(m_state != STATE_IS_ACTIVE) {
        if(line.m_type == LineEntry::HIGHLIGHT || line.m_type == LineEntry::ACTIVITY_ERROR) {
            m_state = STATE_HIGHLIGHT;
        }
        else if(line.m_type == LineEntry::MESSAGE && m_state != STATE_HIGHLIGHT) {
            m_state = STATE_MESSAGES;
        }
        else if(line.m_type == LineEntry::ACTIVITY && m_state != STATE_HIGHLIGHT
                && m_state != STATE_MESSAGES)
        {
            m_state = STATE_ACTIVITY;
        }
        events::emit("window status updated", static_cast<Window*>(this), m_state);
    }
}

LineEntry ScrolledWindow::koskenkorva_viina(const display::LineEntry &line_)
{
    auto line = line_;
    std::string text;
 
    text = utils::time_to_string(m_timestamp, line.m_time);
    line.m_indent += text.length();

	if (line.m_type == LineEntry::ACTIVITY_ERROR) {
		line.m_text = "%04" + line.m_text + "%04";
	}

	if (line.m_type == LineEntry::ACTIVITY || line.m_type == LineEntry::ACTIVITY_ERROR)
        text += "%21%01-%01!%01-%01%21 ";

    line.m_text = text + line.str();
    return line;
}

void ScrolledWindow::scroll_window(int aLines)
{
	auto window_width = static_cast<int>(get_width());
	auto window_height = static_cast<int>(get_height());

	/* Don't scroll if everything fits on the screen */
	int countedHeight = 0;
	int lastFirst = m_lines.size();
	for (const auto& line : m_lines | reversed) {
		auto h = Window::calculate_height(line.str(), window_width, line.m_indent);
		countedHeight += h;
		if (countedHeight > window_height) {
			break;
		}

		lastFirst--;
	}

	if (countedHeight <= window_height) {
		return;
	}

	countedHeight = 0;
	int oldFirst = m_scrollPosition >= 0 ? m_scrollPosition : lastFirst;
	// scale the lines based on the text height
	if (aLines > 0) {
		if (oldFirst < lastFirst) {
			auto lastIter = m_lines.begin() + lastFirst;
			for (auto p = m_lines.begin() + oldFirst; p < lastIter && countedHeight < aLines; ++p) {
				auto h = Window::calculate_height((*p).str(), window_width, (*p).m_indent);
				m_scrollPosition++;
				countedHeight += h;
			}
		}
	} else {
		if (m_scrollPosition == -1)
			m_scrollPosition = lastFirst;

		for (auto p = m_lines.begin() + oldFirst; p >= m_lines.begin() && countedHeight < abs(aLines); --p) {
			auto h = Window::calculate_height((*p).str(), window_width, (*p).m_indent);
			m_scrollPosition--;
			countedHeight += h;
		}

		if (m_scrollPosition < 0) {
			m_scrollPosition = 0;
		}
	}

	//core::Log::get()->log("scroll done, countedHeight:" + Util::toString(countedHeight) + " oldFirst: " + Util::toString(oldFirst) + " lastFirst: " + Util::toString(lastFirst));

	if (m_scrollPosition >= lastFirst) {
		m_scrollPosition = -1;
	}

    events::emit("window updated", this);
}

void ScrolledWindow::redraw()
{
    resize();
    auto window_height = static_cast<int>(get_height());
	auto window_width = static_cast<int>(get_width());

    /* Find out the first message to print */
	int countedHeight = 0;
	int first = m_lines.size();
	for (const auto& line : m_lines | reversed) {
		if (countedHeight == window_height) {
			first--;
			break;
		}

		auto h = Window::calculate_height(line.str(), window_width, line.m_indent);
		countedHeight += h;
		first--;
		if (countedHeight >= window_height) {
			break;
		}
	}

	//set_title("height" + Util::toString(countedHeight) + " first " + Util::toString(first) + " lines " + Util::toString(m_lines.size()) + " window " + Util::toString(window_height));
	if (m_scrollPosition == -1 || first == 0 || m_scrollPosition >= first) {
		m_scrollPosition = -1; // we have resized the window? all text can fit the screen or at least everything after the scroll pos
		countedHeight = window_height - countedHeight;
	} else {
		countedHeight = 0; // don't cut the first line
		first = m_scrollPosition;
	}

    /* Iterator to the first message to print */
    auto it = m_lines.begin()+first;

    Window::erase();

	std::string indentation;
    for(int height=0; it != m_lines.end() && height < window_height; ++it) {
        const auto& message = it->str();

        std::string::size_type i = 0;
        while(i < message.length()) {
            auto prevEnd = i;
            if(i == 0) {
                i = Window::find_line_end(message, i, window_width);
				indentation = "";
            } else {
                i = Window::find_line_end(message, i, window_width-it->m_indent);
                indentation = std::string(it->m_indent, ' ');
            }

            /* countedHeight is < 0 if the first message doesn't
             * fit completely on the screen */
			if (++countedHeight > 0) {
				print(indentation + message.substr(prevEnd, i - prevEnd + 1), 0, height++);
			}

			i++;
			for (; i < message.length() && isspace(message[i]); i++);
        }

        Window::clear_flags();
    }

    Window::refresh();
}

} // namespace display

