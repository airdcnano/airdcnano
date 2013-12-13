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

#include <functional>
#include <core/log.h>
#include <utils/utils.h>
#include <ui/window_sharebrowser.h>
#include <client/ClientManager.h>
#include <client/DirectoryListingManager.h>
#include <client/QueueManager.h>
#include <core/events.h>
#include <input/completion.h>

namespace ui {

WindowShareBrowser::WindowShareBrowser(DirectoryListing* aList, const std::string& aDir, const std::string& aXML) :
	dl(aList),
	m_path(aDir), ListView(display::TYPE_LISTBROWSER, "list" + aList->getHintedUser().user->getCID().toBase32(), true)
{
	m_infoboxHeight = 2;

	insert_column(new display::Column("Type"));
	insert_column(new display::Column("Name", 10, 15, 50));
	insert_column(new display::Column("Size", 10, 15, 20));
	insert_column(new display::Column("Date", 6, 6, 12));
	insert_column(new display::Column("Dupe", 4, 4, 5));

	m_bindings[KEY_LEFT] = [&] { loadDirectory(Util::getNmdcParentDir(m_path)); };
	m_bindings[KEY_RIGHT] = [&] { handle_line(""); };


	// download
	m_bindings['d'] = std::bind(&WindowShareBrowser::download, this, SETTING(DOWNLOAD_DIRECTORY));

	// download to..
	m_bindings['D'] = std::bind(&WindowShareBrowser::set_property, this, PROP_DOWNLOAD);

	m_bindings['r'] = std::bind(&WindowShareBrowser::reload, this, false);
	m_bindings['R'] = std::bind(&WindowShareBrowser::reload, this, true);
	m_bindings['n'] = std::bind(&WindowShareBrowser::set_property, this, PROP_FILTER_NAME);

	setInsertMode(false);
	updateTitles();

	aList->addListener(this);
    try {
		if (dl->getPartialList()) {
			dl->addPartialListTask(aDir, aXML, false, true);
		} else {
			dl->addFullListTask(aDir);
		}
    } catch(Exception &e) {
		core::Log::get()->log("Cannot open list '" + aDir + "'" +
			" for user " + ClientManager::getInstance()->getFormatedNicks(aList->getHintedUser()));
    }
}

void WindowShareBrowser::complete(const std::vector<std::string>& aArgs, int /*pos*/, std::vector<std::string>& suggest_, bool& appendSpace_) {
	if (m_property == PROP_DOWNLOAD) {
		if (!aArgs[0].empty()) {
			input::Completion::getDiskPathSuggestions(aArgs[0], suggest_);
		} else {
			suggest_ = SettingsManager::getInstance()->getHistory(SettingsManager::HISTORY_DIR);
		}

		appendSpace_ = false;
	}
}

void WindowShareBrowser::set_property(Property property) {
	m_property = property;
	setInsertMode(true);
	const char *text[] = {
		"",
		"Download to:",
		"Name?"
	};
	m_prompt = text[m_property];
}

void WindowShareBrowser::reload(bool all) {
	loadDirectory(m_path, true);
}

string getTargetBrowser(const string& aTarget) {
	auto target = aTarget.empty() ? SETTING(DOWNLOAD_DIRECTORY) : aTarget;
	if (!target.empty() && target.back() != '/')
		target += "/";
	return target;
}

void WindowShareBrowser::download(const std::string& aPath) {
	int row = get_selected_row();
	if (row == -1)
		return;

	auto name = get_text(1, row);

	auto target = getTargetBrowser(aPath);

	auto curDir = dl->findDirectory(m_path);
	if (!curDir)
		return;

	try {
		if (!isDirectorySelected()) {
			auto f = find_if(curDir->files.begin(), curDir->files.end(), [&](const DirectoryListing::File* aFile) { return aFile->getName() == name; });
			if (f == curDir->files.end())
				return;

			QueueManager::getInstance()->createFileBundle(
				target + name,
				0, (*f)->getTTH(),
				dl->getHintedUser(), (*f)->getRemoteDate(), 0, QueueItemBase::DEFAULT);
			set_prompt_timed("Queued " + target + name);
		} else {
			auto d = find_if(curDir->directories.begin(), curDir->directories.end(), [&](const DirectoryListing::Directory::Ptr& aDir) { return aDir->getName() == name; });
			if (d == curDir->directories.end())
				return;

			DirectoryListingManager::getInstance()->addDirectoryDownload((*d)->getPath(), name,
				dl->getHintedUser(), target, TargetUtil::TARGET_PATH, NO_CHECK);
			set_prompt_timed("Queued " + (*d)->getPath());
		}

		if (!aPath.empty())
			SettingsManager::getInstance()->addToHistory(target, SettingsManager::HISTORY_DIR);
	} catch (const Exception &e) {
		set_prompt_timed("Error downloading the file: " + e.getError());
	}
}

void WindowShareBrowser::updateTitles() {
	auto nicks = ClientManager::getInstance()->getFormatedNicks(dl->getHintedUser());
	set_title("Browsing user " + nicks + " (" + ClientManager::getInstance()->getFormatedHubNames(dl->getHintedUser()) + ")");
	set_name("List:" + nicks);
}

WindowShareBrowser::~WindowShareBrowser() {
	dl->removeListener(this);
}

void WindowShareBrowser::handle_line(const std::string &line) {
	if (!getInsertMode()) {
		if (!isDirectorySelected())
			return;

		int row = get_selected_row();
		if (row == -1)
			return;

		loadDirectory(m_path + get_text(1, row) + "\\");
		return;
	}

	if (m_property == PROP_DOWNLOAD) {
		download(line);
	} else if (m_property == PROP_FILTER_NAME) {
		if (line.empty()) {
			nameFilter.reset();
		} else {
			nameFilter.reset(new StringMatch);
			nameFilter->pattern = line;
			nameFilter->setMethod(StringMatch::PARTIAL);
			nameFilter->prepare();
		}

		auto d = dl->findDirectory(m_path);
		if (d)
			insertItems(d, nullptr);
	}

	setInsertMode(false);
	set_prompt("");
	m_property = PROP_NONE;
}

bool WindowShareBrowser::isDirectorySelected() {
	int row = get_selected_row();
	if (row == -1)
		return false;

	return get_text(0, row) == "d";
}

void WindowShareBrowser::loadDirectory(const std::string& aDir, bool reload) {
	delete_all();
	nameFilter.reset();

	auto d = dl->findDirectory(aDir);
	if (!d) {
		set_prompt_timed("Directory " + aDir + " not found");
		return;
	}

	updateItems(d);

	if (reload || (!d->isComplete() && !d->getLoading())) {
		if (dl->getIsOwnList()) {
			//d->setLoading(true);
			//dl->addPartialListTask(Util::emptyString, d->getPath(), aReload == RELOAD_ALL);
		} else if (dl->getUser()->isOnline()) {
			try {
				QueueManager::getInstance()->addList(dl->getHintedUser(), QueueItem::FLAG_PARTIAL_LIST | QueueItem::FLAG_CLIENT_VIEW, d->getPath());
				d->setLoading(true);
				//callAsync([]= ctrlTree.updateItemImage(ii);
				set_prompt_timed((reload ? "Reloading " : "Downloading ") + Util::toAdcFile(aDir) + "...");
			} catch (const QueueException& e) {
				set_prompt_timed(e.getError());
			}
		} else {
			set_prompt_timed(STRING(USER_OFFLINE));
		}
	}
}

void WindowShareBrowser::updateItems(const DirectoryListing::Directory::Ptr& d) {
	/*ctrlFiles.list.SetRedraw(FALSE);
	updating = true;*/
	optional<string> selectedName;

	if (d->getPath() == Util::getNmdcParentDir(m_path)) {
		selectedName = Util::getNmdcLastDir(m_path);
	}

	m_path = d->getPath();

	//ctrlFiles.onListChanged(false);
	insertItems(d, selectedName);

	//updating = false;
	//updateStatus();
}

void WindowShareBrowser::insertItems(const DirectoryListing::Directory::Ptr& aDir, const optional<string>& selectedName) {
	delete_all();

	int selectedPos = -1;

	auto getDupeText = [&](int aDupe) {
		switch (aDupe) {
			case SHARE_DUPE: return "S";
			case SHARE_QUEUE_DUPE: return "SQ";
			case QUEUE_DUPE: return "Q";
		}

		return "";
	};

	auto dirs = aDir->directories;
	sort(dirs.begin(), dirs.end(), DirectoryListing::Directory::Sort());
	for (const auto& d : dirs) {
		if (nameFilter && !nameFilter->match(d->getName()))
			continue;

		auto row = insert_row();
		set_text(0, row, "d");
		set_text(1, row, utils::escape(d->getName()));
		set_text(2, row, Util::formatBytes(d->getTotalSize(false)));
		set_text(3, row, Util::getDateTime(d->getRemoteDate()));
		set_text(4, row, getDupeText(d->getDupe()));

		if (selectedName && d->getName() == *selectedName) {
			selectedPos = row;
		}
	}

	auto files = aDir->files;
	sort(files.begin(), files.end(), DirectoryListing::File::Sort());
	for (const auto& f : files) {
		if (nameFilter && !nameFilter->match(f->getName()))
			continue;

		auto row = insert_row();
		set_text(0, row, "f");
		set_text(1, row, utils::escape(f->getName()));
		set_text(2, row, Util::formatBytes(f->getSize()));
		set_text(3, row, Util::getDateTime(f->getRemoteDate()));
		set_text(4, row, getDupeText(f->getDupe()));
	}

	if (selectedPos > 0) {
		scroll_list(selectedPos);
	}
}

void WindowShareBrowser::on(DirectoryListingListener::LoadingFinished, int64_t aStart, const string& aDir, bool reloadList, bool changeDir, bool loadInGUIThread) noexcept{
	if (!changeDir && !reloadList && !AirUtil::isParentOrExact(aDir, m_path)) {
		return;
	}

	if (!dl->getIsOwnList() && SETTING(DUPES_IN_FILELIST))
		dl->checkShareDupes();

	callAsync([=] {
		loadDirectory(aDir);
		dl->setWaiting(false);
		set_prompt_timed(Util::toAdcFile(aDir) + " loaded");
	});
}

void WindowShareBrowser::on(DirectoryListingListener::LoadingFailed, const string& aReason) noexcept{
	if (!dl->getClosing()) {
		/*callAsync([=] {
			updateStatus(Text::toT(aReason));
			if (!dl->getPartialList()) {
				PostMessage(WM_CLOSE, 0, 0);
			} else {
				changeWindowState(true);
			}
		});*/
	}
}

void WindowShareBrowser::on(DirectoryListingListener::LoadingStarted, bool changeDir) noexcept{
	callAsync([=] {
		if (changeDir) {
			//DisableWindow(false);
		} else {
			//changeWindowState(false);
			//ctrlStatus.SetText(0, CTSTRING(LOADING_FILE_LIST));
		}
		dl->setWaiting(false);
	});
}
void WindowShareBrowser::on(DirectoryListingListener::QueueMatched, const string& aMessage) noexcept{

}

void WindowShareBrowser::on(DirectoryListingListener::Close) noexcept{

}
void WindowShareBrowser::on(DirectoryListingListener::SearchStarted) noexcept{

}
void WindowShareBrowser::on(DirectoryListingListener::SearchFailed, bool timedOut) noexcept{

}
void WindowShareBrowser::on(DirectoryListingListener::ChangeDirectory, const string& aDir, bool isSearchChange) noexcept{
	callAsync([=] { loadDirectory(aDir); });
	/*if (isSearchChange) {
		callAsync([=] { updateStatus(TSTRING_F(X_RESULTS_FOUND, dl->getResultCount())); });
		findSearchHit(true);
	}*/

	//changeWindowState(true);
}
void WindowShareBrowser::on(DirectoryListingListener::UpdateStatusMessage, const string& aMessage) noexcept{
	callAsync([=] { set_prompt_timed(STRING(USER_OFFLINE)); });
}
void WindowShareBrowser::on(DirectoryListingListener::RemovedQueue, const string& aDir) noexcept{
	callAsync([=] { set_prompt_timed(aDir + " was removed from queue"); });
}
void WindowShareBrowser::on(DirectoryListingListener::SetActive) noexcept{
	//set_current();
}
void WindowShareBrowser::on(DirectoryListingListener::HubChanged) noexcept{

}


std::string WindowShareBrowser::get_infobox_line(unsigned int n) {
	int row = get_selected_row();
	if (row == -1)
		return "";

	auto name = get_text(1, row);

	std::stringstream ss;
	switch (n) {
	case 1:
	{
		ss << utils::escape(Util::toAdcFile(m_path) + name + (isDirectorySelected() ? "/" : ""));
		break;
	}
	/*case 2:
		ss << "%21Size:%21 " << std::left << std::setw(9)
			<< Util::formatBytes(result->getSize())
			<< " %21Slots:%21 " << result->getFreeSlots() << "/" << result->getSlots()
			<< " %21TTH:%21 " << (result->getType() == SearchResult::TYPE_FILE ? result->getTTH().toBase32() : "-");
		break;
	case 3:
		ss << "%21Hub:%21 " << std::left << std::setw(18)
			<< ClientManager::getInstance()->getFormatedHubNames(result->getUser())
			<< " %21Date:%21 " << Util::getDateTime(result->getDate());
		break;
	case 4:
	{
		ss << utils::escape(Util::toAdcFile(result->getPath()));
		break;
	}*/
	}
	return ss.str();
}

} // namespace ui
