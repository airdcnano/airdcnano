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
#include <client/TimerManager.h>

#include <boost/signals2.hpp>

#include <display/scrolled_window.h>
#include <utils/mutex.h>
#include <map>
#include <string>

using namespace dcpp;

namespace ui {

class WindowHub:
    public display::ScrolledWindow,
    public ClientListener,
    public TimerManagerListener
{
public:
    WindowHub(const std::string &address);

    virtual void update_config();

	void reconnect();
    static void openWindow(std::string address, ProfileToken shareProfile, bool activate);
    void handle_line(const std::string &line);
    Client* get_client() const { return m_client; }
    void print_names();

    /** @todo regular expressions in filters */
    bool filter_messages(const std::string &nick, const std::string &msg);
    std::string get_nick() const;
    const OnlineUser *get_user(const std::string &nick) throw (std::out_of_range);
    ~WindowHub();
	void connect() noexcept;

    void on(TimerManagerListener::Second, uint64_t) noexcept;

    // ClientListener stuff..
	//void on(ClientListener::Redirect, const Client*, const string&) noexcept;
	//void on(ClientListener::Failed, const string&, const string&) noexcept;
	//void on(ClientListener::GetPassword, const Client*) noexcept;
	//void on(ClientListener::HubUpdated, const Client*) noexcept;
	//void on(ClientListener::Message, const Client*, const ChatMessage&) noexcept;
	//void on(ClientListener::StatusMessage, const Client*, const string&, int = ClientListener::FLAG_NORMAL) noexcept;
	//void on(ClientListener::NickTaken, const Client*) noexcept;
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

	std::vector<std::string> complete(const std::string& aStr);
private:
	void handleFav() noexcept;
	void handleCreated() noexcept;
	void onChatMessage(const ChatMessage&) noexcept;
	void onPrivateMessage(const ChatMessage&) noexcept;

    Client *m_client;
    int64_t m_lastJoin;
    bool m_joined;
    bool m_timer;
    typedef std::map<std::string, const OnlineUser*> Users;
    typedef Users::const_iterator UserIter;
    Users m_users;
    UserIter m_currentUser;
    core::StringVector m_showNicks;
    core::StringVector m_ignoreNicks;
    core::StringVector m_highlights;
    bool m_showJoins;
    bool m_showJoinsOnThisHub;
    bool m_showNickList;
    bool m_resolveIps;
    bool m_utf8;
    std::string m_nmdcCharset;

    mutable utils::Mutex m_mutex;

	boost::signals2::connection reconnectConn;
	boost::signals2::connection createdConn;
	boost::signals2::connection favConn;
	boost::signals2::connection favoriteConn;
	boost::signals2::connection namesConn;
	boost::signals2::connection helpConn;

	void print_help();
	void handleNames();
};

} // namespace ui

#endif // _WINDOWHUB_H_
