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

#include <functional>
#include <core/log.h>
#include <utils/utils.h>
#include <ui/window_sharebrowser.h>
#include <client/ClientManager.h>
#include <core/events.h>

namespace ui {

WindowShareBrowser::WindowShareBrowser(DirectoryListing* aList, const std::string& aDir, const std::string& aXML) :
	m_listing(aList),
	m_path(aDir), DirectoryWindow(display::TYPE_LISTBROWSER, aList->getUser()->getCID().toBase32())
{
	setInsertMode(false);
    set_title("Browsing user " + Util::listToString(ClientManager::getInstance()->getNicks(aList->getHintedUser())));
	set_name("List:" + Util::listToString(ClientManager::getInstance()->getNicks(aList->getHintedUser())));

	aList->addListener(this);
    try {
		if (m_listing->getPartialList()) {
			m_listing->addPartialListTask(aXML, aDir, false, false);
		} else {
			m_listing->addFullListTask(aDir);
		}
    } catch(Exception &e) {
		core::Log::get()->log("Cannot open list '" + aDir + "'" +
			" for user " + Util::listToString(ClientManager::getInstance()->getNicks(aList->getHintedUser())));
    }
}

WindowShareBrowser::~WindowShareBrowser() {
	m_listing->removeListener(this);
}

void WindowShareBrowser::on(DirectoryListingListener::LoadingFinished, int64_t aStart, const string& aDir, bool reloadList, bool changeDir, bool loadInGUIThread) noexcept{
	m_root = new display::Directory(nullptr);
	m_current = m_root;

	create_tree(m_listing->getRoot().get(), m_root);
	create_list();
}
void WindowShareBrowser::on(DirectoryListingListener::LoadingFailed, const string& aReason) noexcept{

}
void WindowShareBrowser::on(DirectoryListingListener::LoadingStarted, bool changeDir) noexcept{

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

}
void WindowShareBrowser::on(DirectoryListingListener::UpdateStatusMessage, const string& aMessage) noexcept{

}
void WindowShareBrowser::on(DirectoryListingListener::RemovedQueue, const string& aDir) noexcept{

}
void WindowShareBrowser::on(DirectoryListingListener::SetActive) noexcept{

}
void WindowShareBrowser::on(DirectoryListingListener::HubChanged) noexcept{

}

void WindowShareBrowser::create_tree(Dir *parent, display::Directory *parent2)
{
    display::Directory *current = new display::Directory(parent2);
    parent2->get_children().push_back(current);

	for (const auto& d : parent->directories) {
		create_tree(d.get(), current);
	}

    for(const auto& i: parent->files) {
        current->append_child(new ShareItem(i));
        //core::Log::get()->log(i->getName());
    }

}

void WindowShareBrowser::create_list(/*display::Directory *parent*/)
{
    //set_title(m_current->getName());

    const auto& dirs = m_current->get_children();
    for(const auto& d: dirs) {
        m_dirView->set_text(0, m_dirView->insert_row(), d->get_name());
    }

    const auto& files = m_current->get_items();
    for(const auto& i: files) {
        m_fileView->set_text(0, m_fileView->insert_row(), i->get_name());
    }

	m_state = display::STATE_ACTIVITY;
	events::emit("window status updated", static_cast<Window*>(this), m_state);
}

#if 0
void WindowShareBrowser::create_list()
{
/*    typedef DirectoryListing::Directory Dir;
    typedef DirectoryListing::File File;
    set_title(m_current->getName());
    display::Directory *currentDir = new display::Directory(m_current);

    DirectoryListing::Directory dirs = m_current->directories;
    for(Dir::Iter it = dirs.begin(); it != dirs.end(); it++) {
        m_dirView->set_text(0, m_dirView->insert_row(), (*it)->getName());
    }

    DirectoryListing::File files = m_current->files;
    for(File::List::iterator i = files.begin(); i != files.end(); ++i) {
        m_fileView->set_text(0, m_fileView->insert_row(), (*i)->getName());
    }
    m_windowupdated(this);*/
}


void WindowShareBrowser::create_list(DirectoryListing::Directory::List dirs, display::Directory *parent)
{
    typedef DirectoryListing::Directory Dir;
    typedef DirectoryListing::File File;
    display::Directory *child = new display::Directory(parent);

    for(Dir::Iter it = dirs.begin(); it != dirs.end(); it++) {
        for(File::List::iterator i=(*it)->files.begin(); i != (*it)->files.end(); ++i) {
            int row = m_fileView->insert_row();
            m_fileView->set_text(0, row, (*i)->getName());
            if(parent->get_parent() == 0) {
                set_current(child);
            }
            child->append_child(new ShareItem(*i));
        }
        int row = m_dirView->insert_row();
        m_dirView->set_text(0, row, (*it)->getName());
        create_list((*it)->directories, child);
    }
    m_windowupdated(this);
}

#endif

} // namespace ui
