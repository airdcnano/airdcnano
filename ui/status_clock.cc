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

#include <core/events.h>
#include <core/settings.h>
#include <ui/status_clock.h>
#include <utils/utils.h>

#include <client/Util.h>

namespace ui {

StatusClock::StatusClock() : StatusItem("clock")
{
    update_config();
    core::Settings::get()->add_listener(
            std::bind(&StatusClock::update_config, this));

	events::add_listener("timer started", [this] { TimerManager::getInstance()->addListener(this); });
	//events::add_listener("command quit", [this] { TimerManager::getInstance()->removeListener(this); });
    update();
}

void StatusClock::on(TimerManagerListener::Second, uint64_t)
    noexcept
{
    update();
    events::emit("statusbar updated");
}

void StatusClock::update()
{
	callAsync([this] { m_text = dcpp::Util::formatTime(m_timeformat, GET_TIME()); });
}

void StatusClock::update_config()
{
	m_timeformat = core::Settings::get()->find_str("clock_format", "%H:%M:%S");
}

StatusClock::~StatusClock()
{
	TimerManager::getInstance()->removeListener(this);
}

} // namespace ui
