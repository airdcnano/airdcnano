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

#include <stdexcept>
#include <sstream>
#include <functional>
#include <core/settings.h>
#include <core/log.h>
#include <utils/utils.h>
#include <utils/lock.h>
#include <display/status_bar.h>
#include <display/screen.h>

namespace display {

StatusBar::StatusBar():
    CursesWindow(0, Screen::get_height()-2, Screen::get_width(), 1)
{
    update_config();
    core::Settings::get()->add_listener(
        std::bind(&StatusBar::update_config, this));
}

void StatusBar::add_item(display::StatusItem *item, int pos)
    throw(std::runtime_error)
{
    utils::Lock l(m_mutex);
    StatusItems::iterator it = std::find_if(m_items.begin(), m_items.end(),
        std::bind(
            std::bind2nd(std::equal_to<std::string>(), item->get_name()),
            std::bind(&StatusItem::get_name, std::placeholders::_1)
        ));

    if(it != m_items.end())
        throw std::runtime_error("Statusbar already has a item named "
                + item->get_name());

    if(pos == -1 || pos > static_cast<int>(m_items.size()))
        m_items.push_back(item);
    else
        m_items.insert(m_items.begin()+pos, item);
}

StatusItem* StatusBar::remove_item(const std::string &name)
    throw(std::runtime_error)
{
    utils::Lock l(m_mutex);
	auto it = std::find_if(m_items.begin(), m_items.end(), [&](const StatusItem* si) { return si->get_name() == name; });

    if(it == m_items.end())
        throw std::runtime_error("Unknown item: " + name);

    m_items.erase(it);
    return *it;
}

void StatusBar::free_item(const std::string &name)
    throw(std::runtime_error)
{
    delete remove_item(name);
}

void StatusBar::update_item(const std::string &name)
    throw(std::runtime_error)
{
    utils::Lock l(m_mutex);

    StatusItems::iterator it = std::find_if(m_items.begin(), m_items.end(),
        std::bind(
            std::bind2nd(std::equal_to<std::string>(), name),
            std::bind(&StatusItem::get_name, std::placeholders::_1)
        ));

    if(it == m_items.end())
        throw std::runtime_error("Unknown item: " + name);

    (*it)->update();
    redraw();
}

void StatusBar::redraw()
{
    utils::Lock l(m_mutex);
    erase();
    set_background(10);

    // [item] [another item] [third item]...
    std::ostringstream oss;
    for(unsigned int i=0; i<m_items.size(); ++i) {
        oss << "%11[%11%15"
            << m_items.at(i)->get_text() << "%15%11]%11 ";
    }

    print(oss.str(), 0, 0);
    refresh();
}

void StatusBar::update_config()
{
}

StatusBar::~StatusBar()
{
    std::for_each(m_items.begin(), m_items.end(),
            utils::delete_functor<StatusItem>());
}

} // namespace display
