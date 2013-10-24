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

#ifndef _UIMANAGER_H_
#define _UIMANAGER_H_

#include <display/status_bar.h>
#include <display/window.h>
#include <display/input_window.h>
#include <utils/instance.h>
#include <utils/mutex.h>
#include <utils/lock.h>

#include <client/stdinc.h>
#include <client/DCPlusPlus.h>
#include <client/TimerManager.h>
#include <client/ClientManagerListener.h>
#include <client/DirectoryListingManagerListener.h>

#include <string>
#include <deque>

using namespace dcpp;

namespace ui {

typedef std::deque<display::Window*> Windows;

class Manager:
    public utils::Instance<ui::Manager>,
    public TimerManagerListener,
	public ClientManagerListener,
	public DirectoryListingManagerListener
{
public:
    Manager();

    void init();

    void main_loop();

    /** Open some windows. */
    void create_windows();

    /** Creates statusbar elements. */
    void init_statusbar();

    /** Redraws the screen. */
    void redraw_screen();

    /** Destructor. */
    ~Manager();
private:
	void on(ClientManagerListener::ClientCreated, Client* c) noexcept;
	void on(DirectoryListingManagerListener::OpenListing, DirectoryListing* aList, const std::string& aDir, const std::string& aXML) noexcept;
	void on(TimerManagerListener::Second, uint64_t) noexcept;

    utils::Mutex m_screenMutex;
    uint32_t m_lastDraw;

	int64_t lastUp = 0;
	int64_t lastDown = 0;
	uint64_t lastUpdate = 0;
};

} // namespace ui

#endif // _UIMANAGER_H_
