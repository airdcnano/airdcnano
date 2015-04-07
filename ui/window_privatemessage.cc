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
#include <utils/utils.h>

#include <client/ClientManager.h>
#include <client/File.h>
#include <client/LogManager.h>
#include <client/MessageManager.h>
#include <client/QueueManager.h>
#include <client/QueueItemBase.h>
#include <client/StringTokenizer.h>

namespace ui {


void WindowPrivateMessage::onPrivateMessage(const ChatMessage& aMessage) noexcept{
    auto dm = display::Manager::get();
    ui::WindowPrivateMessage *pm;

    bool myPM = aMessage.replyTo->getUser() == ClientManager::getInstance()->getMe();
    const auto& user = myPM ? aMessage.to : aMessage.replyTo;

    auto it = dm->find(display::TYPE_PRIVMSG, user->getUser()->getCID().toBase32());
    if (it == dm->end()) {
        pm = new ui::WindowPrivateMessage(HintedUser(user->getUser(), user->getHubUrl()));
        dm->push_back(pm);

        if (AirUtil::getAway()) {
            if (!(SETTING(NO_AWAYMSG_TO_BOTS) && user->getUser()->isSet(User::BOT))) {
                ParamMap params;
                user->getIdentity().getParams(params, "user", false);

                string error;
                auto awayMsg = AirUtil::getAwayMessage(SETTING(DEFAULT_AWAY_MESSAGE), params);
                pm->handle_line(awayMsg);
            }
        }
    } else {
        pm = static_cast<ui::WindowPrivateMessage*>(*it);
    }

    pm->addMessage(aMessage);
}

void WindowPrivateMessage::addMessage(const ChatMessage& aMessage) {
    chat->logMessage(aMessage.format());

    bool myPM = aMessage.replyTo->getUser() == ClientManager::getInstance()->getMe();

    auto nick = aMessage.from->getIdentity().getNick();
    auto text = utils::escape(aMessage.text);


    StringTokenizer<string> lines(text, '\n');
    auto displaySender = myPM ? "%21%08<%21%08%21" + nick + "%21%21%08>%21%08" : "%21%08<%21%08" + nick + "%21%08>%21%08";

    int indent = 4 + g_utf8_strlen(nick.c_str(), -1);
    for (const auto& l : lines.getTokens()) {
        add_line(display::LineEntry(displaySender + " " + l, indent, time(0), display::LineEntry::MESSAGE));
    }

    if (!myPM && get_state() != display::STATE_IS_ACTIVE) {
        set_state(display::STATE_HIGHLIGHT);
    }

}

WindowPrivateMessage* WindowPrivateMessage::getWindow(const HintedUser& user, bool setActive) {
	auto dm = display::Manager::get();
	auto it = dm->find(display::TYPE_PRIVMSG, user.user->getCID().toBase32());
	if (it == dm->end()) {
		auto pm = new ui::WindowPrivateMessage(user);
		dm->push_back(pm);
	}

	if (setActive)
		dm->set_current(it);

	return static_cast<WindowPrivateMessage*>(*it);
}

void WindowPrivateMessage::complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_, bool& appendSpace_) {

}

WindowPrivateMessage::WindowPrivateMessage(const HintedUser& user) :
    ScrolledWindow(user.user->getCID().toBase32(), display::TYPE_PRIVMSG), 
    commands({
                      { "encrypt", boost::bind(&WindowPrivateMessage::handleEncrypt, this), nullptr } 
    })
{
    help.reset(new HelpHandler(&commands, "PM-specific", this));
    chat = MessageManager::getInstance()->addChat(user); 
	updateTitles();
    m_state = display::STATE_NO_ACTIVITY;

    // ^B = get file list
    m_bindings[2] = std::bind(&WindowPrivateMessage::get_list, this);
    chat->addListener(this);

	readLog();
}

void WindowPrivateMessage::updateTitles() {
	auto nicks = ClientManager::getInstance()->getFormatedNicks(chat->getHintedUser());
    auto hub = chat->ccReady() ? "SECURE" : ClientManager::getInstance()->getFormatedHubNames(chat->getHintedUser());
    set_title("Conversation with " + nicks + " (" + hub + ")");
	set_name("PM:" + ClientManager::getInstance()->getNick(chat->getUser(), chat->getHubUrl()));
}

void WindowPrivateMessage::handle_line(const std::string &line)
{
    if (line.empty())
        return;

    string error;
    if (!ClientManager::getInstance()->privateMessage(chat->getHintedUser(), line, error, false)) {
        add_line(display::LineEntry("Failed to send the message: " + error));
    }
}

void WindowPrivateMessage::get_list()
{
    try {
        QueueManager::getInstance()->addList(chat->getHintedUser(), QueueItem::FLAG_CLIENT_VIEW);
    } catch(Exception &e) {
		add_line(display::LineEntry("Couldn't get file list from: " + ClientManager::getInstance()->getFormatedNicks(chat->getHintedUser()) + " " + e.getError()));
    }
}

void WindowPrivateMessage::readLog() {
	if (SETTING(SHOW_LAST_LINES_LOG) == 0) return;

	try {
		File f(chat->getLogPath(), File::READ, File::OPEN);

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
    chat->removeListener(this);
}

void WindowPrivateMessage::onOnlineStateChanged() {
	updateTitles();

	if (m_state == display::STATE_IS_ACTIVE) {
		events::emit("window changed");
		events::emit("window updated", this);
	}
}

void WindowPrivateMessage::addStatusMessage(const string& aMsg) {
	add_line(display::LineEntry(aMsg));
}


void WindowPrivateMessage::on(PrivateChatListener::UserUpdated) noexcept{
	callAsync([this] { onOnlineStateChanged(); });
}

//void WindowPrivateMessage::on(PrivateChatListener::CCPMStatusChanged, const string& aMessage) noexcept{
//	callAsync([this, aMessage] {
		//addStatusLine(Text::toT(aMessage), LogManager::LOG_INFO);
		//updateOnlineStatus(true);
//	});
//}

void WindowPrivateMessage::on(PrivateChatListener::StatusMessage, const string& aMessage, uint8_t) noexcept{
	callAsync([=] {
		addStatusMessage(aMessage);
	});
}

void WindowPrivateMessage::on(PrivateChatListener::PMStatus, uint8_t aType) noexcept{
	callAsync([this, aType] {
		//updatePMInfo(aType);
	switch (aType) {
	case PrivateChat::CCPM_ESTABLISHED:
		addStatusMessage(STRING(CCPM_ESTABLISHED));
		//updateOnlineStatus(true);
		break;

	case PrivateChat::CCPM_ESTABLISHING:
		addStatusMessage(STRING(CCPM_ESTABLISHING));
		break;

	case PrivateChat::CCPM_DISCONNECTED:
		addStatusMessage(STRING(CCPM_DISCONNECTED));
		//updateOnlineStatus(true);
		break;

	case PrivateChat::CCPM_CONNECTION_TIMEOUT:
		addStatusMessage(STRING(CCPM_TIMEOUT));
		break;

	case PrivateChat::CCPM_ERROR:
		addStatusMessage(chat->getLastCCPMError());
		break;
	default:
		break;
	}
	});
}

void WindowPrivateMessage::on(PrivateChatListener::PrivateMessage, const ChatMessage& aMessage) noexcept{
	callAsync([=] {
		addMessage(aMessage);
	});
}

void WindowPrivateMessage::on(PrivateChatListener::Activate, const string& msg, Client* c) noexcept{
	callAsync([this, msg, c] {

	});
}

void WindowPrivateMessage::on(PrivateChatListener::Close) noexcept {
    
}

void WindowPrivateMessage::handleEncrypt() {
    if (chat->ccReady()) {
        chat->closeCC(false, true);
    } else {
        chat->startCC();
    }
}

std::string WindowPrivateMessage::get_my_nick() const {
    return ClientManager::getInstance()->getMyNick(chat->getHubUrl());
}

} // namespace ui
