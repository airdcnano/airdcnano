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

#ifndef _WINDOWHUB_H_
#define _WINDOWHUB_H_

#include <client/stdinc.h>

#include <client/ChatMessage.h>
#include <client/Client.h>
#include <client/ClientManager.h>
#include <client/OnlineUser.h>

#include <boost/signals2.hpp>

#include <display/scrolled_window.h>
#include <input/help_handler.h>

using namespace dcpp;

namespace ui {

class WindowHub:
    public display::ScrolledWindow,
    public ClientListener
{
private:
	unique_ptr<HelpHandler> help;
	HelpHandler::CommandList commands;

public:
    WindowHub(const std::string &address);

    virtual void update_config();

	void handleReconnect();
    static void openWindow(std::string address, ProfileToken shareProfile, bool activate);
    void handle_line(const std::string &line);
    Client* get_client() const { return m_client; }
    void print_names();

    /** @todo regular expressions in filters */
    bool filter_messages(const std::string &nick, const std::string &msg);
    std::string get_nick() const;
    const OnlineUserPtr get_user(const std::string &nick);
    ~WindowHub();

    // ClientListener stuff..
	void on(ClientListener::HubTopic, const Client*, const string&) noexcept;
	void on(ClientListener::AddLine, const Client*, const string&) noexcept;
	void on(ClientListener::SetActive, const Client*) noexcept{}

	void on(ClientListener::UserConnected, const Client*, const OnlineUserPtr&) noexcept;
	void on(ClientListener::UserUpdated, const Client*, const OnlineUserPtr&) noexcept;
	void on(ClientListener::UsersUpdated, const Client*, const OnlineUserList&) noexcept;
	void on(ClientListener::UserRemoved, const Client*, const OnlineUserPtr&) noexcept;
	void on(ClientListener::Message, const Client*, const ChatMessage&) noexcept;
	void on(ClientListener::GetPassword, const Client*) noexcept;

	void on(ClientListener::StatusMessage, const Client*, const string& line, int = ClientListener::FLAG_NORMAL) noexcept;
	void on(ClientListener::Connecting, const Client*) noexcept;
	void on(ClientListener::Connected, const Client*) noexcept;
	void on(ClientListener::Redirect, const Client*, const string &msg) noexcept;
	void on(ClientListener::Failed, const string& url, const string& msg) noexcept;
	void on(ClientListener::HubUpdated, const Client*) noexcept;
	void on(ClientListener::HubFull, const Client*) noexcept{ add_line(display::LineEntry("Hub full")); }
	void on(ClientListener::NickTaken, const Client*) noexcept{ add_line(display::LineEntry("Nick taken")); }
	void on(ClientListener::SearchFlood, const Client*, const string &msg) noexcept{ add_line(display::LineEntry(msg)); }

	void complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_, bool& appendSpace_);
	void handleCreated() noexcept;
private:
	void onJoinedTimer();
	void handleFav() noexcept;
	void onChatMessage(const ChatMessage&) noexcept;
	void onPrivateMessage(const ChatMessage&) noexcept;

	void handleShowJoins();
	void handleMsg();
	void handleBrowse();
    void handleInfo();

    OnlineUserPtr getUserFromParam();

	void print_help();
	void handleNames();
	void updateTitle();

	void handleUserUpdated(const OnlineUserPtr& aUser);
	void handleUserRemoved(const OnlineUserPtr& aUser);
	void handlePassword();
	void handleConnected();
	void handleFailed(const std::string& aMsg);

	typedef std::unordered_map<std::string, const OnlineUserPtr> Users;
	Users m_users;
	core::StringVector m_showNicks;
	core::StringVector m_ignoreNicks;
	core::StringVector m_highlights;
	core::StringVector m_blockedWords;

	boost::signals2::scoped_connection passwordConn;
	bool m_joined = false;
	bool m_showNickList = false;
	bool m_resolveIps = false;
	Client *m_client = nullptr;
};

} // namespace ui

#endif // _WINDOWHUB_H_
