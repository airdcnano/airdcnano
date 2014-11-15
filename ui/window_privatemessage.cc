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
#include <client/File.h>
#include <client/LogManager.h>
#include <client/QueueManager.h>
#include <client/QueueItemBase.h>
#include <client/StringTokenizer.h>

namespace ui {

void WindowPrivateMessage::openWindow(const HintedUser& user) {
	auto n = ClientManager::getInstance()->getMyNick(user.hint);
	if (n.empty()) {
		display::Manager::get()->cmdMessage("Hub/user offline");
		return;
	}

	getWindow(user, n);
}

WindowPrivateMessage* WindowPrivateMessage::getWindow(const HintedUser& user, const std::string &mynick, bool setActive) {
	auto dm = display::Manager::get();
	auto it = dm->find(display::TYPE_PRIVMSG, user.user->getCID().toBase32());
	if (it == dm->end()) {
		auto pm = new ui::WindowPrivateMessage(user, mynick);
		dm->push_back(pm);
	}

	if (setActive)
		dm->set_current(it);

	return static_cast<WindowPrivateMessage*>(*it);
}

void WindowPrivateMessage::complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_, bool& appendSpace_) {

}

WindowPrivateMessage::WindowPrivateMessage(const HintedUser& user, const std::string &mynick) :
m_user(user), ScrolledWindow(user.user->getCID().toBase32(), display::TYPE_PRIVMSG)
{
    m_nick = mynick;
	updateTitles();
    m_state = display::STATE_NO_ACTIVITY;

    // ^B = get file list
    m_bindings[2] = std::bind(&WindowPrivateMessage::get_list, this);
	online = user.user->isOnline();
	ClientManager::getInstance()->addListener(this);

	readLog();
}

void WindowPrivateMessage::updateTitles() {
	auto nicks = ClientManager::getInstance()->getFormatedNicks(m_user);
	set_title("Conversation with " + nicks + " (" + ClientManager::getInstance()->getFormatedHubNames(m_user) + ")");
	set_name("PM:" + nicks);
}

void WindowPrivateMessage::handle_line(const std::string &line)
{
	string error;
	if (!ClientManager::getInstance()->privateMessage(m_user, line, error, false)) {
		add_line(display::LineEntry("Failed to send the message: " + error));
	}
}

void WindowPrivateMessage::get_list()
{
    try {
        QueueManager::getInstance()->addList(m_user, QueueItem::FLAG_CLIENT_VIEW);
    } catch(Exception &e) {
		add_line(display::LineEntry("Couldn't get file list from: " + ClientManager::getInstance()->getFormatedNicks(m_user) + " " + e.getError()));
    }
}

void WindowPrivateMessage::fillLogParams(ParamMap& params) const {
	const CID& cid = m_user.user->getCID();
	const string& hint = m_user.hint;
	params["hubNI"] = [&] { return Util::listToString(ClientManager::getInstance()->getHubNames(cid)); };
	params["hubURL"] = [&] { return Util::validateFileName(hint); };
	params["userCID"] = [&cid] { return cid.toBase32(); };
	params["userNI"] = [&] { return ClientManager::getInstance()->getNick(m_user.user, hint); };
	params["myCID"] = [] { return ClientManager::getInstance()->getMe()->getCID().toBase32(); };
}

void WindowPrivateMessage::readLog() {
	if (SETTING(SHOW_LAST_LINES_LOG) == 0) return;

	ParamMap params;
	fillLogParams(params);
	auto path = LogManager::getInstance()->getPath(m_user.user, params);

	try {
		File f(path, File::READ, File::OPEN);

		int64_t size = f.getSize();

		if (size > 32 * 1024) {
			f.setPos(size - 32 * 1024);
		}
		string buf = f.read(32 * 1024);
		StringList lines;

		if (Util::strnicmp(buf.c_str(), "\xef\xbb\xbf", 3) == 0)
			lines = StringTokenizer<string>(buf.substr(3), "\r\n").getTokens();
		else
			lines = StringTokenizer<string>(buf, "\r\n").getTokens();

		int linesCount = lines.size();

		int i = linesCount > (SETTING(SHOW_LAST_LINES_LOG) + 1) ? linesCount - SETTING(SHOW_LAST_LINES_LOG) : 0;

		for (; i < linesCount; ++i) {
			add_line(display::LineEntry(lines[i], 0, time(0), display::LineEntry::MESSAGE));
		}
		f.close();
	} catch (const FileException&) {
	}
}

WindowPrivateMessage::~WindowPrivateMessage()
{
	ClientManager::getInstance()->removeListener(this);
}

void WindowPrivateMessage::on(ClientManagerListener::UserConnected, const OnlineUser& aUser, bool) noexcept{
	if (aUser.getUser() == m_user.user) {
		callAsync([this] { onOnlineStateChanged(); });
	}
}

void WindowPrivateMessage::on(ClientManagerListener::UserDisconnected, const UserPtr& aUser, bool wentOffline) noexcept{
	if (aUser == m_user.user) {
		callAsync([this] { onOnlineStateChanged(); });
	}
}

void WindowPrivateMessage::onOnlineStateChanged() {
	auto hubs = ClientManager::getInstance()->getHubNames(m_user.user->getCID());
	if (hubs.empty() && online) {
		addStatusMessage("The user went offline");
		online = false;
	}

	if (!online && !hubs.empty()) {
		addStatusMessage("The user came online in " + hubs.front());
		online = true;
	}

	updateTitles();
	m_nick = ClientManager::getInstance()->getMyNick(m_user.hint);

	if (m_state == display::STATE_IS_ACTIVE) {
		events::emit("window changed");
		events::emit("window updated", this);
	}
}

void WindowPrivateMessage::addStatusMessage(const string& aMsg) {
	add_line(display::LineEntry(aMsg));
}

} // namespace ui
