
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

#include <client/ConnectionManager.h>
#include <client/FavoriteManager.h>
#include <client/GeoManager.h>
#include <client/QueueManager.h>
#include <client/Socket.h>
#include <client/Util.h>

#include <core/events.h>
#include <utils/utils.h>
#include <utils/lock.h>
#include <core/log.h>

namespace ui {

static string getFile(const Transfer::Type& type, const string& fileName) {
	string file;

	if (type == Transfer::TYPE_TREE) {
		file = "TTH: " + fileName;
	} else if (type == Transfer::TYPE_FULL_LIST) {
		file = STRING(FILE_LIST);
	} else if (type == Transfer::TYPE_PARTIAL_LIST) {
		file = STRING(FILE_LIST_PARTIAL);
	} else {
		file = fileName;
	}
	return file;
}


WindowTransfers::WindowTransfers() : ListView(display::TYPE_TRANSFERS, "transfers")
{
    DownloadManager::getInstance()->addListener(this);
    ConnectionManager::getInstance()->addListener(this);
    UploadManager::getInstance()->addListener(this);
    TimerManager::getInstance()->addListener(this);
	updateTitle(0, 0);
    set_name("transfers");

    insert_column(new display::Column("ID"));
    insert_column(new display::Column("Flags", 3, 6, 8));
    insert_column(new display::Column("Nick", 10, 15, 20));
    insert_column(new display::Column("Speed", 13, 13, 13));
    insert_column(new display::Column("%%", 6, 6, 6));
    insert_column(new display::Column("Filename/Status", 40, 60, 200));
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
	WindowPrivateMessage::openWindow(user);
}

void WindowTransfers::disconnect()
{
    int row = get_selected_row();
    if(row == -1)
        return;

    std::string id = get_text(0, row);
    ConnectionManager::getInstance()->disconnect(id);
}

void WindowTransfers::force()
{
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

void WindowTransfers::transfer_completed(const Transfer *transfer, bool isDownload) {
	auto ui = new UpdateInfo(transfer->getToken(), isDownload);
	ui->setPos(-1);
	ui->setSpeed(-1);
	ui->setStatusString(isDownload ? STRING(DOWNLOAD_FINISHED_IDLE) : STRING(UPLOAD_FINISHED_IDLE));
	ui->setTimeLeft(-1);
	speak(ui);

    /*set_text(4, row, "100");
	set_text(5, row, isDownload ? "Download finished, idle..." : "Upload finished, idle...");

    auto item = get_transfer(transfer->getToken());
    item->m_left = -1;*/
}

void WindowTransfers::updateTitle(int64_t down, int64_t up) {
	string title = "Transfer window";
	if (down > 0 || up > 0) {
		title += " (total down: ";
		title += Util::formatBytes(down) + "/s, total up: ";
		title += Util::formatBytes(up) + "/s)";
	}

	set_title(title);
}

void WindowTransfers::on(TimerManagerListener::Second, uint64_t aTick) noexcept {
    events::emit("window updated", static_cast<display::Window*>(this));

	if (aTick == lastUpdate)	// FIXME: temp fix for new TimerManager
		return;

	int64_t totalDown = Socket::getTotalDown();
	int64_t totalUp = Socket::getTotalUp();

	int64_t diff = (int64_t) ((lastUpdate == 0) ? aTick - 1000 : aTick - lastUpdate);
	int64_t updiff = totalUp - lastUp;
	int64_t downdiff = totalDown - lastDown;

	callAsync([=] { updateTitle(downdiff * 1000LL / diff, updiff * 1000LL / diff);  });

	lastUpdate = aTick;
	lastUp = totalUp;
	lastDown = totalDown;
}

// ConnectionManager
void WindowTransfers::on(ConnectionManagerListener::Added, const ConnectionQueueItem* aCqi)
    noexcept
{
	auto ui = new UpdateInfo(aCqi->getToken(), aCqi->getDownload());
	if (ui->download) {
		string aTarget, bundleToken; int64_t aSize; int aFlags;
		if (QueueManager::getInstance()->getQueueInfo(aCqi->getHintedUser(), aTarget, aSize, aFlags, bundleToken)) {
			auto type = Transfer::TYPE_FILE;
			if (aFlags & QueueItem::FLAG_USER_LIST)
				type = Transfer::TYPE_FULL_LIST;
			else if (aFlags & QueueItem::FLAG_PARTIAL_LIST)
				type = Transfer::TYPE_PARTIAL_LIST;

			ui->setType(type);
			ui->setTarget(aTarget);
			ui->setSize(aSize);
			//ui->setBundle(bundleToken);
		}
	}

	ui->setUser(aCqi->getHintedUser());
	//ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setStatusString(STRING(CONNECTING));
	ui->setTimeLeft(-1);
	speak(ui, true);
}

void WindowTransfers::on(ConnectionManagerListener::StatusChanged, const ConnectionQueueItem *cqi) noexcept
{
	auto ui = new UpdateInfo(cqi->getToken(), cqi->getDownload());
	if (cqi->getState() == ConnectionQueueItem::CONNECTING)
		ui->setStatusString("Connecting...");
	else
		ui->setStatusString("Waiting to retry");

	speak(ui);
}

void WindowTransfers::on(ConnectionManagerListener::Removed, const ConnectionQueueItem *cqi) noexcept {
	auto token = cqi->getToken();
	callAsync([=] { delete_row(0, token); });
}

void WindowTransfers::on(ConnectionManagerListener::UserUpdated, const ConnectionQueueItem* aCqi) noexcept{
	auto ui = new UpdateInfo(aCqi->getToken(), aCqi->getDownload());
	ui->setUser(aCqi->getHintedUser());
	speak(ui);
}

void WindowTransfers::on(DownloadManagerListener::Requesting, const Download* d, bool hubChanged) noexcept{
	auto ui = new UpdateInfo(d->getToken(), true);
	if (hubChanged)
		ui->setUser(d->getHintedUser());

	starting(ui, d);

	ui->setActual(d->getActual());
	ui->setSize(d->getSize());
	//ui->setStatus(ItemInfo::STATUS_RUNNING);	
	//ui->updateMask &= ~UpdateInfo::MASK_STATUS; // hack to avoid changing item status
	ui->setStatusString(STRING(REQUESTING) + " " + getFile(d->getType(), Util::getFileName(d->getPath())) + "...");
	//ui->setBundle(d->getBundle() ? d->getBundle()->getToken() : Util::emptyString);
	speak(ui);
}

void WindowTransfers::on(ConnectionManagerListener::Failed, const ConnectionQueueItem* aCqi, const std::string& aReason) noexcept{
	auto ui = new UpdateInfo(aCqi->getToken(), aCqi->getDownload(), true);
	if (aCqi->getUser()->isSet(User::OLD_CLIENT)) {
		ui->setStatusString(STRING(SOURCE_TOO_OLD));
	} else {
		ui->setStatusString(aReason);
	}

	ui->setSpeed(-1);
	ui->setPos(-1);
	ui->setTimeLeft(-1);
	speak(ui);
	//ui->setBundle(aCqi->getLastBundle());
	//ui->setStatus(ItemInfo::STATUS_WAITING);
}

// DownloadManager
void WindowTransfers::on(DownloadManagerListener::Starting, const Download *aDownload) noexcept
{
	auto ui = new UpdateInfo(aDownload->getToken(), true);
	starting(ui, aDownload);

	ui->setStatusString(STRING(DOWNLOAD_STARTING));

	speak(ui);

	//ui->setStatus(ItemInfo::STATUS_RUNNING);
	/*ui->setStatusString(STRING(DOWNLOAD_STARTING));
	ui->setTarget(aDownload->getPath());
	ui->setType(aDownload->getType());*/

	//ui->setBundle(aDownload->getBundle() ? aDownload->getBundle()->getToken() : Util::emptyString);
}

void WindowTransfers::on(DownloadManagerListener::Tick, const DownloadList &list) noexcept
{
    for(const auto& dl: list) {
		auto ui = new UpdateInfo(dl->getToken(), true);

        std::string flags = "D";
        if(dl->getUserConnection().isSecure())
            flags += "S";
        if(dl->isSet(Download::FLAG_ZDOWNLOAD))
            flags += "Z";
		if (dl->getUserConnection().isSet(UserConnection::FLAG_MCN1))
			flags += "M";

		ui->setFlags(flags);
		ui->setSpeed(dl->getAverageSpeed());
		ui->setPos(dl->getPos());
		ui->setTimeLeft(dl->getSecondsLeft());
		speak(ui);
    }
}

void WindowTransfers::on(DownloadManagerListener::Failed, const Download* aDownload, const std::string &aReason) noexcept
{
	auto ui = new UpdateInfo(aDownload->getToken(), true, true);
	//ui->setStatus(ItemInfo::STATUS_WAITING);
	ui->setPos(-1);
	ui->setSize(aDownload->getSize());
	ui->setTarget(aDownload->getPath());
	ui->setType(aDownload->getType());
	//ui->setBundle(aDownload->getBundle() ? aDownload->getBundle()->getToken() : Util::emptyString);

	string tmpReason = aReason;
	if (aDownload->isSet(Download::FLAG_SLOWUSER)) {
		tmpReason += ": " + STRING(SLOW_USER);
	} else if (aDownload->getOverlapped() && !aDownload->isSet(Download::FLAG_OVERLAP)) {
		tmpReason += ": " + STRING(OVERLAPPED_SLOW_SEGMENT);
	}

	ui->setStatusString(tmpReason);
	speak(ui);
}

// UploadManager
void WindowTransfers::on(UploadManagerListener::Starting, const Upload *aUpload) noexcept
{
	UpdateInfo* ui = new UpdateInfo(aUpload->getToken(), false);
	starting(ui, aUpload);

	ui->setActual(aUpload->getStartPos() + aUpload->getActual());
	ui->setSize(aUpload->getType() == Transfer::TYPE_TREE ? aUpload->getSize() : aUpload->getFileSize());
	ui->setRunning(1);
	speak(ui);
}

void WindowTransfers::starting(UpdateInfo* ui, const Transfer* t) {
	ui->setPos(t->getPos());
	ui->setTarget(t->getPath());
	ui->setType(t->getType());
	ui->setStart(GET_TICK());
	ui->setSize(t->getSize());
	const auto& uc = t->getUserConnection();

	const auto& ip = uc.getRemoteIp();
	const auto& country = GeoManager::getInstance()->getCountry(uc.getRemoteIp());
	if (country.empty()) {
		ui->setIP(ip);
	} else {
		ui->setIP(country + " (" + ip + ")");
	}
}

void WindowTransfers::on(UploadManagerListener::Tick, const UploadList &list) noexcept
{
    for(const auto& ul: list) {
		if (ul->getPos() == 0) continue;

		UpdateInfo* ui = new UpdateInfo(ul->getToken(), false);

        std::string flags = "U";
        if(ul->getUserConnection().isSecure())
            flags += "S";
        if(ul->isSet(Upload::FLAG_ZUPLOAD))
            flags += "Z";
		if (ul->getUserConnection().isSet(UserConnection::FLAG_MCN1))
			flags += "M";

		ui->setFlags(flags);
		ui->setSpeed(ul->getAverageSpeed());
		ui->setPos(ul->getPos());
		ui->setTimeLeft(ul->getSecondsLeft());
		ui->setActual(ul->getActual());
		speak(ui);
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
    auto text = get_text(0, get_selected_row());
    auto item = get_transfer(text);
	if (!item) {
		return "";
	}

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
			if (!item->m_ip.empty())
				oss << " %21IP:%21 " + item->m_ip;

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

// Updating functions
void WindowTransfers::speak(UpdateInfo* ui, bool added) {
	callAsync([=] { handleUpdateInfo(ui, added); });
}

void WindowTransfers::handleUpdateInfo(UpdateInfo* ui, bool added) {
	int row = get_row(ui->token);
	auto item = added ? create_transfer(ui->user, ui->download, ui->token) : get_transfer(ui->token);
	if (!item) {
		dcassert(0);
		return;
	}

	auto updateMask = ui->updateMask;
	if (updateMask & UpdateInfo::MASK_SIZE) {
		item->m_size = ui->size;
	}

	if (updateMask & UpdateInfo::MASK_POS) {
		set_text(4, row, item->m_size > 0 && ui->pos >= 0 ? utils::to_string(static_cast<int>((ui->pos * 100.0) / item->m_size)) : "");
		item->m_bytes = ui->pos;
	}
	if (updateMask & UpdateInfo::MASK_STATUS_STRING) {
		if (!item->transferFailed)
			set_text(5, row, ui->statusString);
		item->transferFailed = ui->transferFailed;
	}
	//if (updateMask & UpdateInfo::MASK_POS || updateMask & UpdateInfo::MASK_ACTUAL) {
	//ctrlTransfers.updateItem(ii, COLUMN_RATIO);
	//}
	if (updateMask & UpdateInfo::MASK_SPEED) {
		set_text(3, row, ui->speed > 0 ? Util::formatBytes(ui->speed) + "/s" : "");
		item->m_speed = ui->speed;
	}
	if (updateMask & UpdateInfo::MASK_FILE) {
		item->m_target = ui->target;
		set_text(5, row, getFile(ui->type, Util::getFileName(ui->target)));
	}
	if (updateMask & UpdateInfo::MASK_TIMELEFT) {
		item->m_left = ui->timeLeft;
	}
	if (updateMask & UpdateInfo::MASK_IP) {
		item->m_ip = ui->IP;
	}
	if (updateMask & UpdateInfo::MASK_FLAGS) {
		set_text(1, row, ui->flags);
	}
	if (updateMask & UpdateInfo::MASK_START) {
		item->m_started = ui->start;
	}
	if (updateMask & UpdateInfo::MASK_USER) {
		set_text(2, row, Util::listToString(ClientManager::getInstance()->getNicks(ui->user)));
	}

	delete ui;
}

}
