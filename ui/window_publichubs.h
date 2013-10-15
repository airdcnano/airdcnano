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


#ifndef _WINDOWHUBS_H_
#define _WINDOWHUBS_H_

#include <vector>
#include <client/stdinc.h>
#include <client/DCPlusPlus.h>
#include <client/FavoriteManager.h>
#include <core/log.h>
#include <utils/mutex.h>
#include <utils/lock.h>
#include <display/listview.h>

namespace ui {

class WindowHubs:
    public display::ListView,
    public FavoriteManagerListener
{
public:
    WindowHubs();

    void connect();
    void favorite();
    void download();
    void show_entries();

    void handle_line(const std::string &line);

    std::string get_infobox_line(unsigned int n);

    void on(DownloadStarting, const std::string&) noexcept;
    void on(DownloadFailed, const std::string&) noexcept;
    void on(DownloadFinished, const std::string&, bool) noexcept;
	void on(LoadedFromCache, const string& l, const string& d) noexcept;
    /** Destructor. */
    ~WindowHubs();

	void complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_);
	void handleEscape();
private:
    enum Property {
        PROP_NONE,
        PROP_DESCRIPTION,
        PROP_ADDRESS,
        PROP_HUBLIST,
        PROP_USERS,
        PROP_MINSHARE,
        PROP_MAXSHARE
    };

    std::vector<HubEntry> m_hubs;
    int m_hublist;
    utils::Mutex m_entryLock;

    // hub filters
    std::vector<std::string> m_words;
    std::string m_addressFilter;
    int64_t m_minShare;
    int64_t m_maxShare;
    int64_t m_minUsers;
    bool m_shutdown;

    void set_property(Property property);
    Property m_property;

    /** Returns true if search result matches current filters. */
    bool matches(const HubEntry &entry);

    utils::Mutex m_mutex;
	void downloadFinished(bool cached, const std::string list) noexcept;
};

} // namespace ui

#endif // _WINDOWHUBS_H_
