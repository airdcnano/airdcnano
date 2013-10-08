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

#include <functional>
#include <core/log.h>
#include <display/manager.h>
#include <ui/window_privatemessage.h>

#include <client/ClientManager.h>
#include <client/QueueManager.h>
#include <client/QueueItemBase.h>

namespace ui {

WindowPrivateMessage::WindowPrivateMessage(const HintedUser& user, const std::string &mynick) :
m_user(user), ScrolledWindow(user.user->getCID().toBase32(), display::TYPE_PRIVMSG)
{
    m_nick = mynick;
	set_title("Conversation with " + Util::listToString(ClientManager::getInstance()->getNicks(user)));
    m_state = display::STATE_NO_ACTIVITY;

	set_name("PM:" + Util::listToString(ClientManager::getInstance()->getNicks(user)));

    // ^B = get file list
    m_bindings[2] = std::bind(&WindowPrivateMessage::get_list, this);
}

void WindowPrivateMessage::handle_line(const std::string &line)
{
	string error;
	if (!ClientManager::getInstance()->privateMessage(m_user, line, error, false)) {

	}
}

void WindowPrivateMessage::get_list()
{
    try {
        QueueManager::getInstance()->addList(m_user, QueueItem::FLAG_CLIENT_VIEW);
    } catch(Exception &e) {
		core::Log::get()->log("Couldn't get file list from " + Util::listToString(ClientManager::getInstance()->getNicks(m_user)) + " " + e.getError());
    }
}

WindowPrivateMessage::~WindowPrivateMessage()
{

}

} // namespace ui
