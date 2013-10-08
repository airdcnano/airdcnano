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

#ifndef _WINDOWFAVORITES_H_
#define _WINDOWFAVORITES_H_

#include <client/stdinc.h>
#include <client/DCPlusPlus.h>
#include <client/FavoriteManager.h>
#include <display/listview.h>
#include <vector>
#include <map>

using namespace dcpp;

namespace ui {

class WindowFavorites:
    public display::ListView,
    public FavoriteManagerListener
{
public:
    WindowFavorites();
    ~WindowFavorites();
    void toggle_connect();
    void add_new();
    void edit();

    void handle_line(const std::string &line);

    /** Connect to a hub
     * @param activate whether to activate the opened hub window */
    void connect(bool activate);
    void rmfav(bool confirm);

    void set_promt_text(const std::string &text);

    FavoriteHubEntryPtr find_entry(int row);

    std::string get_infobox_line(unsigned int n);

    void update();
	void on(FavoriteManagerListener::FavoriteAdded, const FavoriteHubEntryPtr& entry) noexcept;
	void on(FavoriteManagerListener::FavoriteRemoved, const FavoriteHubEntryPtr& entry) noexcept;
private:
    enum EditState {
        EDIT_NONE,
        EDIT_START,
        SERVER_URL,
        SERVER_NAME,
        SERVER_DESCRIPTION,
		SHARE_PROFILE,
        USER_NICK,
        USER_DESCRIPTION,
        SERVER_PASSWORD,
        SERVER_AUTOCONNECT
    };
    FavoriteHubEntryPtr m_editFav;
    int m_editState;
    int m_confirmRemove;
	FavoriteHubEntryPtr m_newFav;

	void updateTitle();
};

} // namespace ui

#endif // _WINDOWFAVORITES_H_
