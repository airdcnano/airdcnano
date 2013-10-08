
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include <sstream>
#include <iomanip>
#include <functional>
#include <ui/window_transfers.h>
#include <ui/window_privatemessage.h>
#include <ui/window_hub.h>
#include <display/manager.h>
#include <display/screen.h>
#include <client/Util.h>
#include <client/QueueManager.h>
#include <client/FavoriteManager.h>
#include <client/ConnectionManager.h>
#include <core/events.h>
#include <utils/utils.h>
#include <utils/lock.h>
#include <core/log.h>

namespace ui {


WindowTransfers::WindowTransfers() : ListView(display::TYPE_TRANSFERS, "transfers")
{
    DownloadManager::getInstance()->addListener(this);
    ConnectionManager::getInstance()->addListener(this);
    UploadManager::getInstance()->addListener(this);
    TimerManager::getInstance()->addListener(this);
    set_title("Transfer window");
    set_name("transfers");

    insert_column(new display::Column("ID"));
    insert_column(new display::Column("Flags", 3, 6, 6));
    insert_column(new display::Column("Nick", 8, 10, 20));
    insert_column(new display::Column("Speed", 13, 13, 13));
    insert_column(new display::Column("%%", 6, 6, 6));
    insert_column(new display::Column("Filename/Status", 50, 100, 200));
    resize();

    m_bindings['m'] = std::bind(&WindowTransfers::msg, this);
    m_bindings['F'] = std::bind(&WindowTransfers::force, this);
    m_bindings['f'] = std::bind(&WindowTransfers::add_favorite, this);
    m_bindings['r'] = std::bind(&WindowTransfers::remove_source, this);
    m_bindings['c'] = std::bind(&WindowTransfers::disconnect, this);
    m_bindings['R'] = std::bind(&WindowTransfers::remove_download, this);

    // browse
	//m_bindings['b'] = std::bind(&QueueManager::addList, QueueManager::getInstance(),
	 //                   std::bind(&WindowTransfers::get_user, this), QueueItem::FLAG_CLIENT_VIEW);
    // match queue
    //m_bindings['M'] = std::bind(&QueueManager::addList, QueueManager::getInstance(),
    //                    std::bind(&WindowTransfers::get_user, this), QueueItem::FLAG_MATCH_QUEUE);

    // grant a slot
    //m_bindings['g'] = std::bind(&UploadManager::reserveSlot,
    //                    UploadManager::getInstance(),
    //                    std::bind(&WindowTransfers::get_user, this));
}

HintedUser WindowTransfers::get_user()
{
    int row = get_selected_row();
    if(row == -1)
        return HintedUser();

    auto text = get_text(0, get_selected_row());
    return m_transfers[text]->m_user;
}

int WindowTransfers::get_row(const std::string& aToken)
{
	int row = find_row(0, aToken);
    if(row == -1) {
        row = insert_row();
		set_text(0, row, aToken);
    }
    return row;
}

TransferItem* WindowTransfers::create_transfer(const HintedUser& user, bool download, const std::string& aToken)
{
	if (m_transfers.find(aToken) != m_transfers.end()) {
		return m_transfers[aToken];
    }

	auto item = new TransferItem(user, download, aToken);
	m_transfers[aToken] = item;
    return item;
}

TransferItem* WindowTransfers::get_transfer(const std::string& aToken) {
	if (m_transfers.find(aToken) != m_transfers.end()) {
		return m_transfers[aToken];
	}
	
	dcassert(0);
	return nullptr;
}

void WindowTransfers::remove_download()
{
    utils::Lock l(m_mutex);

    int row = get_selected_row();
    if(row == -1)
        return;

    auto id = get_text(0, row);
    auto item = get_transfer(id);
	if (item->is_download())
		QueueManager::getInstance()->removeFile(item->m_target);
}

void WindowTransfers::add_favorite()
{
    utils::Lock l(m_mutex);

    int row = get_selected_row();
    if(row == -1)
        return;

    auto user = get_user();
    FavoriteManager::getInstance()->addFavoriteUser(user);
}

void WindowTransfers::remove_source()
{
    auto user = get_user();
    QueueManager::getInstance()->removeSource(user.user, QueueItem::Source::FLAG_REMOVED);
}

void WindowTransfers::msg()
{
	auto user = get_user();
	auto dm = display::Manager::get();
	auto p = dm->find(display::TYPE_PRIVMSG, user.user->getCID().toBase32());
	if (p != dm->end()) {
		dm->set_active_window(distance(dm->begin(), p));
	} else {
		std::string my_nick = static_cast<ui::WindowHub*>(*p)->get_nick();
		dm->push_back(new ui::WindowPrivateMessage(get_user(), my_nick));
	}
}

void WindowTransfers::disconnect()
{
    utils::Lock l(m_mutex);

    int row = get_selected_row();
    if(row == -1)
        return;

    std::string id = get_text(0, row);
    ConnectionManager::getInstance()->disconnect(id);
}

void WindowTransfers::force()
{
    utils::Lock l(m_mutex);

    int row = get_selected_row();
    if(row == -1)
        return;

	auto id = get_text(0, row);
	auto item = get_transfer(id);
	if (item->is_download()) {
		ConnectionManager::getInstance()->force(id);
		set_text(5, get_selected_row(), "Connecting (forced)");
	}
}

void WindowTransfers::transfer_completed(const Transfer *transfer)
{
	utils::Lock l(m_mutex);
    bool isDownload = transfer->getUserConnection().isSet(UserConnection::FLAG_DOWNLOAD);

    int row = get_row(transfer->getToken());
    set_text(4, row, "100");
	set_text(5, row, isDownload ? "Download finished, idle..." : "Upload finished, idle...");

    auto item = get_transfer(transfer->getToken());
    item->m_left = -1;
}

void WindowTransfers::on(TimerManagerListener::Second, uint64_t) noexcept {
    events::emit("window updated", static_cast<display::Window*>(this));
}

// ConnectionManager
void WindowTransfers::on(ConnectionManagerListener::Added, const ConnectionQueueItem *cqi)
    noexcept
{
    utils::Lock l(m_mutex);

    int row = get_row(cqi->getToken());

	set_text(2, row, Util::listToString(ClientManager::getInstance()->getNicks(cqi->getHintedUser())));
    set_text(4, row, "0");
    set_text(5, row, "Connecting...");

    TransferItem *item = create_transfer(cqi->getHintedUser(), cqi->getDownload(), cqi->getToken());
    item->m_started = GET_TICK();
}

void WindowTransfers::on(ConnectionManagerListener::StatusChanged, const ConnectionQueueItem *cqi) noexcept
{
    utils::Lock l(m_mutex);

    int row = get_row(cqi->getToken());

    if(cqi->getState() == ConnectionQueueItem::CONNECTING)
        set_text(5, row, "Connecting...");
    else
        set_text(5, row, "Waiting to retry");
}

void WindowTransfers::on(ConnectionManagerListener::Removed, const ConnectionQueueItem *cqi) noexcept {
    utils::Lock l(m_mutex);
    delete_row(0, cqi->getToken());
}

void WindowTransfers::on(ConnectionManagerListener::Failed, const ConnectionQueueItem *cqi, const std::string &reason) noexcept {
    utils::Lock l(m_mutex);

    int row = get_row(cqi->getToken());
    set_text(3, row, "");
    set_text(5, row, reason);
}

// DownloadManager
void WindowTransfers::on(DownloadManagerListener::Starting, const Download *dl) noexcept
{
    utils::Lock l(m_mutex);

    int row = get_row(dl->getToken());
    auto target = Text::acpToUtf8(dl->getPath());

    if(dl->getType() == Download::TYPE_FULL_LIST)
        set_text(5, row, "Starting: Filelist");
    else
        set_text(5, row, "Starting: " + Util::getFileName(target));

    auto item = get_transfer(dl->getToken());
    item->m_path = Util::getFilePath(target);
    item->m_size = dl->getSize();
    item->m_started = GET_TICK();
    item->m_target = target;
}

void WindowTransfers::on(DownloadManagerListener::Tick, const DownloadList &list) noexcept
{
    utils::Lock l(m_mutex);

    for(const auto& dl: list) {
        std::string flags = "D";
        if(dl->getUserConnection().isSecure())
            flags += "S";
        if(dl->isSet(Download::FLAG_ZDOWNLOAD))
            flags += "Z";
		if (dl->getUserConnection().isSet(UserConnection::FLAG_MCN1))
			flags += "M";

        int row = get_row(dl->getToken());
        set_text(1, row, flags);
		set_text(2, row, Util::listToString(ClientManager::getInstance()->getNicks(dl->getUserConnection().getHintedUser())));
        set_text(3, row, Util::formatBytes(dl->getAverageSpeed()) + "/s");
        set_text(4, row, utils::to_string(static_cast<int>((dl->getPos() * 100.0) / dl->getSize())));
        set_text(5, row, Util::getFileName(Text::acpToUtf8(dl->getPath())));

        auto item = get_transfer(dl->getToken());
        item->m_left = dl->getSecondsLeft();
        item->m_size = dl->getSize();
        item->m_bytes = dl->getPos();
		item->m_target = dl->getPath();
    }
}

void WindowTransfers::on(DownloadManagerListener::Failed, const Download *dl, const std::string &reason) noexcept
{
    utils::Lock l(m_mutex);

    int row = get_row(dl->getToken());
    set_text(5, row, reason);

    auto item = get_transfer(dl->getToken());
    auto target = Text::acpToUtf8(dl->getPath());

	if (dl->getType() == Download::TYPE_FULL_LIST)
        item->m_file = "Filelist";
    else
        item->m_file = Util::getFileName(target);

    item->m_path = Util::getFilePath(target);
}

// UploadManager
void WindowTransfers::on(UploadManagerListener::Starting, const Upload *ul) noexcept
{
    utils::Lock l(m_mutex);

    auto item = get_transfer(ul->getToken());
    auto target = Text::acpToUtf8(ul->getPath());

    if (ul->getType() == Upload::TYPE_FULL_LIST)
        item->m_file = "Filelist";
	else if (ul->getType() == Upload::TYPE_PARTIAL_LIST)
		item->m_file = "Partial list";
    else
        item->m_file = Util::getFileName(target);

    item->m_path = Util::getFilePath(target);
    item->m_size = ul->getSize();
    item->m_started = GET_TICK();
}

void WindowTransfers::on(UploadManagerListener::Tick, const UploadList &list) noexcept
{
    utils::Lock l(m_mutex);

    for(const auto& ul: list) {
        std::ostringstream stream;

        std::string flags = "U";
        if(ul->getUserConnection().isSecure())
            flags += "S";
        if(ul->isSet(Upload::FLAG_ZUPLOAD))
            flags += "Z";
		if (ul->getUserConnection().isSet(UserConnection::FLAG_MCN1))
			flags += "M";

        int row = get_row(ul->getToken());
        set_text(1, row, flags);
		set_text(2, row, Util::listToString(ClientManager::getInstance()->getNicks(ul->getUserConnection().getHintedUser())));
        set_text(3, row, Util::formatBytes(ul->getAverageSpeed()) + "/s");
        set_text(4, row, utils::to_string(static_cast<int>((ul->getPos() * 100.0) / ul->getSize())));
        set_text(5, row, ul->getPath());

        auto item = get_transfer(ul->getToken());
        item->m_left = ul->getSecondsLeft();
        item->m_bytes = ul->getPos();
        item->m_size = ul->getSize();
    }
}

WindowTransfers::~WindowTransfers()
{
    DownloadManager::getInstance()->removeListener(this);
    ConnectionManager::getInstance()->removeListener(this);
    UploadManager::getInstance()->removeListener(this);
    TimerManager::getInstance()->removeListener(this);
}

std::string WindowTransfers::get_infobox_line(unsigned int n)
{
	//return std::string(); // @todo ListView needs locking before this function can be run without crashes.

    auto text = get_text(0, get_selected_row());
    auto item = get_transfer(text);

    std::ostringstream oss;
    switch(n) {
        case 1:
        {
            oss << "%21Elapsed:%21 " << Util::formatSeconds((GET_TICK()-item->m_started) / 1000);
            if(item->m_left != -1)
                oss << " %21Left:%21 " << Util::formatSeconds(item->m_left);

            if(item->m_bytes != -1 && item->m_size != -1)
                oss << " %21Transferred:%21 " << Util::formatBytes(item->m_bytes)
                    << "/" << Util::formatBytes(item->m_size);
            break;
        }
        case 2:
            oss << item->m_path << item->m_file;
            break;
        case 3:
        {
            auto hubs = ClientManager::getInstance()->getHubNames(get_user());
            oss << "%21Hubs:%21 " + (hubs.empty() ? std::string("(Offline)") : Util::listToString(hubs));
            break;
        }
        case 4:
        {
            if(item->m_bytes == -1 || item->m_size == -1)
                break;

            double percent = static_cast<double>(item->m_bytes)/item->m_size;
            int width = display::Screen::get_width()-2;
            oss << "%21#%21" << std::string(static_cast<int>(width*percent), '#') + 
                std::string(static_cast<int>(width*(1-percent)), ' ') + "%21#%21";
        }
    }
    return oss.str();
}

}
