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

#include <client/ChatMessage.h>
#include <client/HintedUser.h>
//#include <client/PrivateChat.h>
#include <client/MessageManager.h>
#include <client/PrivateChatListener.h>
//#include <client/PrivateChat.h>

#include <input/help_handler.h>
#include <display/scrolled_window.h>

using namespace dcpp;

namespace ui {

class WindowPrivateMessage:
    public display::ScrolledWindow,
    private PrivateChatListener
{
public:

    static void onPrivateMessage(const ChatMessage& aMessage) noexcept;
	static void openWindow(const HintedUser& user);
	static WindowPrivateMessage* getWindow(const HintedUser& user, bool setActive = true);
    WindowPrivateMessage(const HintedUser& user);

    /** Send private message to the user */
    virtual void handle_line(const std::string &line);

    /** Get user's file list. */
    void get_list();

    ~WindowPrivateMessage();

	void complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_, bool& appendSpace_);
    std::string get_my_nick() const;
private:
	virtual void on(PrivateChatListener::StatusMessage, const string& aMessage, uint8_t sev) noexcept;
	virtual void on(PrivateChatListener::PrivateMessage, const ChatMessage& aMessage) noexcept;
	virtual void on(PrivateChatListener::Activate, const string& msg, Client* c) noexcept;
	virtual void on(PrivateChatListener::UserUpdated) noexcept;
	virtual void on(PrivateChatListener::PMStatus, uint8_t aType) noexcept;
	//virtual void on(PrivateChatListener::CCPMStatusChanged, const string& aMessage) noexcept;
	virtual void on(PrivateChatListener::Close) noexcept;

	void onOnlineStateChanged();

    void addMessage(const ChatMessage& aMessage);
    void addStatusMessage(const string& aMsg);
	void updateTitles();
	void readLog();
    void handleEncrypt();

    PrivateChat* chat;

    unique_ptr<HelpHandler> help;
    HelpHandler::CommandList commands;
};

} // namespace ui

#endif // _WINDOWPRIVATEMESSAGE_H_
