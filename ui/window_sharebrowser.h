/* vim:set ts=4 sw=4 sts=4 et cindent: */
/*
 * AirDC++ nano
 * Copyright © 2013 maksis@adrenaline-network.com
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


#ifndef _WINDOWSHAREBROWSER_H_
#define _WINDOWSHAREBROWSER_H_

#include <client/stdinc.h>
#include <client/DirectoryListing.h>
#include <client/DirectoryListingListener.h>

#include <display/listview.h>

using namespace dcpp;

namespace ui {

typedef DirectoryListing::Directory Dir;
typedef DirectoryListing::File File;

class WindowShareBrowser:
	public display::ListView, public DirectoryListingListener
{
public:
	WindowShareBrowser(DirectoryListing* aList, const std::string& aDir, const std::string& aXML);

	~WindowShareBrowser();

	/** Handle user input. */
	void handle_line(const std::string &line);
	void complete(const std::vector<std::string>& aArgs, int pos, std::vector<std::string>& suggest_, bool& appendSpace_);
private:
	enum Property {
		PROP_NONE,
		PROP_DOWNLOAD
	};
	void set_property(Property property);
	Property m_property = PROP_NONE;

    DirectoryListing* dl;
    std::string m_path;

	void download(const std::string& aPath);
	bool isDirectorySelected();
	void updateTitles();
	void loadDirectory(const std::string& aDir);
	void updateItems(const DirectoryListing::Directory::Ptr& d);
	void insertItems(const DirectoryListing::Directory::Ptr& d, const optional<string>& selectedName);

	void on(DirectoryListingListener::LoadingFinished, int64_t aStart, const string& aDir, bool reloadList, bool changeDir, bool loadInGUIThread) noexcept;
	void on(DirectoryListingListener::LoadingFailed, const string& aReason) noexcept;
	void on(DirectoryListingListener::LoadingStarted, bool changeDir) noexcept;
	void on(DirectoryListingListener::QueueMatched, const string& aMessage) noexcept;
	void on(DirectoryListingListener::Close) noexcept;
	void on(DirectoryListingListener::SearchStarted) noexcept;
	void on(DirectoryListingListener::SearchFailed, bool timedOut) noexcept;
	void on(DirectoryListingListener::ChangeDirectory, const string& aDir, bool isSearchChange) noexcept;
	void on(DirectoryListingListener::UpdateStatusMessage, const string& aMessage) noexcept;
	void on(DirectoryListingListener::RemovedQueue, const string& aDir) noexcept;
	void on(DirectoryListingListener::SetActive) noexcept;
	void on(DirectoryListingListener::HubChanged) noexcept;

	std::string get_infobox_line(unsigned int);
};

} // namespace ui

#endif // _WINDOWSHAREBROWSER_H_
