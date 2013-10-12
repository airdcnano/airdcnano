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

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <functional>
#include <display/manager.h>
#include <core/events.h>
#include <ui/window_hub.h>
#include <ui/window_publichubs.h>
#include <utils/utils.h>
#include <utils/strings.h>

namespace ui {

WindowHubs::WindowHubs():
    m_hublist(0),
    m_minShare(0),
    m_maxShare(0),
    m_minUsers(0),
    m_shutdown(false),
    m_property(PROP_NONE),
	ListView(display::TYPE_HUBLIST, "hublist")
{
    FavoriteManager::getInstance()->addListener(this);

    set_title("Public hubs (press d to download the list)");
    set_name("hublist");

    insert_column(new display::Column("Users", 6, 6, 6));
    insert_column(new display::Column("Min share", 6, 10, 15));
    insert_column(new display::Column("Address", 10, 25, 40));
    insert_column(new display::Column("Name", 10, 25, 200));

    m_bindings['f'] = std::bind(&WindowHubs::favorite, this);
    m_bindings['c'] = std::bind(&WindowHubs::connect, this);
//    m_bindings['s'] = std::bind(&WindowHubs::sort_items, this);
    m_bindings['d'] = std::bind(&WindowHubs::download, this);

    m_bindings['s'] = std::bind(&WindowHubs::set_property, this, PROP_DESCRIPTION);
    m_bindings['a'] = std::bind(&WindowHubs::set_property, this, PROP_ADDRESS);
    m_bindings['h'] = std::bind(&WindowHubs::set_property, this, PROP_HUBLIST);
    m_bindings['u'] = std::bind(&WindowHubs::set_property, this, PROP_USERS);
    m_bindings['n'] = std::bind(&WindowHubs::set_property, this, PROP_MINSHARE);
    m_bindings['m'] = std::bind(&WindowHubs::set_property, this, PROP_MAXSHARE);
}

void WindowHubs::complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_) {

}

void WindowHubs::connect()
{
    int row = get_selected_row();
    if(row == -1)
        return;

	auto address = get_text(2, row);
//#if 0
    /*std::string address = get_text(2, row);
    display::Manager::iterator it = display::Manager::get()->find(address);

    if(it != display::Manager::get()->end()) {
        if((*it)->get_type() == display::TYPE_HUBWINDOW) {
            ui::WindowHub *hub = static_cast<ui::WindowHub*>(*it);
            if(!hub->get_client()->isConnected())
                hub->connect();
        }
    }
    else {
        ui::WindowHub *hub = new ui::WindowHub(address);
        hub->connect(address, "", "", "");
        display::Manager::get()->push_back(hub);
    }*/

	ui::WindowHub::openWindow(address, SETTING(DEFAULT_SP), true);
   // display::Manager::get()->set_current(it);
//#endif
}

void WindowHubs::download()
{
    utils::Lock l(m_mutex);

    FavoriteManager *man = FavoriteManager::getInstance();
    man->setHubList(m_hublist);
}

void WindowHubs::favorite()
{
    utils::Lock l(m_mutex);

    int row = get_selected_row();
    if(row == -1)
        return;

    std::string address = get_text(2, row);
    if(FavoriteManager::getInstance()->getFavoriteHubEntry(address)) {
        core::Log::get()->log("Hub already exists as a favorite");
    }
    else {
        FavoriteHubEntryPtr entry;
        entry->setServerStr(address);
        FavoriteManager::getInstance()->addFavorite(entry);
        FavoriteManager::getInstance()->save();
    }
}

void WindowHubs::set_property(Property property)
{
    utils::Lock l(m_mutex);

    m_property = property;
    const char *properties [] = {
        "",
        "Hub description?",
        "Hub address?",
        "Hublist number?",
        "Users?",
        "Min share?",
        "Max share?"
    };
    m_prompt = properties[m_property];
	setInsertMode(true);

    events::emit("window updated", static_cast<display::Window*>(this));
}

void WindowHubs::handle_line(const std::string &line)
{
    utils::Lock l(m_mutex);

    std::string param = line;
    switch(m_property) {
        case PROP_DESCRIPTION:
            m_words.clear();
            strings::split(utils::tolower(param), " ", std::back_inserter(m_words));
            break;
        case PROP_ADDRESS:
            m_addressFilter = utils::tolower(param);
            break;
        case PROP_HUBLIST:
            m_hublist = utils::to<int>(param);
            break;
        case PROP_USERS:
            m_minUsers = utils::to<int64_t>(param);
            break;
        case PROP_MINSHARE:
        case PROP_MAXSHARE:
        {
            int64_t size = utils::to<unsigned long>(param);
            size *= 1024*1024*1024;

            if(m_property == PROP_MINSHARE)
                m_minShare = size;
            else
                m_maxShare = size;
            /*
            std::ostringstream oss;
            oss << "min:" << m_minShare << "=" << Util::formatBytes(m_minShare)
                << ", max:" << m_maxShare << "=" << Util::formatBytes(m_maxShare);
            core::Log::get()->log(oss.str());*/
            break;
        }
        default:
            throw std::logic_error("WindowHubs::handle_line(): Should not happen");
    }

	setInsertMode(false);
    show_entries();
    m_property = PROP_NONE;
}

void WindowHubs::show_entries()
{
    //utils::Lock l(m_mutex);

    delete_all();
   // utils::Lock lock(m_entryLock);

	for (const auto& h: m_hubs) {
        if(matches(h)) {
            int row = insert_row();
            set_text(0, row, h.getUsers());
			set_text(1, row, Util::formatBytes(h.getMinShare()));
			set_text(2, row, h.getServer());
			set_text(3, row, h.getName());
        }
    }

    set_title("Public hubs: Showing " + utils::to_string(get_size()) + " of " + utils::to_string(m_hubs.size()) + " hubs");

    events::emit("window updated", static_cast<display::Window*>(this));
}

bool WindowHubs::matches(const HubEntry &entry)
{
    if(m_minShare && entry.getMinShare() < m_minShare)
        return false;

    if(m_maxShare && entry.getMinShare() > m_maxShare)
        return false;

    if(m_minUsers && m_minUsers < entry.getMaxUsers())
        return false;

    std::string description = utils::tolower(entry.getDescription());

    if(m_words.size() && !utils::find_in_string(description, m_words.begin(), m_words.end())) {
        return false;
    }

    if(m_addressFilter.size() && entry.getServer().find(m_addressFilter) == std::string::npos)
        return false;

    return true;
}

void WindowHubs::on(DownloadStarting, const std::string &list)
    noexcept
{
    //utils::Lock l(m_mutex);

    std::ostringstream oss;
    oss << "Public hubs: Downloading list " << m_hublist << "/"
        << FavoriteManager::getInstance()->getPublicHubs().size()
        << " - " << list;
    set_title(oss.str());
    events::emit("window updated", static_cast<display::Window*>(this));
}

void WindowHubs::on(DownloadFailed, const std::string &list)
    noexcept
{
    utils::Lock l(m_mutex);

    set_title("Public hubs: Failed " + list);
    events::emit("window updated", static_cast<display::Window*>(this));
}

void WindowHubs::on(DownloadFinished, const std::string &list, bool) noexcept {
	downloadFinished(false, list);
}

void WindowHubs::on(LoadedFromCache, const string& l, const string& d) noexcept{
	downloadFinished(true, l);
}

void WindowHubs::downloadFinished(bool cached, const std::string list) noexcept{
	utils::Lock l(m_mutex);

	std::ostringstream oss;
	oss << "Public hubs list " << m_hublist << "/"
		<< FavoriteManager::getInstance()->getPublicHubs().size()
		<< " - " << list;
	set_title(oss.str());

	m_entryLock.lock();
	m_hubs = FavoriteManager::getInstance()->getPublicHubs();
	m_entryLock.unlock();

	show_entries();
}

WindowHubs::~WindowHubs()
{
    m_shutdown = true;
    FavoriteManager::getInstance()->removeListener(this);
}

std::string WindowHubs::get_infobox_line(unsigned int n)
{
    utils::Lock l(m_mutex);

    using std::bind;
    using std::placeholders::_1;

    std::vector<HubEntry>::iterator it = std::find_if(m_hubs.begin(), m_hubs.end(),
        bind(std::equal_to<std::string>(),
            bind(&HubEntry::getServer, _1),
            get_text(2, get_selected_row())));

    if(it == m_hubs.end())
        return "[BUG in WindowHubs::get_infobox_line]";

    HubEntry entry = *it;

    std::ostringstream oss;
    switch(n) {
        case 1:
            oss << entry.getDescription();
            break;
        case 2:
            oss << "%21Country:%21 " << std::left << std::setw(24) << entry.getCountry()
                << "%21Max hubs:%21 " << std::setw(5) << entry.getMaxHubs()
                << "%21Users/max users:%21 " << entry.getUsers() << "/" << entry.getMaxUsers();
            break;
        case 3:
            oss << "%21Reliability:%21 " << std::left << std::fixed << std::setprecision(2)
                << entry.getReliability()
                << " %21Rating:%21 " << entry.getRating();
            break;
        case 4:
            oss << "%21Shared:%21 " << std::left << std::setw(10) << Util::formatBytes(entry.getShared())
                << "%21Min slots:%21 " << entry.getMinSlots();
            break;
    }
    return oss.str();
}

} // namespace ui
