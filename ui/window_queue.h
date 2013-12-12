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

#ifndef _WINDOWQUEUEU_H_
#define _WINDOWQUEUEU_H_

#include <client/stdinc.h>

#include <client/DownloadManagerListener.h>
#include <client/QueueManagerListener.h>

#include <client/Bundle.h>
/*#include <client/Upload.h>
#include <client/Transfer.h>
#include <client/HintedUser.h>*/

#include <display/listview.h>

using namespace dcpp;

namespace ui {

	class BundleItem {
	public:
		friend class WindowTransfers;
		BundleItem(const std::string& aToken) :
			m_token(aToken),
			m_left(-1), m_size(-1), m_bytes(-1) {
		}
	private:
		std::string m_target;
		std::string m_token;
		int64_t m_left = -1;
		int64_t m_size = -1;
		int64_t m_speed = 0;
		int64_t m_started = 0;
		int64_t m_bytes = -1;
	};

	class WindowQueue :
		public display::ListView,
		public DownloadManagerListener,
		public QueueManagerListener {
	public:
		WindowQueue();
		~WindowQueue();

		std::string get_infobox_line(unsigned int n);

		void search_bundle_alt();
		void remove_bundle();
		void remove_bundle_finished();
		void set_priority();

		void on(QueueManagerListener::BundleMoved, const BundlePtr& aBundle) noexcept;
		void on(QueueManagerListener::BundleMerged, const BundlePtr& aBundle, const string& oldTarget) noexcept;
		//void on(QueueManagerListener::BundleSources, const BundlePtr& aBundle) noexcept{ on(QueueManagerListener::BundlePriority(), aBundle); };
		//void on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept{ on(QueueManagerListener::BundlePriority(), aBundle); };
		void on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) noexcept;
		void on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept;
		void on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept;
		void on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept;

		void on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t aTick) noexcept;
		void on(DownloadManagerListener::BundleWaiting, const BundlePtr aBundle) noexcept;
	private:
		enum Columns {
			COLUMN_ID,
			COLUMN_NAME,
			COLUMN_STATUS,
			COLUMN_SIZE,
			COLUMN_PERCENT,
			COLUMN_SPEED,
			COLUMN_PRIORITY
			//COLUMN_TARGET
		};

		struct UpdateInfo {
			enum {
				MASK_POS = 0x01,
				MASK_SIZE = 0x02,
				MASK_ACTUAL = 0x04,
				MASK_SPEED = 0x08,
				MASK_FILE = 0x10,
				MASK_STATUS = 0x20,
				MASK_TIMELEFT = 0x40,
				MASK_STATUS_STRING = 0x100,
				MASK_FLAGS = 0x400,
				MASK_START = 0x2000,
				MASK_PRIORITY = 0x8000
			};

			UpdateInfo(string aToken) :
				updateMask(0), token(std::move(aToken)) {
			}

			uint32_t updateMask;

			string token;

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
			void setStatusString(const string& aStatusString) { statusString = aStatusString; updateMask |= MASK_STATUS_STRING; }
			string statusString;
			void setTarget(const string& aTarget) { target = aTarget; updateMask |= MASK_FILE; }
			string target;
			void setFlags(const string& aFlags) { flags = aFlags; updateMask |= MASK_FLAGS; }
			string flags;
			QueueItemBase::Priority prio;
			void setPriority(QueueItemBase::Priority aPrio) { prio = aPrio; updateMask |= MASK_PRIORITY; }
			HintedUser user;
		};

		enum Property {
			PROP_NONE,
			PROP_PRIORITY,
			PROP_REMOVE,
			PROP_REMOVE_FINISHED
		};
		void set_property(Property property);
		Property m_property = PROP_NONE;

		/** Handle user input. */
		void handle_line(const std::string &line);


		void speak(UpdateInfo* ui, bool added = false);
		void handleUpdateInfo(UpdateInfo* ui, bool added);

		int get_row(const std::string& aToken);
		BundlePtr get_bundle(const std::string& aToken);
		BundlePtr get_selected_bundle();
		//BundlePtr create_transfer(const HintedUser& user, bool download, const std::string& aToken);

		//void starting(UpdateInfo* ui, const Transfer* t);

		std::unordered_map<std::string, BundlePtr> m_bundles;
		void updateTitle();
		BundlePtr createBundle(const std::string& aToken);
	};
} // namespace ui

#endif // _WINDOWTRANSFERS_H_
