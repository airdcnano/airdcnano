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

#ifndef _WINDOWTRANSFERS_H_
#define _WINDOWTRANSFERS_H_

#include <client/stdinc.h>

#include <client/DownloadManagerListener.h>
#include <client/UploadManagerListener.h>
#include <client/ConnectionManagerListener.h>

#include <client/Download.h>
#include <client/Upload.h>
#include <client/Transfer.h>
#include <client/HintedUser.h>

#include <display/listview.h>

using namespace dcpp;

namespace ui {

class TransferItem
{
public:
    friend class WindowTransfers;
    TransferItem(const HintedUser& user, bool isDownload, const std::string& aToken):
		m_user(user), m_isDownload(isDownload), m_token(aToken),
        m_left(-1), m_size(-1), m_bytes(-1)
    { }

    bool is_download() const { return m_isDownload; }
private:
    HintedUser m_user;
    bool m_isDownload;

    std::string m_file;
    std::string m_path;
    std::string m_target;
	std::string m_token;
    //int m_progress;
    //char m_status;
    int64_t m_left;
    int64_t m_size;
    int64_t m_speed;
    int64_t m_started;
    int64_t m_bytes;
	std::string m_ip;
	bool transferFailed = false;
};

class WindowTransfers:
    public display::ListView,
    public DownloadManagerListener,
    public ConnectionManagerListener,
    public UploadManagerListener
{
public:
    WindowTransfers();
    ~WindowTransfers();


    /** Called when a transfer is completed. */
    void transfer_completed(const Transfer *transfer, bool isDownload);

    std::string get_infobox_line(unsigned int n);

    void force();
    void msg();
    void add_favorite();
    void remove_source();
    void disconnect();
    /** Disconnects the selected download and removes it from the queue. */
    void remove_download();

	void on(ConnectionManagerListener::Added, const ConnectionQueueItem *item) noexcept;
	void on(ConnectionManagerListener::Removed, const ConnectionQueueItem *item) noexcept;
    void on(ConnectionManagerListener::Failed, const ConnectionQueueItem *item, const string &reason) noexcept;
	void on(ConnectionManagerListener::StatusChanged, const ConnectionQueueItem *item) noexcept;
	void on(ConnectionManagerListener::UserUpdated, const ConnectionQueueItem* aCqi) noexcept;

    void on(DownloadManagerListener::Starting, const Download *dl) noexcept;
    void on(DownloadManagerListener::Tick, const DownloadList &list) noexcept;
    void on(DownloadManagerListener::Complete, const Download *dl, bool) noexcept { transfer_completed(dl, true); }
    void on(DownloadManagerListener::Failed, const Download *dl, const string &reason) noexcept;
	void on(DownloadManagerListener::Requesting, const Download* aDownload, bool hubChanged) noexcept;

    void on(UploadManagerListener::Starting, const Upload *ul) noexcept;
	void on(UploadManagerListener::Tick, const UploadList &list) noexcept;
    void on(UploadManagerListener::Complete, const Upload *ul) noexcept { transfer_completed(ul, false); }
private:
	struct UpdateInfo {
		enum {
			MASK_POS = 0x01,
			MASK_SIZE = 0x02,
			MASK_ACTUAL = 0x04,
			MASK_SPEED = 0x08,
			MASK_FILE = 0x10,
			MASK_STATUS = 0x20,
			MASK_TIMELEFT = 0x40,
			MASK_IP = 0x80,
			MASK_STATUS_STRING = 0x100,
			MASK_SEGMENT = 0x200,
			MASK_FLAGS = 0x400,
			MASK_TOTALSPEED = 0x800,
			MASK_BUNDLE = 0x1000,
			MASK_START = 0x2000,
			MASK_USER = 0x4000,
			MASK_PRIORITY = 0x8000
		};

		UpdateInfo(string aToken, bool isDownload, bool isTransferFailed = false) :
			updateMask(0), user(HintedUser()), download(isDownload), token(std::move(aToken)), transferFailed(isTransferFailed), type(Transfer::TYPE_LAST) {
		}

		uint32_t updateMask;

		string token;

		bool download;
		bool transferFailed;
		void setRunning(int16_t aRunning) { running = aRunning; updateMask |= MASK_SEGMENT; }
		int16_t running;
		//void setStatus(ItemInfo::Status aStatus) { status = aStatus; updateMask |= MASK_STATUS; }
		//ItemInfo::Status status;
		void setPos(int64_t aPos) { pos = aPos; updateMask |= MASK_POS; }
		int64_t pos;
		void setSize(int64_t aSize) { size = aSize; updateMask |= MASK_SIZE; }
		int64_t size;
		void setActual(int64_t aActual) { actual = aActual; updateMask |= MASK_ACTUAL; }
		int64_t actual;
		void setSpeed(int64_t aSpeed) { speed = aSpeed; updateMask |= MASK_SPEED; }
		int64_t speed;
		void setStart(uint64_t aStart) { start = aStart; updateMask |= MASK_START; }
		uint64_t start;
		void setTimeLeft(int64_t aTimeLeft) { timeLeft = aTimeLeft; updateMask |= MASK_TIMELEFT; }
		int64_t timeLeft;
		void setTotalSpeed(int64_t aTotalSpeed) { totalSpeed = aTotalSpeed; updateMask |= MASK_TOTALSPEED; }
		int64_t totalSpeed;
		void setStatusString(const string& aStatusString) { statusString = aStatusString; updateMask |= MASK_STATUS_STRING; }
		string statusString;
		void setTarget(const string& aTarget) { target = aTarget; updateMask |= MASK_FILE; }
		string target;
		void setFlags(const string& aFlags) { flags = aFlags; updateMask |= MASK_FLAGS; }
		string flags;
		void setIP(const string& aIP) { IP = aIP; updateMask |= MASK_IP; }
		string IP;
		//void setCipher(const string& aCipher) { cipher = aCipher; updateMask |= MASK_CIPHER; }
		//string cipher;
		void setType(const Transfer::Type& aType) { type = aType; }
		Transfer::Type type;
		//void setBundle(const string& aBundle) { bundle = aBundle; updateMask |= MASK_BUNDLE; }
		//string bundle;
		//void setUsers(const int16_t aUsers) { users = aUsers; updateMask |= MASK_USERS; }
		//int16_t users;
		void setUser(const HintedUser& aUser) { user = aUser; updateMask |= MASK_USER; }
		QueueItemBase::Priority prio;
		void setPriority(QueueItemBase::Priority aPrio) { prio = aPrio; updateMask |= MASK_PRIORITY; }
		HintedUser user;
	};

	void speak(UpdateInfo* ui, bool added = false);
	void handleUpdateInfo(UpdateInfo* ui, bool added);

    int get_row(const std::string& aToken);
    TransferItem *get_transfer(const std::string& aToken);
	TransferItem* create_transfer(const HintedUser& user, bool download, const std::string& aToken);

	HintedUser get_user();

	void starting(UpdateInfo* ui, const Transfer* t);

    std::unordered_map<std::string, TransferItem*> m_transfers;
	void updateTitle(int64_t down, int64_t up);

	void handleBytes() noexcept;
	boost::signals2::connection bytesConn;
};
} // namespace ui

#endif // _WINDOWTRANSFERS_H_
