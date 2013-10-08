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
#include <functional>
#include <display/manager.h>
#include <ui/window_hub.h>
#include <ui/window_privatemessage.h>
#include <utils/strings.h>
#include <core/log.h>
#include <utils/utils.h>
#include <utils/lock.h>
#include <utils/strings.h>
#include <client/HubEntry.h>
#include <core/events.h>

#include <client/FavoriteManager.h>
#include <client/ShareManager.h>

#include <boost/algorithm/string/replace.hpp>

namespace ui {


WindowHub::WindowHub(const std::string &address):
    m_client(nullptr),
    m_lastJoin(0),
    m_joined(false),
    m_timer(false),
    m_currentUser(m_users.end()),
	ScrolledWindow(address, display::TYPE_HUBWINDOW),
	reconnectConn(events::add_listener("command reconnect", boost::bind(&WindowHub::reconnect, this))),
	createdConn(events::add_listener_last("hub created", boost::bind(&WindowHub::handleCreated, this))),
	favConn(events::add_listener_last("command fav", boost::bind(&WindowHub::handleFav, this))),
	favoriteConn(events::add_listener_last("command favorite", boost::bind(&WindowHub::handleFav, this))),
	namesConn(events::add_listener_last("command names", boost::bind(&WindowHub::handleNames, this))),
	helpConn(events::add_listener_last("command help", boost::bind(&WindowHub::print_help, this)))
{
    set_title(address);
    set_name(address);
    update_config();
}

void WindowHub::print_help() {
	if (*display::Manager::get()->get_current() != this)
		return;

	add_line(display::LineEntry("Hub context: /fav /names /reconnect"));
}

void WindowHub::handleFav() noexcept{
	if (*display::Manager::get()->get_current() != this)
		return;

	auto existingHub = FavoriteManager::getInstance()->getFavoriteHubEntry(m_client->getHubUrl());
	if (!existingHub) {
		FavoriteHubEntryPtr e = new FavoriteHubEntry();
		e->setServerStr(m_client->getHubUrl());
		e->setName(m_client->getHubName());
		e->setDescription(m_client->getHubDescription());
		e->setConnect(true);
		e->setShareProfile(ShareManager::getInstance()->getShareProfile(m_client->getShareProfile(), true));
		if (!m_client->getPassword().empty()) {
			e->setPassword(m_client->getPassword());
		}

		FavoriteManager::getInstance()->addFavorite(e);
		add_line(display::LineEntry("Favorite hub added"));
	} else {
		add_line(display::LineEntry("The hub exists in favorites already"));
	}
}

void WindowHub::handleNames() {
	if (*display::Manager::get()->get_current() != this)
		return;

	utils::Lock l(m_mutex);
	print_names();
}

void WindowHub::handleCreated() noexcept{
	auto url = events::arg<std::string>(0);
	if (url == get_title()) {
		utils::Lock l(m_mutex);
		auto vec = core::Settings::get()->find_vector("show_joins_hubs");
		m_showJoinsOnThisHub = utils::find_in_string(m_client->getAddress(), vec.begin(), vec.end());

		m_client = ClientManager::getInstance()->getClient(url);
		dcassert(m_client);
		m_client->addListener(this);
		m_client->connect();
	}
}

void WindowHub::update_config()
{
    utils::Lock l(m_mutex);

    this->ScrolledWindow::update_config();

    core::Settings *settings = core::Settings::get();
    m_showNicks = settings->find_vector("show_joins_nicks");
    m_ignoreNicks = settings->find_vector("ignore_nicks");
    m_highlights = settings->find_vector("hilight_words");

    if(m_client) {
        auto vec = settings->find_vector("show_joins_hubs");
        m_showJoinsOnThisHub = utils::find_in_string(m_client->getAddress(), vec.begin(), vec.end());
    }

    m_showJoins = settings->find_bool("show_joins", false);
    m_showNickList = settings->find_bool("show_nicklist", false);
    m_resolveIps = settings->find_bool("resolve_ips", false);
    m_utf8 = settings->find_bool("utf8_input", false);
    m_nmdcCharset = settings->find("nmdc_charset", "ISO-8859-1");
}

void WindowHub::handle_line(const std::string &line)
{
    utils::Lock l(m_mutex);

	if (!m_client || !m_client->isConnected()) {
		return;
	}

	string error;
	if (!m_client->hubMessage(Text::toUtf8(line), error)) {
		add_line(display::LineEntry("Failed to send: " + error));
	}
}

void WindowHub::on(TimerManagerListener::Second, uint64_t)
    noexcept
{
    utils::Lock l(m_mutex);

    // group users in 2 seconds
    if(m_lastJoin && !m_joined && m_lastJoin+2000 < TimerManager::getInstance()->getTick()) {
        m_joined = true;
        TimerManager::getInstance()->removeListener(this);
        m_timer = false;
        if(m_client->isConnected()) {
            add_line(display::LineEntry("Joined to the hub"));
            if(m_showNickList)
                print_names();
        }
    }
}

void WindowHub::onChatMessage(const ChatMessage& aMessage) noexcept{
	utils::Lock l(m_mutex);

	std::string temp(aMessage.text);
	boost::replace_all(temp, "\n", " ");
	temp = strings::escape(temp);

	//auto from = HintedUser(aMessage.from->getUser(), aMessage.from->getHubUrl());
	auto nick = aMessage.from->getIdentity().getNick();

	// don't show the message if the nick is ignored
	int matches = std::count_if(m_ignoreNicks.begin(), m_ignoreNicks.end(),
		std::bind1st(std::equal_to<std::string>(), nick));
	if (matches)
		return;

	display::LineEntry::Type flag = display::LineEntry::MESSAGE;

	/* XXX: this is a bug in core?
	* m_client->getMyNick() returns the real nick
	* user.getUser()->getFirstNick() returns SETTING(NICK) */
	std::string realNick = m_client->getMyNick();
	std::string nick2 = SETTING(NICK);
	if (nick2 == nick || nick == realNick) {
		/* bold my own nick */
		nick = "%21" + realNick + "%21";
	} else if (utils::find_in_string(temp, m_highlights.begin(), m_highlights.end())) {
		nick = "%21%06" + nick + "%21%06";
		flag = display::LineEntry::HIGHLIGHT;
	}

	bool op = aMessage.from->getIdentity().isOp();
	bool bot = aMessage.from->getIdentity().isBot();
	char status = (op ? '@' : (bot ? '$' : ' '));

	std::ostringstream line;
	line << "%21%08<%21%08" << status << nick << "%21%08>%21%08 " << temp;

	int indent = 4 + g_utf8_strlen(nick.c_str(), -1);
	//int indent = 4;

	add_line(display::LineEntry(line.str(), indent, time(0), flag));
}

void WindowHub::onPrivateMessage(const ChatMessage& aMessage) noexcept{
	bool myPM = aMessage.replyTo->getUser() == ClientManager::getInstance()->getMe();

	utils::Lock l(m_mutex);

	auto nick = aMessage.from->getIdentity().getNick();
	auto text = aMessage.text;

	//core::Log::get()->log("privmsg: From:" + nick +
	//	", To: " + aMessage.to->getIdentity().getNick() + " reply: " + aMessage.replyTo->getIdentity().getNick() + " Message:" + text);

	auto dm = display::Manager::get();
	ui::WindowPrivateMessage *pm;

	const auto& user = myPM ? aMessage.to : aMessage.replyTo;

	/* XXX: see the explanation in WindowHub(Message, ...) */
	std::string realNick = m_client->getMyNick();
	/*if (aMessage.replyTo->getUser() == ClientManager::getInstance()->getMe() ||
		nick == SETTING(NICK)) {
			me = true;
			user = aMessage.to->getUser();
	}*/

	std::string name = "PM:" + nick;

	auto it = dm->find(display::TYPE_PRIVMSG, user->getUser()->getCID().toBase32());
	if (it == dm->end()) {
		/* don't filter if i'm the sender */
		if (!myPM && !filter_messages(nick, text)) {
			core::Log::get()->log("MY PM");
			return;
		}

		pm = new ui::WindowPrivateMessage(HintedUser(aMessage.from->getUser(), aMessage.from->getHubUrl()), realNick);
		dm->push_back(pm);
	} else {
		pm = static_cast<ui::WindowPrivateMessage*>(*it);
	}

	boost::replace_all(text, "\n", "\\");

	if (myPM) {
		pm->add_line("%21%08<%21%08%21" + realNick + "%21%21%08>%21%08 " + text);
	} else {
		pm->add_line("%21%08<%21%08" + nick + "%21%08>%21%08 " + text);
		if (pm->get_state() != display::STATE_IS_ACTIVE)
			pm->set_state(display::STATE_HIGHLIGHT);
	}
}

void WindowHub::on(ClientListener::Message, const Client *, const ChatMessage& aMessage) noexcept{
	if (aMessage.to && aMessage.replyTo) {
		onPrivateMessage(aMessage);
	} else {
		onChatMessage(aMessage);
	}
}

void WindowHub::on(ClientListener::StatusMessage, const Client*, const string& line, int)
    noexcept
{
    std::string tmp = strings::escape(line);
    if(!tmp.empty())
        add_line(display::LineEntry(tmp));
    else
        core::Log::get()->log("WindowHub::on(StatusMessage...): received empty line from " + m_client->getAddress());
}

void WindowHub::on(ClientListener::UserConnected, const Client* c, const OnlineUserPtr& aUser) noexcept{
	on(UserUpdated(), c, aUser);
}

void WindowHub::on(ClientListener::UserUpdated, const Client*, const OnlineUserPtr& aUser)
    noexcept
{
	if (aUser->isHidden())
		return;

	utils::Lock l(m_mutex);

	auto user = HintedUser(aUser->getUser(), aUser->getHubUrl());
	std::string nick = aUser->getIdentity().getNick();
	if (m_joined
		&& m_users.find(nick) == m_users.end()
		&& (m_showJoins || m_showJoinsOnThisHub ||
		utils::find_in_string(nick, m_showNicks.begin(), m_showNicks.end())
		)
		) {
			// Lehmis [127.0.0.1] has joined the hub
			std::ostringstream oss;
			std::string ip = aUser->getIdentity().getIp();
			oss << "%03%21" << nick << "%03%21 ";
			if (!ip.empty()) {
				if (m_resolveIps) {
					try {
						ip = utils::ip_to_host(ip);
					} catch (std::exception &e) {
						//core::Log::get()->log("utils::ip_to_host(" + ip + "): " + std::string(e.what()));
					}
				}
				oss << "%21%08[%21%08%03" << ip
					<< "%03%21%08]%21%08 ";
			}

			oss << "has joined the hub";
			add_line(display::LineEntry(oss.str()));
	}

	m_lastJoin = GET_TICK();
	m_users[nick] = aUser.get();
}

void WindowHub::on(ClientListener::UserRemoved, const Client*, const OnlineUserPtr& aUser)
    noexcept
{
    utils::Lock l(m_mutex);
	auto nick = aUser->getIdentity().getNick();
	auto p = m_users.find(nick);
	if (p == m_users.end())
		return;

    m_users.erase(p);

    bool showJoin = m_showJoins || m_showJoinsOnThisHub ||
                    utils::find_in_string(nick, m_showNicks.begin(),
                            m_showNicks.end());

    if(m_users.find(nick) == m_users.end() && m_joined && showJoin)
    {
        // Lehmis [127.0.0.1] has left the hub
        std::ostringstream oss;
		std::string ip = aUser->getIdentity().getIp();
        oss << "%03" << nick << "%03 ";
        if(!ip.empty()) {
            if(m_resolveIps) {
                try {
                    ip = utils::ip_to_host(ip);
                } catch(std::exception &e) {
                    //core::Log::get()->log("utils::ip_to_host(" + ip + "): " + std::string(e.what()));
                }
            }
            oss << "%21%08[%21%08" << ip 
                << "%21%08]%21%08 ";
        }

        oss << "has left the hub";
        add_line(display::LineEntry(oss.str()));
    }

    if(!m_joined) {
        m_joined = true;
        if(m_timer) {
            TimerManager::getInstance()->removeListener(this);
            m_timer = false;
        }
        if(m_client->isConnected()) {
            add_line(display::LineEntry("Joined to the hub"));
            if(m_showNickList)
                print_names();
        }
        else {
            core::Log::get()->log(m_client->getAddress() + " is not connected");
        }
    }
}

void WindowHub::on(ClientListener::UsersUpdated, const Client*, const OnlineUserList &users)
    noexcept
{
    utils::Lock l(m_mutex);
    
	for (const auto& u: users) {
		if (!u->isHidden())
			m_users[u->getIdentity().getNick()] = u.get();
    }
}

void WindowHub::on(ClientListener::GetPassword, const Client*)
    noexcept
{
    utils::Lock l(m_mutex);

    add_line(display::LineEntry("Sending password"));
    m_client->password(m_client->getPassword());
}

void WindowHub::on(ClientListener::HubUpdated, const Client *client)
    noexcept
{
    utils::Lock l(m_mutex);

    std::string title = client->getHubName() + " - " + client->getHubDescription();

    set_title(title);
}

void WindowHub::on(ClientListener::Failed, const string& /*url*/, const string& msg)
    noexcept
{
    utils::Lock l(m_mutex);

    set_title(m_client->getAddress() + " (offline)");
    add_line(display::LineEntry(msg));
    m_joined = false;
    if(m_timer) {
        TimerManager::getInstance()->removeListener(this);
        m_timer = false;
    }
    m_lastJoin = 0;
    m_users.clear();
}

void WindowHub::on(ClientListener::Connecting, const Client*) noexcept{
	add_line(display::LineEntry("Connecting to " + m_client->getHubUrl() + " ...")); 
}

void WindowHub::on(ClientListener::Redirect, const Client*, const string &msg) noexcept{
	add_line(display::LineEntry("Redirect: ")); 
}

void WindowHub::on(ClientListener::HubTopic, const Client*, const string&) noexcept{

}
void WindowHub::on(ClientListener::AddLine, const Client*, const string& aMsg) noexcept{
	add_line(display::LineEntry(aMsg));
}

void WindowHub::reconnect() {
	if (*display::Manager::get()->get_current() != this)
		return;

	if (m_client)
		m_client->reconnect();
}

void WindowHub::openWindow(std::string address, ProfileToken shareProfile, bool activate)
{
	auto mger = display::Manager::get();
	auto it = mger->find(display::TYPE_HUBWINDOW, address);

	/* Check if the hub already exists. Try to reconnect and
	* activate the hub window. */
	if (it != mger->end()) {
		auto hub = static_cast<ui::WindowHub*>(*it);
		if (!hub->get_client()->isConnected())
			hub->connect();
		mger->set_current(it);
	} else {
		auto hub = new ui::WindowHub(address);
		mger->push_back(hub);
		if (activate)
			mger->set_current(it);

		RecentHubEntryPtr rp(new RecentHubEntry(address));
		ClientManager::getInstance()->createClient(rp, shareProfile); 
	}
}

void WindowHub::connect() noexcept {
    utils::Lock l(m_mutex);

    m_client->connect();
}

struct _identity
{
    typedef bool result_type;
    _identity(bool (Identity::*__pf)()const) : _M_f(__pf) {}
    bool operator()(const std::pair<std::string, const OnlineUser*> &pair) const {
        return (pair.second->getIdentity().*_M_f)();
    }
    bool (Identity::*_M_f)()const;
};

void WindowHub::print_names()
{
    std::string time = utils::time_to_string("[%H:%M:%S] ");
    for(auto i=m_users.begin(); i != m_users.end();) {
        std::ostringstream oss;
        for(int j=0; j<4 && i != m_users.end(); ++j, ++i) {
            int length = strings::length(i->first) - 18;
            oss << "%21%08[%21%08" << i->first
                << std::string(std::max(0, length), ' ')
                << "%21%08]%21%08 ";
        }
        display::LineEntry line(oss.str(), 0, -1, display::LineEntry::MESSAGE);
        add_line(line, false);
    }

    int ops = std::count_if(m_users.begin(), m_users.end(), _identity(&Identity::isOp));
    int bots = std::count_if(m_users.begin(), m_users.end(), _identity(&Identity::isBot));
    //int active = std::count_if(m_users.begin(), m_users.end(), _identity(&Identity::isTcp4Active(m_client)));
	int active = 0;
    int hidden = std::count_if(m_users.begin(), m_users.end(), _identity(&Identity::isHidden));

    std::ostringstream oss;
    oss << m_users.size() << " users " << ops << "/" << bots << "/" 
        << m_users.size()-active << "/" << hidden << " ops/bots/passive/hidden";
    add_line(display::LineEntry(oss.str()));
}

void WindowHub::on(ClientListener::Connected, const Client*)
    noexcept
{
    utils::Lock l(m_mutex);
    add_line(display::LineEntry("Connected"));
    set_title(m_client->getHubName());
    TimerManager::getInstance()->addListener(this);
    m_timer = true;
}

bool WindowHub::filter_messages(const std::string &nick, const std::string &msg)
{
    //utils::Lock l(m_mutex);

    core::Settings *settings = core::Settings::get();
    core::StringVector blocked;

    if(!settings->exists("block_messages")) {
        // block urls by default
        core::Settings::get()->set("block_messages", "http://;www.");
    }
    blocked = core::Settings::get()->find_vector("block_messages");

    if (utils::find_in_string(msg, blocked.begin(), blocked.end()) ||
        utils::find_in_string(nick, m_ignoreNicks.begin(), m_ignoreNicks.end()))
    {
        core::Log::get()->log("%21Ignore:%21 from: " + nick + ", msg: " + msg, core::MT_DEBUG);
        return false;
    }
    return true;
}

std::string WindowHub::get_nick() const
{
    utils::Lock l(m_mutex);

    if(!m_client)
        return utils::empty_string;

    std::string nick;
    if(m_client->isOp())
        nick = "%21@%21";
    nick += m_client->getMyNick();
    return nick;
}

const OnlineUser *WindowHub::get_user(const std::string &nick)
    throw(std::out_of_range)
{
    utils::Lock l(m_mutex);

    auto it = m_users.find(nick);
    if(it == m_users.end())
        throw std::out_of_range("user not found");
    return it->second;
}

WindowHub::~WindowHub()
{
    if(m_timer)
        TimerManager::getInstance()->removeListener(this);

    if(m_client) {
        m_client->removeListener(this);
        ClientManager::getInstance()->putClient(m_client);
    }
    m_users.clear();

	favoriteConn.disconnect();
	favConn.disconnect();
	reconnectConn.disconnect();
	createdConn.disconnect();
	namesConn.disconnect();

	//events::remove_listener("command reconnect", std::bind(&WindowHub::reconnect, this));
	//events::remove_listener("hub created", std::bind(&WindowHub::handleConnect, this));
}

} // namespace ui
