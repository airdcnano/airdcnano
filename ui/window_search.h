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

#ifndef _WINDOWSEARCH_H_
#define _WINDOWSEARCH_H_

#include <client/stdinc.h>
#include <client/SearchManagerListener.h>

#include <client/HintedUser.h>
#include <client/SearchQuery.h>
#include <client/SearchResult.h>

#include <display/listview.h>

using namespace dcpp;

namespace ui {

const unsigned int MIN_SEARCH = 2;

class WindowSearch:
    public display::ListView,
    public SearchManagerListener
{
public:
    WindowSearch(const std::string &str);

    /** Do a search. */
    void search(const std::string &str);

    /** Download selected file */
    void download(const std::string &path);

    void download_directory(const std::string &path);

    /** Handle user input. */
    void handle_line(const std::string &line);

    /** Displays search result on the screen if it matches the filters */
    void add_result(SearchResult *result);

    /** Decreases reference count of all added search results. */
    void free_results();

    void create_list();

    std::string get_infobox_line(unsigned int);

    SearchResultPtr get_result();
    HintedUser get_user() { return get_result()->getUser(); }

	void toggle_slots();

    enum Property {
        PROP_NONE,
        PROP_SEARCHFILTER,
        PROP_MINSIZE,
        PROP_MAXSIZE,
        PROP_EXTENSION,
        PROP_FILETARGET,
        PROP_DIRECTORYTARGET
    };
    void set_property(Property property);

    /** Destructor. */
    ~WindowSearch();

    /** Called when a search result is received. */
    void on(SearchManagerListener::SR, const SearchResultPtr& result) noexcept;
	void complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_, bool& appendSpace_);
	void handleEscape();
private:
    bool m_shutdown = false;
    Property m_property = PROP_NONE;
    int64_t m_lastSearch = 0;

    std::string m_searchStr;
	SearchResultList m_results;

    /** Returns true if search result matches current filters. */
    bool matches(const SearchResultPtr& result);
	void add_result(const SearchResultPtr& result);

	void handleGetList();
	void handleMatchQueue();
	std::string token;

	// search result filters
	bool m_freeSlots = false;
	unique_ptr<SearchQuery> curSearch;
	bool filtering = false;

	void updateTitle();
};

} // namespace ui

#endif // _WINDOWSEARCH_H_
