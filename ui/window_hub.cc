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
#include <input/completion.h>

#include <core/events.h>
#include <core/argparser.h>

#include <client/LogManager.h>
#include <client/FavoriteManager.h>
#include <client/ShareManager.h>
#include <client/StringTokenizer.h>

#include <boost/algorithm/cxx11/copy_if.hpp>
#include <boost/range/adaptor/filtered.hpp>

namespace ui {

	using boost::adaptors::filtered;

WindowHub::WindowHub(const std::string &address):
    m_client(nullptr),
    m_lastJoin(0),
    m_joined(false),
    m_timer(false),
    m_currentUser(m_users.end()),
	ScrolledWindow(address, display::TYPE_HUBWINDOW),
	createdConn(events::add_listener_last("hub created", boost::bind(&WindowHub::handleCreated, this))),
	commands({
		{ "fav", boost::bind(&WindowHub::handleFav, this), nullptr },
		{ "msg", boost::bind(&WindowHub::handleMsg, this), COMPLETION(WindowHub::complete), false },
		{ "names", boost::bind(&WindowHub::handleNames, this), nullptr },
		{ "reconnect", boost::bind(&WindowHub::reconnect, this), nullptr },
		{ "showjoins", boost::bind(&WindowHub::handleShowJoins, this), nullptr }
	})
{
	help.reset(new HelpHandler(&commands, "Hub-specific", this));
    set_title(address);
    set_name(address);
    update_config();
}

void WindowHub::print_help() {
	if (*display::Manager::get()->get_current() != this)
		return;

	add_line(display::LineEntry("Hub context: /fav /names /reconnect /showjoins"));
}

void WindowHub::handleMsg() {
	core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
	parser.parse();

	if (parser.args() < 1) {
		display::Manager::get()->cmdMessage("Not enough parameters given");
		return;
	}

	auto nick = parser.arg(0);

	auto p = m_users.find(nick);
	if (p == m_users.end()) {
		display::Manager::get()->cmdMessage("User " + nick + " not found");
		return;
	}

	auto user = p->second;
	auto pm = WindowPrivateMessage::getWindow(HintedUser(user->getUser(), user->getHubUrl()), m_client->getMyNick());

	if (parser.args() > 1) {
		/* all text after first argument (nick) */
		auto line = parser.get_text(1);
		pm->handle_line(line);
	}
}

void WindowHub::handleFav() noexcept{
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
	utils::Lock l(m_mutex);
	print_names();
}

void WindowHub::handleCreated() noexcept{
	auto url = events::arg<std::string>(0);
	if (url == get_title()) {
		utils::Lock l(m_mutex);
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
	bool myPM = aMessage.from->getUser() == ClientManager::getInstance()->getMe();

	utils::Lock l(m_mutex);

	auto text(strings::escape(aMessage.text));
	auto nick = aMessage.from->getIdentity().getNick();

	// don't show the message if the nick is ignored
	int matches = std::count_if(m_ignoreNicks.begin(), m_ignoreNicks.end(),
		std::bind1st(std::equal_to<std::string>(), nick));
	if (matches)
		return;

	if (m_client->get(HubSettings::LogMainChat)) {
		ParamMap params;
		params["message"] = aMessage.format();
		m_client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = Util::cleanPathChars(m_client->getHubUrl());
		m_client->getMyIdentity().getParams(params, "my", true);
		LOG(LogManager::CHAT, params);
	}

	auto flag = display::LineEntry::MESSAGE;

	if (myPM) {
		/* bold my own nick */
		nick = "%21" + nick + "%21";
	} else if (utils::find_in_string(text, m_highlights.begin(), m_highlights.end())) {
		nick = "%21%06" + nick + "%21%06";
		flag = display::LineEntry::HIGHLIGHT;
	}

	bool op = aMessage.from->getIdentity().isOp();
	bool bot = aMessage.from->getIdentity().isBot();
	char status = (op ? '@' : (bot ? '$' : ' '));

	std::ostringstream displaySender;
	displaySender << "%21%08<%21%08" << status << nick << "%21%08>%21%08";

	int indent = 4 + g_utf8_strlen(nick.c_str(), -1);

	StringTokenizer<string> lines(text, '\n');
	for (const auto& l : lines.getTokens()) {
		add_line(display::LineEntry(displaySender.str() + " " + l, indent, time(0), flag));
	}
}

void WindowHub::handleShowJoins() {
	string status;
	if (m_client->changeBoolHubSetting(HubSettings::ShowJoins)) {
		status = STRING(JOIN_SHOWING_ON);
	} else {
		status = STRING(JOIN_SHOWING_OFF);
	}

	add_line(display::LineEntry(status));
}

void WindowHub::onPrivateMessage(const ChatMessage& aMessage) noexcept{
	bool myPM = aMessage.replyTo->getUser() == ClientManager::getInstance()->getMe();

	utils::Lock l(m_mutex);

	auto nick = aMessage.from->getIdentity().getNick();
	auto text = strings::escape(aMessage.text);

	auto dm = display::Manager::get();
	ui::WindowPrivateMessage *pm;

	const auto& user = myPM ? aMessage.to : aMessage.replyTo;

	auto name = "PM:" + nick;

	auto it = dm->find(display::TYPE_PRIVMSG, user->getUser()->getCID().toBase32());
	if (it == dm->end()) {
		/* don't filter if i'm the sender */
		if (!myPM && !filter_messages(nick, text)) {
			core::Log::get()->log("MY PM");
			return;
		}

		pm = new ui::WindowPrivateMessage(HintedUser(user->getUser(), user->getHubUrl()), m_client->getMyNick());
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

	if (SETTING(LOG_PRIVATE_CHAT)) {
		ParamMap params;
		params["message"] = aMessage.format();
		pm->fillLogParams(params);
		LogManager::getInstance()->log(user->getUser(), params);
	}

	//boost::replace_all(text, "\n", "\\");

	StringTokenizer<string> lines(text, '\n');
	auto displaySender = myPM ? "%21%08<%21%08%21" + nick + "%21%21%08>%21%08" : "%21%08<%21%08" + nick + "%21%08>%21%08";

	int indent = 4 + g_utf8_strlen(nick.c_str(), -1);
	for (const auto& l : lines.getTokens()) {
		pm->add_line(display::LineEntry(displaySender + " " + l, indent, time(0), display::LineEntry::MESSAGE));
	}

	if (!myPM && pm->get_state() != display::STATE_IS_ACTIVE) {
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
	if (SETTING(LOG_STATUS_MESSAGES)) {
		ParamMap params;
		m_client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = m_client->getHubUrl();
		m_client->getMyIdentity().getParams(params, "my", true);
		params["message"] = line;
		LOG(LogManager::STATUS, params);
	}

	std::string tmp = strings::escape(line);
	add_line(display::LineEntry(tmp));
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
	auto nick = aUser->getIdentity().getNick();
	if (m_joined
		&& m_users.find(nick) == m_users.end()
		&& (m_client->get(HubSettings::ShowJoins) ||
		utils::find_in_string(nick, m_showNicks.begin(), m_showNicks.end())
		)
		) {
			// Lehmis [127.0.0.1] has joined the hub
			std::ostringstream oss;
			auto ip = aUser->getIdentity().getIp();
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

	bool showJoin = m_client->get(HubSettings::ShowJoins) ||
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
	updateTitle();
}

void WindowHub::on(ClientListener::Failed, const string& /*url*/, const string& msg)
    noexcept
{
    utils::Lock l(m_mutex);

	updateTitle();
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
	utils::Lock l(m_mutex);
	updateTitle();
}
void WindowHub::on(ClientListener::AddLine, const Client*, const string& aMsg) noexcept{
	add_line(display::LineEntry(aMsg));
}

void WindowHub::reconnect() {
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
	updateTitle();
    TimerManager::getInstance()->addListener(this);
    m_timer = true;
}

void WindowHub::updateTitle() {
	std::string title;
	if (m_client->isConnected()) {
		title = m_client->getHubName() + " - " + m_client->getHubDescription() + " (" + m_client->getHubUrl() + ")";
	} else {
		title = m_client->getAddress() + " (offline)";
	}

	set_title(title);
	set_name(m_client->getHubName());
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

void WindowHub::complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_) {
	if (aArgs.empty())
		return;

	auto s = aArgs[pos];

	utils::Lock l(m_mutex);
	for (const auto& n : m_users | map_keys | filtered(input::PrefixComparator(s))) {
		suggest_.push_back(pos == 0 ? n + ": " : n);
	}
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

	createdConn.disconnect();

	//events::remove_listener("command reconnect", std::bind(&WindowHub::reconnect, this));
	//events::remove_listener("hub created", std::bind(&WindowHub::handleConnect, this));
}

} // namespace ui
