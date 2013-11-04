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

#include <iomanip>
#include <ui/status_hash.h>

namespace ui {

StatusHash::StatusHash():
m_startBytes(0), StatusItem("hash")
{
	m_visible = false;
    TimerManager::getInstance()->addListener(this);
}

void StatusHash::update()
{
}

void StatusHash::on(Second, uint64_t)
    noexcept
{
    std::string file;
    int64_t bytes = 0;
    size_t files = 0;
	int64_t speed = 0;
	int hashers = 0;

	HashManager::getInstance()->getStats(file, bytes, files, speed, hashers);
	if (bytes == 0) {
		// nothing to hash
		if (m_visible) {
			m_visible = false;
			callAsync([=] { display::StatusBar::get()->remove_item(get_name()); });
		}

		m_startBytes = 0;
		return;
	} else if (!m_visible) {
		// just started
		m_startBytes = bytes;
		m_visible = true;
		callAsync([=] { display::StatusBar::get()->add_item(this, 1); });
	}

	// update the progress
	callAsync([=] {
		double percent = static_cast<double>(m_startBytes - bytes) / static_cast<double>(m_startBytes);

		std::ostringstream oss;
		oss << std::setiosflags(ios::fixed)
			<< std::setprecision(1)
			<< percent * 100 << "%%";

		m_text = oss.str();
	});
}

StatusHash::~StatusHash()
{
    TimerManager::getInstance()->removeListener(this);
}

} // namespace ui
