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

#include <iostream>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <functional>

#include <utils/utils.h>
#include <core/log.h>
#include <core/events.h>
#include <display/screen.h>
#include <ui/manager.h>
#include <ui/window_hub.h>
#include <ui/window_favorites.h>
#include <ui/window_transfers.h>
#include <ui/window_log.h>
#include <ui/window_privatemessage.h>
#include <ui/window_publichubs.h>
#include <ui/window_search.h>
#include <ui/window_sharebrowser.h>
#include <ui/window_queue.h>
#include <ui/status_clock.h>
#include <ui/status_user.h>
#include <ui/status_windowinfo.h>
#include <ui/status_windowlist.h>
#include <ui/status_hash.h>

#include <client/ConnectivityManager.h>
#include <client/DirectoryListingManager.h>
#include <client/FavoriteManager.h>
#include <client/UpdateManager.h>

namespace ui {

Manager::Manager():
    m_lastDraw(utils::get_millisecs())
{
   events::add_listener("client created",
            std::bind(&Manager::create_windows, this));

    events::add_listener("timer started",
            std::bind(&Manager::init_statusbar, this));

    events::add_listener_first("window updated",
        std::bind(&Manager::redraw_screen, this));

    events::add_listener_first("statusbar updated",
        std::bind(&Manager::redraw_screen, this));
}

void Manager::redraw_screen()
{
    if(m_lastDraw+20 < utils::get_millisecs()) {
        display::Manager::get()->redraw();
        m_lastDraw = utils::get_millisecs();
    }
}

void Manager::init()
{
    display::Screen::initialize();
    display::Manager::create();
    display::Manager::get()->push_back(new ui::WindowLog());
    display::StatusBar::get()->add_item(new ui::StatusClock());
}

void Manager::create_windows()
{
    display::Manager *dm = display::Manager::get();
	if (SETTING(OPEN_PUBLIC))
		dm->push_back(new ui::WindowHubs());
	if (SETTING(OPEN_FAVORITE_HUBS))
		dm->push_back(new ui::WindowFavorites());
	if (SETTING(SHOW_TRANSFERVIEW))
		dm->push_back(new ui::WindowTransfers());
	if (SETTING(OPEN_QUEUE))
		dm->push_back(new ui::WindowQueue());

	ClientManager::getInstance()->addListener(this);
	DirectoryListingManager::getInstance()->addListener(this);
	TimerManager::getInstance()->addListener(this);
    MessageManager::getInstance()->addListener(this);

	try {
		ConnectivityManager::getInstance()->setup(true, true);
	} catch (const Exception& e) {
		core::Log::get()->log("ConnectivityManager::setup failure: " + e.getError());
	}

	UpdateManager::getInstance()->checkVersion(false);
	if (!Util::hasParam("-no-autoconnect"))
		FavoriteManager::getInstance()->autoConnect();

	//core::Log::get()->log("Client loaded");
}

void Manager::init_statusbar()
{
    display::StatusBar *m_statusbar = display::StatusBar::get();
    m_statusbar->add_item(new ui::StatusUser());
    m_statusbar->add_item(new ui::StatusWindowInfo());
    m_statusbar->add_item(new ui::StatusWindowList());
    m_statusbar->add_item(new ui::StatusHash());
}

Manager::~Manager() {
	ConnectivityManager::getInstance()->disconnect();

	ClientManager::getInstance()->removeListener(this);
	DirectoryListingManager::getInstance()->removeListener(this);
	TimerManager::getInstance()->removeListener(this);
    MessageManager::getInstance()->removeListener(this);
}

void Manager::on(ClientManagerListener::ClientCreated, Client* c) noexcept {
	auto mger = display::Manager::get();
	auto it = mger->find(display::TYPE_HUBWINDOW, c->getHubUrl());
	if (it == mger->end()) {
		ui::WindowHub *hub = new ui::WindowHub(c->getHubUrl());
		mger->push_back(hub);
		it = mger->end() - 1;
	}

	auto h = static_cast<ui::WindowHub*>(*it);
	h->callAsync([=] { h->handleCreated(); });
}

void Manager::on(DirectoryListingManagerListener::OpenListing, DirectoryListing* aList, const std::string& aDir, const std::string& aXML) noexcept{
    callAsync([=] {
        auto mger = display::Manager::get();
	    auto it = mger->find(display::TYPE_LISTBROWSER, aList->getUser()->getCID().toBase32());
	    if (it == mger->end()) {
		    auto dl = new ui::WindowShareBrowser(aList, aDir, aXML);
		    mger->push_back(dl);
	    }

	    mger->set_current(it);
    });
}

void Manager::on(TimerManagerListener::Second, uint64_t aTick) noexcept{
	if (aTick == lastUpdate)	// FIXME: temp fix for new TimerManager
		return;

	int64_t totalDown = Socket::getTotalDown();
	int64_t totalUp = Socket::getTotalUp();

	int64_t diff = (int64_t) ((lastUpdate == 0) ? aTick - 1000 : aTick - lastUpdate);
	int64_t updiff = totalUp - lastUp;
	int64_t downdiff = totalDown - lastDown;

	events::emit("bytes transferred", static_cast<int64_t>(downdiff * 1000LL / diff), static_cast<int64_t>(updiff * 1000LL / diff));

	SettingsManager::getInstance()->set(SettingsManager::TOTAL_UPLOAD, SETTING(TOTAL_UPLOAD) + updiff);
	SettingsManager::getInstance()->set(SettingsManager::TOTAL_DOWNLOAD, SETTING(TOTAL_DOWNLOAD) + downdiff);
	lastUpdate = aTick;
	lastUp = totalUp;
	lastDown = totalDown;
}

void Manager::on(MessageManagerListener::PrivateMessage, const ChatMessage& aMessage) noexcept{
    callAsync([=] { WindowPrivateMessage::onPrivateMessage(aMessage); });
}

} // namespace ui

