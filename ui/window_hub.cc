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
#include <core/log.h>
#include <utils/utils.h>
#include <input/completion.h>
#include <input/manager.h>

#include <core/events.h>
#include <core/argparser.h>

#include <client/GeoManager.h>
#include <client/LogManager.h>
#include <client/FavoriteManager.h>
#include <client/HubEntry.h>
#include <client/QueueManager.h>
#include <client/ShareManager.h>
#include <client/StringTokenizer.h>

#include <boost/algorithm/cxx11/copy_if.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/algorithm/count_if.hpp>

namespace ui {

using boost::adaptors::filtered;

WindowHub::WindowHub(const std::string &address):
	ScrolledWindow(address, display::TYPE_HUBWINDOW),
	commands({
		{ "fav", boost::bind(&WindowHub::handleFav, this), nullptr },
		{ "browse", boost::bind(&WindowHub::handleBrowse, this), COMPLETION(WindowHub::complete), false },
		{ "msg", boost::bind(&WindowHub::handleMsg, this), COMPLETION(WindowHub::complete), false },
        { "info", boost::bind(&WindowHub::handleInfo, this), COMPLETION(WindowHub::complete), false },
        { "names", boost::bind(&WindowHub::handleNames, this), nullptr },
		{ "reconnect", boost::bind(&WindowHub::handleReconnect, this), nullptr },
		{ "showjoins", boost::bind(&WindowHub::handleShowJoins, this), nullptr }
	})
{
	timedEvents.reset(new DelayedEvents<int>());
	help.reset(new HelpHandler(&commands, "Hub-specific", this));
    set_title(address);
    set_name(address);
    update_config();
	m_bindings[INPUT_CTRL('R')] = std::bind(&WindowHub::handleReconnect, this);
}

void WindowHub::print_help() {
	if (*display::Manager::get()->get_current() != this)
		return;

	add_line(display::LineEntry("Hub context: /fav /names /reconnect /showjoins"));
}

OnlineUserPtr WindowHub::getUserFromParam() {
    core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
    parser.parse();

    if (parser.args() < 1) {
        display::Manager::get()->cmdMessage("Not enough parameters given");
        return nullptr;
     }   

    auto nick = parser.arg(0);

    auto p = m_users.find(nick);
    if (p == m_users.end()) { 
        display::Manager::get()->cmdMessage("User " + nick + " not found");
        return nullptr; 
    }

    return p->second;
}

void WindowHub::handleBrowse() {
	auto user = getUserFromParam();
    if(!user)
        return;

	try {
		QueueManager::getInstance()->addList(HintedUser(user->getUser(), m_client->getHubUrl()), QueueItem::FLAG_CLIENT_VIEW | QueueItem::FLAG_PARTIAL_LIST);
	} catch (const Exception& e) {
		LogManager::getInstance()->message(e.getError(), LogManager::LOG_ERROR);
	}
}

void WindowHub::handleMsg() {
    auto user = getUserFromParam();
    if (!user)
        return;

    auto pm = WindowPrivateMessage::getWindow(HintedUser(user->getUser(), user->getHubUrl()), m_client->getMyNick());

    core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
    parser.parse();
    if (parser.args() > 1) {
        /* all text after first argument (nick) */
        auto line = parser.get_text(1);
        pm->handle_line(line);
    }
}

struct FieldName {
    string field;
    string name;
    string (*convert)(const string &val);
};
static string formatBytes(const string& val) {
    return Util::formatBytes(Util::toInt64(val));
}

static string formatSpeed(const string& val) {
    return Util::formatConnectionSpeed(Util::toInt64(val));
}

static string formatIP(const string& val) {
    auto country = GeoManager::getInstance()->getCountry(val);
    if (!country.empty())
        return val + " (" + country + ")";
    return val;
}

static const FieldName fields[] =
{
    { "NI", "Nick: ", nullptr },
    { "AW", "Away: ", nullptr },
    { "DE", "Description: ", nullptr },
    { "EM", "E-Mail: ", nullptr },
    { "SS", "Shared: ", &formatBytes },
    { "SF", "Shared files: ", nullptr },
    { "US", "Upload speed: ", &formatSpeed },
    { "DS", "Download speed: ", &formatSpeed },
    { "SL", "Total slots: ", nullptr },
    { "FS", "Free slots: ", nullptr },
    { "HN", "Hubs (normal): ", nullptr },
    { "HR", "Hubs (registered): ", nullptr },
    { "HO", "Hubs (op): ", nullptr },
    { "I4", "IP (v4): ", &formatIP },
    { "I6", "IP (v6): ", &formatIP },
    { "U4", "Search port (v4): ", nullptr },
    { "U6", "Search port (v6): ", nullptr },
    { "SU", "Features: ", nullptr },
    { "VE", "Application version: ", nullptr },
    { "AP", "Application: ", nullptr },
    { "ID", "CID: ", nullptr },
    { "KP", "TLS Keyprint: ", nullptr },
    { "CO", "Connection: ", nullptr },
    //{ "CT", "Client type: ", nullptr },
    { "TA", "Tag: ", nullptr },
    { "", "", 0 }
};


void WindowHub::handleInfo() {
    auto user = getUserFromParam();
    if (!user)
        return;

    display::Manager::get()->cmdMessage("");
    const auto info = user->getIdentity().getInfo();
    for (const auto& field: fields) {
        auto i = info.find(field.field);
        if (i != info.end()) {
            string print = field.name;
            if (field.convert) {
                print += field.convert(i->second);
            } else {
                print += i->second;
            }

            print += "\n";
            display::Manager::get()->cmdMessage(print);
        }
    }

    display::Manager::get()->cmdMessage("");
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
	print_names();
}

void WindowHub::handleCreated() noexcept{
	m_client = ClientManager::getInstance()->getClient(get_title());
	dcassert(m_client);
	m_client->addListener(this);
	m_client->connect();
}

void WindowHub::update_config()
{
    this->ScrolledWindow::update_config();

    core::Settings *settings = core::Settings::get();
    m_ignoreNicks = settings->find_vector("ignore_nicks");
    m_highlights = settings->find_vector("hilight_words");
	m_blockedWords = settings->find_vector("block_messages");

    m_showNickList = settings->find_bool("show_nicklist", false);
    m_resolveIps = settings->find_bool("resolve_ips", false);
}

void WindowHub::handle_line(const std::string &line)
{
    if (line.empty())
	return;

    if (!m_client || !m_client->isConnected()) {
        return;
    }

    string error;
    if (!m_client->hubMessage(Text::toUtf8(line), error)) {
        add_line(display::LineEntry("Failed to send: " + error));
    }
}

void WindowHub::onJoinedTimer() {
    // group users in 1,5 seconds
	m_joined = true;
	callAsync([this] {
		if (m_client->isConnected()) {
			add_line(display::LineEntry("Joined to the hub"));
			if (m_showNickList)
				print_names();
		}
	});
}

void WindowHub::onChatMessage(const ChatMessage& aMessage) noexcept{
	bool myMessage = aMessage.from->getUser() == ClientManager::getInstance()->getMe();

	auto text(utils::escape(aMessage.text));
	auto nick = aMessage.from->getIdentity().getNick();

	bool op = aMessage.from->getIdentity().isOp();
	bool bot = aMessage.from->getIdentity().isBot();

	// don't show the message if the nick is ignored
	if (!myMessage && (!op || m_client->isOp()) && !bot && filter_messages(nick, aMessage.text)) {
		return;
	}

	if (m_client->get(HubSettings::LogMainChat)) {
		ParamMap params;
		params["message"] = aMessage.format();
		m_client->getHubIdentity().getParams(params, "hub", false);
		params["hubURL"] = Util::validateFileName(m_client->getHubUrl());
		m_client->getMyIdentity().getParams(params, "my", true);
		LOG(LogManager::CHAT, params);
	}

	auto flag = display::LineEntry::MESSAGE;

	if (myMessage) {
		/* bold my own nick */
		nick = "%21" + nick + "%21";
	} else if (utils::find_in_string(text, m_highlights.begin(), m_highlights.end())) {
		nick = "%21%06" + nick + "%21%06";
		flag = display::LineEntry::HIGHLIGHT;
	}

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

	auto nick = aMessage.from->getIdentity().getNick();
	auto text = utils::escape(aMessage.text);

	auto dm = display::Manager::get();
	ui::WindowPrivateMessage *pm;

	const auto& user = myPM ? aMessage.to : aMessage.replyTo;

	auto name = "PM:" + nick;

	auto it = dm->find(display::TYPE_PRIVMSG, user->getUser()->getCID().toBase32());
	if (it == dm->end()) {
		/* don't filter if i'm the sender */
		if (!myPM && filter_messages(nick, text)) {
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
	callAsync([=] {
		if (aMessage.to && aMessage.replyTo) {
			onPrivateMessage(aMessage);
		} else {
			onChatMessage(aMessage);
		}
	});
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

	auto tmp = utils::escape(line);
	callAsync([=] { add_line(display::LineEntry(tmp)); });
}

void WindowHub::handleUserUpdated(const OnlineUserPtr& aUser) {
	if (aUser->isHidden())
		return;

	if (!m_joined && !aUser->getIdentity().isBot()) {
		timedEvents->addEvent(0, [this] { onJoinedTimer(); }, 1500);
	}

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

	m_users.emplace(nick, aUser);
}

void WindowHub::on(ClientListener::UserConnected, const Client* c, const OnlineUserPtr& aUser) noexcept{
	callAsync([=] { handleUserUpdated(aUser); });
}

void WindowHub::on(ClientListener::UserUpdated, const Client*, const OnlineUserPtr& aUser) noexcept {
	callAsync([=] { handleUserUpdated(aUser); });
}

void WindowHub::handleUserRemoved(const OnlineUserPtr& aUser) {
	auto nick = aUser->getIdentity().getNick();
	auto p = m_users.find(nick);
	if (p == m_users.end())
		return;

	m_users.erase(p);

	bool showJoin = m_client->get(HubSettings::ShowJoins) ||
		utils::find_in_string(nick, m_showNicks.begin(),
		m_showNicks.end());

	if (m_users.find(nick) == m_users.end() && m_joined && showJoin) {
		// Lehmis [127.0.0.1] has left the hub
		std::ostringstream oss;
		std::string ip = aUser->getIdentity().getIp();
		oss << "%03" << nick << "%03 ";
		if (!ip.empty()) {
			if (m_resolveIps) {
				try {
					ip = utils::ip_to_host(ip);
				} catch (std::exception &e) {
					//core::Log::get()->log("utils::ip_to_host(" + ip + "): " + std::string(e.what()));
				}
			}
			oss << "%21%08[%21%08" << ip
				<< "%21%08]%21%08 ";
		}

		oss << "has left the hub";
		add_line(display::LineEntry(oss.str()));
	}

	/*if (!m_joined) {
		m_joined = true;
		if (m_client->isConnected()) {
			onJoinedTimer();
		} else {
			core::Log::get()->log(m_client->getAddress() + " is not connected");
		}
	}*/
}

void WindowHub::on(ClientListener::UserRemoved, const Client*, const OnlineUserPtr& aUser)
    noexcept
{
	callAsync([=] { handleUserRemoved(aUser); });
}

void WindowHub::on(ClientListener::UsersUpdated, const Client*, const OnlineUserList &users)
    noexcept
{
	callAsync([=] {
		for (const auto& u : users) {
			if (!u->isHidden())
				m_users.emplace(u->getIdentity().getNick(), u);
		}
	});
}

void WindowHub::on(ClientListener::GetPassword, const Client*)
    noexcept
{
	callAsync([=] { handlePassword();  });
}

void WindowHub::handlePassword() {
	if (!m_client->getPassword().empty()) {
		add_line(display::LineEntry("Sending password"));
		m_client->password(m_client->getPassword());
	} else {
		m_input.setText("/password ");
		passwordConn = events::add_listener("command password", [=] {
			if (display::Manager::get()->get_current_window() != this)
				return;

			auto pw = events::arg<std::string>(0);
			m_client->password(pw);
			passwordConn.disconnect();
			m_input.setText("");
		});
	}
}

void WindowHub::on(ClientListener::HubUpdated, const Client *client)
    noexcept
{
	callAsync([=] { updateTitle(); });
}

void WindowHub::handleFailed(const std::string& aMsg) {
	updateTitle();
	add_line(display::LineEntry(aMsg));
	m_joined = false;
	timedEvents->clear();
	m_users.clear();
}

void WindowHub::on(ClientListener::Failed, const string& /*url*/, const string& msg)
    noexcept
{
	callAsync([=] { handleFailed(msg); });
}

void WindowHub::on(ClientListener::Connecting, const Client*) noexcept{
	callAsync([=] { add_line(display::LineEntry("Connecting to " + m_client->getHubUrl() + " ...")); });
}

void WindowHub::on(ClientListener::Redirect, const Client*, const string &msg) noexcept{
	callAsync([=] { add_line(display::LineEntry("Redirect: ")); });
}

void WindowHub::on(ClientListener::HubTopic, const Client*, const string&) noexcept{
	callAsync([=] { updateTitle(); });
}
void WindowHub::on(ClientListener::AddLine, const Client*, const string& aMsg) noexcept{
	callAsync([=] { add_line(display::LineEntry(aMsg)); });
}

void WindowHub::handleReconnect() {
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
			hub->get_client()->connect();
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

struct _identity
{
    typedef bool result_type;
    _identity(bool (Identity::*__pf)()const) : _M_f(__pf) {}
    bool operator()(const std::pair<std::string, const OnlineUserPtr> &pair) const {
        return (pair.second->getIdentity().*_M_f)();
    }
    bool (Identity::*_M_f)()const;
};

void WindowHub::print_names()
{
    for(auto i=m_users.begin(); i != m_users.end();) {
        std::ostringstream oss;
        for(int j=0; j<4 && i != m_users.end(); ++j, ++i) {
            int length = utils::length(i->first) - 18;
            oss << "%21%08[%21%08" << i->first
                << std::string(std::max(0, length), ' ')
                << "%21%08]%21%08 ";
        }
        display::LineEntry line(oss.str(), 0, -1, display::LineEntry::MESSAGE);
        add_line(line, false);
    }

    int ops = std::count_if(m_users.begin(), m_users.end(), _identity(&Identity::isOp));
    int bots = std::count_if(m_users.begin(), m_users.end(), _identity(&Identity::isBot));
	int active = boost::count_if(m_users | map_values, [this](const OnlineUserPtr& ou) { return !ou->getIdentity().isBot() && ou->getIdentity().isTcpActive(m_client); });
    int hidden = std::count_if(m_users.begin(), m_users.end(), _identity(&Identity::isHidden));

    std::ostringstream oss;
    oss << m_users.size() << " users " << ops << "/" << bots << "/" 
        << m_users.size()-active-bots-hidden << /*"/" << hidden <<*/ " ops/bots/passive";
    add_line(display::LineEntry(oss.str()));
}

void WindowHub::on(ClientListener::Connected, const Client*)
    noexcept
{
	callAsync([=] { handleConnected(); });
}

void WindowHub::handleConnected() {
	add_line(display::LineEntry("Connected"));
	updateTitle();
}

void WindowHub::updateTitle() {
	std::string title;
	if (m_client->isConnected()) {
		title = m_client->getHubName() + (!m_client->getHubDescription().empty() ? " - " + m_client->getHubDescription() : "") + " (" + m_client->getHubUrl() + ")";
	} else {
		title = m_client->getAddress() + " (offline)";
	}

	set_title(title);
	set_name(m_client->getHubName());
}

bool WindowHub::filter_messages(const std::string &nick, const std::string &msg)
{
	if (utils::find_in_string(msg, m_blockedWords.begin(), m_blockedWords.end()) ||
        utils::find_in_string(nick, m_ignoreNicks.begin(), m_ignoreNicks.end()))
    {
		//core::Log::get()->log("%21Ignore:%21 from: " + nick + ", msg: " + msg + " " + Util::listToString(m_blockedWords) + Util::listToString(m_ignoreNicks));
        return true;
    }
    return false;
}

std::string WindowHub::get_nick() const
{
    if(!m_client)
        return utils::empty_string;

    std::string nick;
    if(m_client->isOp())
        nick = "%21@%21";
    nick += m_client->getMyNick();
    return nick;
}

const OnlineUserPtr WindowHub::get_user(const std::string &nick) {
    auto it = m_users.find(nick);
	return it != m_users.end() ? it->second : nullptr;
}

void WindowHub::complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_, bool& appendSpace_) {
	if (aArgs.empty())
		return;

	auto s = aArgs[pos];

	for (const auto& n : m_users | map_keys | filtered(input::PrefixComparator(s))) {
		suggest_.push_back(pos == 0 ? n + ":" : n);
	}
	appendSpace_ = true;
}

WindowHub::~WindowHub()
{
    if(m_client) {
        m_client->removeListener(this);
        ClientManager::getInstance()->putClient(m_client);
    }
    m_users.clear();
}

} // namespace ui
