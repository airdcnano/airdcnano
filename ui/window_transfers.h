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
#include <client/Download.h>
#include <client/Upload.h>
#include <client/DownloadManager.h>
#include <client/UploadManager.h>
#include <client/ConnectionManager.h>
#include <client/ClientManager.h>
#include <client/ConnectionManagerListener.h>
#include <client/TimerManager.h>
#include <client/Transfer.h>
#include <client/User.h>
#include <client/HintedUser.h>

#include <core/log.h>
#include <display/listview.h>
#include <utils/mutex.h>
#include <vector>
#include <string>
#include <map>

using namespace dcpp;

namespace ui {

class TransferItem
{
public:
    friend class WindowTransfers;
    TransferItem(const HintedUser& user, bool isDownload, const std::string aToken):
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
    int m_progress;
    char m_status;
    int64_t m_left;
    int64_t m_size;
    int64_t m_speed;
    int64_t m_started;
    int64_t m_bytes;
};

class WindowTransfers:
    public display::ListView,
    public DownloadManagerListener,
    public ConnectionManagerListener,
    public UploadManagerListener,
    public TimerManagerListener
{
public:
    WindowTransfers();
    ~WindowTransfers();


    /** Called when a transfer is completed. */
    void transfer_completed(const Transfer *transfer);

    std::string get_infobox_line(unsigned int n);

    void force();
    void msg();
    void add_favorite();
    void remove_source();
    void disconnect();
    /** Disconnects the selected download and removes it from the queue. */
    void remove_download();

    void on(TimerManagerListener::Second, uint64_t) noexcept;

	void on(ConnectionManagerListener::Added, const ConnectionQueueItem *item) noexcept;
	void on(ConnectionManagerListener::Removed, const ConnectionQueueItem *item) noexcept;
    void on(ConnectionManagerListener::Failed, const ConnectionQueueItem *item, const string &reason) noexcept;
	void on(ConnectionManagerListener::StatusChanged, const ConnectionQueueItem *item) noexcept;

    void on(DownloadManagerListener::Starting, const Download *dl) noexcept;
    void on(DownloadManagerListener::Tick, const DownloadList &list) noexcept;
    void on(DownloadManagerListener::Complete, const Download *dl, bool) noexcept { transfer_completed(dl); }
    void on(DownloadManagerListener::Failed, const Download *dl, const string &reason) noexcept;

    void on(UploadManagerListener::Starting, const Upload *ul) noexcept;
	void on(UploadManagerListener::Tick, const UploadList &list) noexcept;
    void on(UploadManagerListener::Complete, const Upload *ul) noexcept { transfer_completed(ul); }
private:
    int get_row(const std::string& aToken);
    TransferItem *get_transfer(const std::string& aToken);
	TransferItem* create_transfer(const HintedUser& user, bool download, const std::string& aToken);

	HintedUser get_user();


    std::map<std::string, TransferItem*> m_transfers;
    utils::Mutex m_mutex;
};
} // namespace ui

#endif // _WINDOWTRANSFERS_H_
