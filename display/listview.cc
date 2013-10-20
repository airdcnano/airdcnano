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

#include <limits> // std::numeric_limits
#include <numeric> // std::accumulate
#include <functional>
#include <display/listview.h>
#include <input/manager.h>
#include <utils/lock.h>
#include <core/log.h>
#include <utils/strings.h>
#include <utils/algorithm.h>
#include <core/events.h>

#include <client/stdinc.h>
#include <client/Exception.h>
#include <client/Text.h>
#include <client/Pointer.h>

using namespace dcpp;

namespace display {

ListView::ListView(display::Type aType, const std::string& aID, bool allowMove) :
    m_rowCount(0),
    m_currentItem(std::numeric_limits<int>::min()),
    m_infoboxHeight(4),
	Window(aID, aType, false)
{
    m_insertMode = false;
	m_prompt = "";
    m_bindings[KEY_UP] = std::bind(&ListView::scroll_list, this, -1);
    m_bindings[KEY_DOWN] = std::bind(&ListView::scroll_list, this, 1);
    m_bindings[KEY_PPAGE] = std::bind(&ListView::scroll_list, this, -20);
    m_bindings[KEY_NPAGE] = std::bind(&ListView::scroll_list, this, 20);
	if (allowMove)
		m_bindings['m'] = std::bind(&ListView::handleMove, this);

    resize();
}

void ListView::handleMove() {
	if (m_rowCount == 0)
		return;

	if (moving) {
		moving = false;
		set_prompt("");
	} else {
		moving = true;
		set_prompt("Move mode enabled. Press 'm' when you are ready.");
	}

	events::emit("window updated", this);
}

void ListView::setInsertMode(bool enable) {
	moving = false;
	m_insertMode = enable;
	m_input.clear_text();
}

void ListView::resize()
{
    Window::resize();

    if(m_columns.size() == 0)
        return;

    int window_width = get_width();
    int width = 0;

    std::for_each(m_columns.begin(), m_columns.end(), std::mem_fun(&Column::reset_width));

    using std::bind;
    using std::placeholders::_1;

    Columns::const_iterator it = m_columns.end();
    do {
        // total width of all columns
        width = std::accumulate(m_columns.begin(), m_columns.end(), 0, &Column::calc_width);

        for(unsigned int i=0; i<m_columns.size() && width < window_width; ++i) {
            Column *c = m_columns[i];
            if(c->get_real_width() < c->get_preferred_width()) {
                c->set_real_width(c->get_real_width()+1);
                width++;
            }
        }

        it = std::find_if(m_columns.begin(), m_columns.end(),
            bind(std::less<int>(),
                bind(&Column::get_real_width, _1),
                bind(&Column::get_preferred_width, _1)));
    } while(width < window_width && it != m_columns.end());

    do {
        width = std::accumulate(m_columns.begin(), m_columns.end(), 0, &Column::calc_width);

        for(unsigned int i=0; i<m_columns.size() && width < window_width; ++i) {
            Column *c = m_columns[i];
            if(c->get_real_width() < c->get_max_width()) {
                c->set_real_width(c->get_real_width()+1);
                width++;
            }
        }

        it = std::find_if(m_columns.begin(), m_columns.end(),
            bind(std::less<int>(),
                bind(&Column::get_real_width, _1),
                bind(&Column::get_max_width, _1)));
    } while(width < window_width && it != m_columns.end());
}

int ListView::insert_row()
{
    std::for_each(m_columns.begin(), m_columns.end(),
        std::mem_fun(&Column::insert_row));
    return m_rowCount++;
}

void ListView::delete_row(int column, const std::string &text)
{
    int row = m_columns.at(column)->find_row(text);
    if(row >= m_rowCount)
        return;

    std::for_each(m_columns.begin(), m_columns.end(),
        std::bind2nd(
            std::mem_fun(&Column::delete_row),
            row));

    if(row == m_rowCount-1) {
        m_currentItem--;}

    m_rowCount--;
}

void ListView::delete_all()
{
    std::for_each(m_columns.begin(), m_columns.end(),
        std::mem_fun(&Column::clear));
    m_rowCount = 0;
    events::emit("window updated", this);
}

void ListView::set_text(int column, int row, const std::string &text) {
	auto c = m_columns[column];
    c->set_text(row, text);
    events::emit("window updated", this);
}

void ListView::handle(wint_t key)
{
    if(m_insertMode) {
		if (key == KEY_ESCAPE) {
			setInsertMode(false);
			set_prompt();
			handleEscape();
		} else if (m_input.pressed(key)) {
			events::emit("window updated", static_cast<display::Window*>(this));
		} else if (key >= 0x20) {
			string tmp;
			Text::wcToUtf8(key, tmp);
			m_input.text_insert(tmp);
			events::emit("window updated", static_cast<display::Window*>(this));
		}
		return;
    }
    
    try {
        if(m_bindings.find(key) != m_bindings.end())
            m_bindings[key]();
    }
    catch(std::out_of_range &) {
    }
    catch(Exception &e) {
        core::Log::get()->log(e.getError());
    }
}

void ListView::scroll_list(int items)
{
	auto newPos = m_currentItem + items;
	if (moving && (newPos < 0 || newPos >= m_rowCount)) {
		return;
	}

	if (moving) {
		for (auto& c : m_columns) {
			utils::slide(c->m_rows, m_currentItem, newPos);
		}
		onListMove(m_currentItem, newPos);
	}

	m_currentItem = newPos;
    events::emit("window updated", this);
}

void ListView::redraw()
{
    if(m_currentItem >= static_cast<int>(m_rowCount) || m_currentItem == std::numeric_limits<int>::min())
        m_currentItem = 0;
    else if(m_currentItem < 0)
        m_currentItem = m_rowCount-1;

    erase();

    unsigned int y = 0;
    unsigned int x = 0;
    for(const auto& c: m_columns) {
        if(!c->is_hidden()) {
            print("%21" + c->get_name() + "%21", x, y);
            x += c->get_real_width();
        }
    }
    clear_flags();

    if(m_rowCount == 0) {
        refresh();
        return;
    }

    auto range = rak::advance_bidirectional<unsigned int>(0, m_currentItem, m_rowCount, get_height()-1-m_infoboxHeight);
    while(range.first != range.second) {
        x = 0;
        y++;

		for (const auto& c : m_columns) {
            if(c->is_hidden()) {
                continue;
            }

            const auto& text = c->get_text(range.first);
            auto width = c->get_real_width();

			print(static_cast<int>(strings::length(text)) >= width ? text.substr(0, width - 1) : text, x, y);
            x += width;

            if(m_currentItem == static_cast<int>(range.first))
                set_attr(0, y, get_width(), A_REVERSE, COLOR_PAIR(0));
        }
        range.first++;
    }

    for(unsigned int i=m_infoboxHeight; i>0; i--) {
        print(get_infobox_line(i), 0, get_height()-i);    
    }

    refresh();
}

ListView::~ListView()
{
    std::for_each(m_columns.begin(), m_columns.end(), DeleteFunction());
}

} // namespace display
