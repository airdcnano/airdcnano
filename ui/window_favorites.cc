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

#include <sstream>
#include <iomanip>
#include <functional>
#include <utils/utils.h>
#include <core/log.h>
#include <core/events.h>
#include <ui/window_favorites.h>
#include <ui/window_hub.h>

#include <client/ScopedFunctor.h>
#include <client/ShareManager.h>

#include <boost/algorithm/string/trim.hpp>

namespace ui {

WindowFavorites::WindowFavorites():
    m_editState(EDIT_NONE),
    m_confirmRemove(-1),
	ListView(display::TYPE_FAVORITES, "favorites", true)
{
	updateTitle();
    set_name("favorites");

    m_bindings['c'] = std::bind(&WindowFavorites::connect, this, true);
    m_bindings['C'] = std::bind(&WindowFavorites::connect, this, false);
    m_bindings['d'] = std::bind(&WindowFavorites::rmfav, this, true);
    m_bindings['D'] = std::bind(&WindowFavorites::rmfav, this, false);
    m_bindings['e'] = std::bind(&WindowFavorites::edit, this);
    m_bindings['n'] = std::bind(&WindowFavorites::add_new, this);
    m_bindings[' '] = std::bind(&WindowFavorites::toggle_connect, this);

    insert_column(new display::Column("Auto", 5, 5, 5));
    insert_column(new display::Column("Nick", 10, 15, 20));
    insert_column(new display::Column("Address", 10, 30, 45));
    insert_column(new display::Column("Name", 10, 25, 200));

    auto favhubs = FavoriteManager::getInstance()->getFavoriteHubs();
    for(const auto& f: favhubs)
        on(FavoriteManagerListener::FavoriteAdded(), f);

    m_state = display::STATE_NO_ACTIVITY;
    FavoriteManager::getInstance()->addListener(this);
}

void WindowFavorites::complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_) {

}

void WindowFavorites::handleEscape() {
	m_newFav = nullptr;
	m_editState = EDIT_NONE;
}

void WindowFavorites::handleMove(int prevPos, int diff) {
	utils::slide(FavoriteManager::getInstance()->getFavoriteHubs(), prevPos, diff);
}

void WindowFavorites::updateTitle() {
	set_title("Favorite hubs: " + utils::to_string(FavoriteManager::getInstance()->getFavoriteHubs().size()));
}

void WindowFavorites::toggle_connect()
{
    int row = get_selected_row();
    if(row == -1)
        return;

    auto entry = find_entry(row);
    entry->setConnect(!entry->getConnect());
    FavoriteManager *favman = FavoriteManager::getInstance();
    favman->save();

    set_text(0, row, entry->getConnect());
}

void WindowFavorites::add_new()
{
    m_newFav = new FavoriteHubEntry();
	setInsertMode(true);
    handle_line("");
}

FavoriteHubEntryPtr WindowFavorites::find_entry(int row) {
    auto& favhubs = FavoriteManager::getInstance()->getFavoriteHubs();
	auto p = std::find_if(favhubs.begin(), favhubs.end(), [=](const FavoriteHubEntryPtr& aFav) {
		return aFav->getServerStr() == get_text(2, row);
	});

	return p != favhubs.end() ? *p : nullptr;
}

void WindowFavorites::edit()
{
    int row = get_selected_row();
    if(row == -1)
        return;

    auto entry = find_entry(row);
    m_editFav = entry;
	m_newFav = new FavoriteHubEntry();
	setInsertMode(true);
    handle_line("");
}

void WindowFavorites::set_promt_text(const std::string &text)
{
	m_input.setText(text);
}

void WindowFavorites::handle_line(const std::string &line)
{
    /* if we are confirming hub removal */
    if(m_confirmRemove != -1) {
        if(line == "y")
            rmfav(false);
        set_prompt();
		setInsertMode(false);
        return;
    }

	auto listProfiles = [] {
		auto profiles = ShareManager::getInstance()->getProfiles();

		string ret;
		for (const auto& p : profiles) {
			ret += p->getPlainName() + "|";
		}

		ret.pop_back();
		return ret;
	};

    string questions[] = { 
        "Hub address?",
        "Hub name?",
        "Hub description?",
		"Share profile (" + listProfiles() + ")?",
        "Your nick?",
        "Your description?",
        "Hub password?",
        "Autoconnect (y/n)?",
        ""
    };

    /* ask one question at a time */
    m_editState++;

	optional<ProfileToken> token = nullptr;
	ShareProfilePtr profile = nullptr;
    switch(m_editState) {
        case EDIT_START:
			if (m_editFav)
				set_promt_text(m_editFav->getServerStr());
            break;
        case SERVER_URL:
			if (line.empty()) {
				m_editState--;
				break;
			}

			m_newFav->setServerStr(boost::trim_copy(line));
			if (m_editFav) {
				set_promt_text(m_editFav->getName());
			}
            break;
        case SERVER_NAME:
        {
			m_newFav->setName(line);
			if (m_editFav)
				set_promt_text(m_editFav->getDescription());
            break;}
        case SERVER_DESCRIPTION:
			m_newFav->setDescription(line);
			if (m_newFav->isAdcHub()) {
				if (m_editFav)
					set_promt_text(m_editFav->getShareProfile() ? m_editFav->getShareProfile()->getPlainName() : "");
			} else {
				m_newFav->setShareProfile(ShareManager::getInstance()->getShareProfile(SETTING(DEFAULT_SP)));
				if (m_editFav)
					set_promt_text(m_editFav->get(HubSettings::Nick));
				m_editState++;
			}
            break;
		case SHARE_PROFILE:
		{
			token = ShareManager::getInstance()->getProfileByName(line);
			if (!token) {
				m_editState--;
				break;
			}

			profile = ShareManager::getInstance()->getShareProfile(*token);
			m_newFav->setShareProfile(profile);
			if (m_editFav)
				set_promt_text(m_editFav->get(HubSettings::Nick));
			break;
		}
        case USER_NICK:
			m_newFav->get(HubSettings::Nick) = boost::trim_copy(line);
			if (m_editFav)
				set_promt_text(m_editFav->get(HubSettings::Description));
            break;
        case USER_DESCRIPTION:
			m_newFav->get(HubSettings::Description) = line;
			if (m_editFav)
				set_promt_text(m_editFav->getPassword());
            break;
        case SERVER_PASSWORD:
        {
			m_newFav->setPassword(line);
			if (m_editFav) {
				auto text = m_editFav->getConnect() == true ? "y" : "n";
				set_promt_text(text);
			}
            break;
        }
        case SERVER_AUTOCONNECT:
        {
			ScopedFunctor([&] { m_editFav = nullptr; m_newFav = nullptr; setInsertMode(false); m_editState = EDIT_NONE; set_prompt(); });

			m_newFav->setConnect((line == "y"));
            FavoriteManager *favman = FavoriteManager::getInstance();

			auto addresses = m_newFav->getServerStr();
			size_t pos = addresses.find(";");
			if (!favman->isUnique(pos != string::npos ? addresses.substr(0, pos) : addresses, m_editFav ? m_editFav->getToken() : 0)) {
				core::Log::get()->log("Duplicate favorite hub");
				return;
			}

			if (m_editFav) {
				auto oldAddress = m_editFav->getServerStr();
				m_editFav->setServerStr(m_newFav->getServerStr());
				m_editFav->setName(m_newFav->getName());
				m_editFav->setDescription(m_newFav->getDescription());
				m_editFav->setShareProfile(m_newFav->getShareProfile());

				m_editFav->get(HubSettings::Nick) = m_newFav->get(HubSettings::Nick);
				m_editFav->get(HubSettings::Description) = m_newFav->get(HubSettings::Description);

				m_editFav->setPassword(m_newFav->getPassword());
				m_editFav->setConnect(m_newFav->getConnect());

				try {
					int row = find_row(2, oldAddress);
					set_text(0, row, m_newFav->getConnect());
					set_text(1, row, m_newFav->get(HubSettings::Nick));
					set_text(2, row, m_newFav->getServerStr());
					set_text(3, row, m_newFav->getName());
				} catch (std::exception &e) {
					throw std::logic_error(std::string("Should not happen: ") + e.what());
				}
			} else {
				favman->addFavorite(m_newFav);
			}

            favman->save();
            return;
        }
        default:
            throw std::logic_error("should not be reached");
    }

	set_prompt(questions[m_editState-1]);
}

void WindowFavorites::connect(bool activate)
{
    int row = get_selected_row();
    if(row == -1)
        return;

    auto hub = find_entry(row);

	WindowHub::openWindow(hub->getServers()[0].first, hub->getShareProfile()->getToken(), activate);

	//events::emit("command connect", hub->getServers()[0], hub->getShareProfile(), hub->get(HubSettings::Nick),
    //        hub->getPassword(), hub->getDescription(), activate);
}

void WindowFavorites::rmfav(bool confirm)
{
    int row = get_selected_row();
    if(row == -1)
        return;

    if(confirm) {
        m_confirmRemove = get_current();
        m_prompt = "Really remove (y/n)?";
		setInsertMode(true);
        return;
    }

    FavoriteManager::getInstance()->removeFavorite(find_entry(row));
    m_confirmRemove = -1;
	updateTitle();
}

void WindowFavorites::on(FavoriteManagerListener::FavoriteAdded, const FavoriteHubEntryPtr& entry) noexcept {
    int row = insert_row();
    set_text(0, row, entry->getConnect());
	set_text(1, row, entry->get(HubSettings::Nick));
    set_text(2, row, entry->getServerStr());
    set_text(3, row, entry->getName());
	updateTitle();
}

void WindowFavorites::on(FavoriteManagerListener::FavoriteRemoved, const FavoriteHubEntryPtr& entry) noexcept
{
	delete_row(2, entry->getServerStr());
}

WindowFavorites::~WindowFavorites()
{
}


std::string WindowFavorites::get_infobox_line(unsigned int n)
{
    auto entry = find_entry(get_selected_row());
	if (!entry)
		return "";

    std::stringstream ss;
    switch(n) {
        case 1:
            ss << "%21Hub name:%21 " << entry->getName();
            break;
        case 2:
			ss << "%21Nick:%21 " << std::left << std::setw(20) << entry->get(HubSettings::Nick)
                << " %21Description:%21 " << entry->get(HubSettings::Description);
            break;
        case 3:
        {
            std::string passwd(entry->getPassword().length(), '*');
            ss << "%21Password:%21 " << std::left << std::setw(17) 
                << (passwd.empty()?"no password":passwd)
				<< "%21Share profile:%21 " << entry->getShareProfile()->getPlainName();
            break;
        }
        case 4:
            ss << "%21Hub description:%21 " << entry->getDescription();
            break;
		//case 5:
		//	ss << "%21Share profile:%21 " << entry->getShareProfile()->getPlainName();
		//	break;
            
    }
    return ss.str();
}

} // namespace ui
