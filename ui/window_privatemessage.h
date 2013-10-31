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

#ifndef _WINDOWPRIVATEMESSAGE_H_
#define _WINDOWPRIVATEMESSAGE_H_

#include <client/stdinc.h>
#include <client/forward.h>
#include <client/ClientManagerListener.h>
#include <client/HintedUser.h>

#include <display/scrolled_window.h>

using namespace dcpp;

namespace ui {

class WindowPrivateMessage:
    public display::ScrolledWindow,
	private ClientManagerListener
{
public:
	static void openWindow(const HintedUser& user);
	static WindowPrivateMessage* getWindow(const HintedUser& user, const std::string &mynick, bool setActive = true);
    WindowPrivateMessage(const HintedUser& user, const std::string &mynick);

    /** Send private message to the user */
    virtual void handle_line(const std::string &line);

    /** Get my nick. */
    std::string get_nick() const { return m_nick; }

    /** Get user's file list. */
    void get_list();

    HintedUser get_user() { return m_user; }

    ~WindowPrivateMessage();

	void fillLogParams(ParamMap& params) const;
	void complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_, bool& appendSpace_);
private:
    HintedUser m_user;
    std::string m_nick;

	// ClientManagerListener
	void on(ClientManagerListener::UserConnected, const OnlineUser& aUser, bool wasOffline) noexcept;
	void on(ClientManagerListener::UserDisconnected, const UserPtr& aUser, bool wentOffline) noexcept;

	void onOnlineStateChanged();
	bool online = true;
	void addStatusMessage(const string& aMsg);
	void updateTitles();
	void readLog();
};

} // namespace ui

#endif // _WINDOWPRIVATEMESSAGE_H_
