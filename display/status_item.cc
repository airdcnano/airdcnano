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

#include <display/status_item.h>
#include <display/status_bar.h>
#include <utils/lock.h>

#include <client/Util.h>

namespace display {

const std::string& StatusItem::get_text() const {
	return m_text;
}

const std::string& StatusItem::get_name() const {
	return m_id;
}

void StatusItem::callAsync(std::function<void()> aF) {
	events::emit("asyncbar" + m_id, aF);
}

void StatusItem::handleAsync() {
	events::arg<std::function<void()>>(0)();
}

StatusItem::StatusItem(const std::string& aID) : m_id(aID),
asyncConn(events::add_listener("asyncbar" + aID, std::bind(&StatusItem::handleAsync, this)))
{

}


ProgressUpdater::ProgressUpdater(const std::string& aID) : StatusItem(aID) {

}

void ProgressUpdater::updateStatus(double percent) {
	if (percent == 1 && visible) {
		display::StatusBar::get()->remove_item(get_name());
		visible = false;
		return;
	} else if (percent > 0 && !visible) {
		visible = true;
		display::StatusBar::get()->add_item(this, 1);
	}

	/*std::ostringstream oss;
	oss << std::setiosflags(ios::fixed)
	<< std::setprecision(1)
	<< percent * 100 << "%%";*/

	m_text = dcpp::Util::toString(percent);
}

ProgressUpdater::~ProgressUpdater() {
	if (visible)
		display::StatusBar::get()->remove_item(get_name());
}

} // namespace display
