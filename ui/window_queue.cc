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

#include <sstream>
#include <iomanip>
#include <functional>
#include <ui/window_queue.h>
#include <display/manager.h>
#include <display/screen.h>

#include <client/DownloadManager.h>
#include <client/QueueManager.h>
#include <client/Util.h>

#include <core/events.h>
#include <utils/utils.h>
#include <input/completion.h>

namespace ui {

static string getStatusString(const BundlePtr& aBundle) {
	if (aBundle->isPausedPrio()) {
		return STRING(PAUSED);
	}

	switch (aBundle->getStatus()) {
	case Bundle::STATUS_NEW:
	case Bundle::STATUS_QUEUED: return STRING(QUEUED);
	case Bundle::STATUS_DOWNLOADED:
	case Bundle::STATUS_RECHECK: return "Rechecking";
	case Bundle::STATUS_MOVED: return "Downloaded";
	case Bundle::STATUS_FAILED_MISSING:
	case Bundle::STATUS_SHARING_FAILED: return "Failed";
	case Bundle::STATUS_FINISHED: return "Finished";
	case Bundle::STATUS_HASHING: return "Hashing";
	case Bundle::STATUS_HASH_FAILED: return "Hash failed";
	case Bundle::STATUS_HASHED: return "Hashed";
	case Bundle::STATUS_SHARED: return "Shared";
	}
}

static string getPrioString(const BundlePtr& aBundle) {
	return AirUtil::getPrioText(aBundle->getPriority()) + (aBundle->getAutoPriority() ? " (auto)" : "");
}

WindowQueue::WindowQueue() : ListView(display::TYPE_QUEUE, "queue") {
	DownloadManager::getInstance()->addListener(this);
	QueueManager::getInstance()->addListener(this);
	updateTitle();
	set_name("queue");

	insert_column(new display::Column("ID"));
	insert_column(new display::Column("Name", 10, 15, 50));
	insert_column(new display::Column("Status", 10, 15, 20));
	insert_column(new display::Column("Size", 6, 6, 12));
	insert_column(new display::Column("%%", 6, 6, 6));
	insert_column(new display::Column("Speed", 3, 8, 15));
	insert_column(new display::Column("Priority", 13, 13, 20));
	//insert_column(new display::Column("Target", 40, 60, 200));
	resize();

	m_bindings['a'] = std::bind(&WindowQueue::search_bundle_alt, this);
	m_bindings['r'] = std::bind(&WindowQueue::remove_bundle, this);
	m_bindings['R'] = std::bind(&WindowQueue::remove_bundle_finished, this);
	m_bindings['p'] = std::bind(&WindowQueue::set_priority, this);
	m_bindings['m'] = std::bind(&WindowQueue::move_bundle, this);
	m_bindings['F'] = std::bind(&WindowQueue::force_share_bundle, this);
	m_bindings['S'] = std::bind(&WindowQueue::rescan_bundle, this);
    m_bindings['i'] = std::bind(&WindowQueue::recheck_integrity, this);

	// add the existing bundles
	{
		auto qm = QueueManager::getInstance();
		RLock l(qm->getCS());
		for (const auto& b : qm->getBundles() | map_values) {
			on(QueueManagerListener::BundleAdded(), b);
		}
	}
}

void WindowQueue::complete(const std::vector<std::string>& aArgs, int /*pos*/, std::vector<std::string>& suggest_, bool& appendSpace_) {
	if (m_property == PROP_MOVE) {
		if (!aArgs[0].empty()) {
			input::Completion::getDiskPathSuggestions(aArgs[0], suggest_);
		} else {
			suggest_ = SettingsManager::getInstance()->getHistory(SettingsManager::HISTORY_DIR);
		}

		appendSpace_ = false;
	}
}

BundlePtr WindowQueue::get_bundle(const std::string& aToken) {
	if (m_bundles.find(aToken) != m_bundles.end()) {
		return m_bundles[aToken];
	}

	dcassert(0);
	return nullptr;
}

void WindowQueue::set_property(Property property) {
	m_property = property;
	setInsertMode(true);
	const char *text[] = {
		"",
		"Priority? (0 - 6)",
		"Remove bundle? (y/N)",
		"Remove bundle and finished files? (y/N)",
		"Enter new location"
	};
	m_prompt = text[m_property];
}

void WindowQueue::handle_line(const std::string &line) {
	bool resetPrompt = true;
	if (!getInsertMode())
		return;

	if (!line.empty()) {
		if (m_property == PROP_PRIORITY) {
			int row = get_selected_row();
			if (row == -1)
				return;

			QueueManager::getInstance()->setBundlePriority(get_text(0, row), static_cast<QueueItemBase::Priority>(Util::toInt(line)));
			set_prompt_timed("Priority set to " + AirUtil::getPrioText(Util::toInt(line)));
			resetPrompt = false;
		} else if (m_property == PROP_REMOVE_FINISHED || m_property == PROP_REMOVE) {
			if (line == "y") {
				auto b = get_selected_bundle();
				if (b)
					QueueManager::getInstance()->removeBundle(b, m_property == PROP_REMOVE_FINISHED);
			}
		} else if (m_property == PROP_MOVE) {
			auto b = get_selected_bundle();
			if (b) {
				QueueManager::getInstance()->moveBundle(b, Util::validatePath(line, true), true);
			}
		}
	}

	setInsertMode(false);
	if (resetPrompt)
		set_prompt("");
	m_property = PROP_NONE;
}

BundlePtr WindowQueue::get_selected_bundle() {
	int row = get_selected_row();
	if (row == -1)
		return nullptr;

	return get_bundle(get_text(0, row));
}

void WindowQueue::move_bundle() {
	set_property(PROP_MOVE);
}

void WindowQueue::search_bundle_alt() {
	auto b = get_selected_bundle();
	if (b) {
		QueueManager::getInstance()->searchBundle(b, true);
	}
}

void WindowQueue::force_share_bundle() {
	auto b = get_selected_bundle();
	if (b) {
		QueueManager::getInstance()->shareBundle(b, true);
	}
}

void WindowQueue::rescan_bundle() {
	auto b = get_selected_bundle();
	if (b) {
		QueueManager::getInstance()->shareBundle(b, false);
	}
}

void WindowQueue::recheck_integrity() {
    auto b = get_selected_bundle();
    if (b) {
        QueueManager::getInstance()->recheckBundle(b->getToken());
    }
}

void WindowQueue::remove_bundle() {
	auto b = get_selected_bundle();
	if (!b)
		return;

	// no confirmation with finished bundles
	if (b->isFinished()) {
		QueueManager::getInstance()->removeBundle(b, false);
		return;
	}

	set_property(PROP_REMOVE);
}

void WindowQueue::remove_bundle_finished() {
	set_property(PROP_REMOVE_FINISHED);
}

void WindowQueue::set_priority() {
	set_property(PROP_PRIORITY);
}

int WindowQueue::get_row(const std::string& aToken) {
	int row = find_row(0, aToken);
	if (row == -1) {
		row = insert_row();
		set_text(0, row, aToken);
	}
	return row;
}

void WindowQueue::updateTitle() {
	string title = "Queue window";
	title += " (" + Util::toString(m_bundles.size()) + " bundles)";
	set_title(title);
}

void WindowQueue::on(QueueManagerListener::BundleMoved, const BundlePtr& aBundle) noexcept{
	auto ui = new UpdateInfo(aBundle->getToken());
	ui->setTarget(aBundle->getTarget());
	speak(ui);
}

void WindowQueue::on(QueueManagerListener::BundleMerged, const BundlePtr& aBundle, const string& oldTarget) noexcept{

}

void WindowQueue::on(QueueManagerListener::BundleSize, const BundlePtr& aBundle) noexcept{
	auto ui = new UpdateInfo(aBundle->getToken());
	ui->setSize(aBundle->getSize());
	speak(ui);
}

void WindowQueue::on(QueueManagerListener::BundlePriority, const BundlePtr& aBundle) noexcept{
	auto ui = new UpdateInfo(aBundle->getToken());
	ui->setPriority(aBundle->getPriority());
	ui->setStatusString(getStatusString(aBundle));
	speak(ui);
}

BundlePtr WindowQueue::createBundle(const std::string& aToken) {
	auto b = QueueManager::getInstance()->findBundle(aToken);
	if (b) {
		m_bundles.emplace(b->getToken(), b);
	}

	return b;
}

void WindowQueue::on(QueueManagerListener::BundleAdded, const BundlePtr& aBundle) noexcept{
	auto ui = new UpdateInfo(aBundle->getToken());
	ui->setStatusString(getStatusString(aBundle));
	ui->setTimeLeft(-1);
	ui->setTarget(aBundle->getTarget());
	ui->setSize(aBundle->getSize());
	ui->setPriority(aBundle->getPriority());
	speak(ui, true);
}

void WindowQueue::on(QueueManagerListener::BundleRemoved, const BundlePtr& aBundle) noexcept{
	auto token = aBundle->getToken();
	callAsync([=] { 
		delete_row(0, token);
		m_bundles.erase(token);
	});
}

void WindowQueue::on(QueueManagerListener::BundleStatusChanged, const BundlePtr& aBundle) noexcept{
	auto ui = new UpdateInfo(aBundle->getToken());
	ui->setStatusString(getStatusString(aBundle));
	if (aBundle->isFinished()) {
		// reset columns
		ui->setPriority(aBundle->getPriority());
		ui->setPos(aBundle->getDownloadedBytes());
	}

	speak(ui);
}

void WindowQueue::on(DownloadManagerListener::BundleTick, const BundleList& tickBundles, uint64_t aTick) noexcept{
	for (const auto& b : tickBundles) {
		auto ui = new UpdateInfo(b->getToken());
		ui->setSpeed(b->getSpeed());
		ui->setPos(b->getDownloadedBytes());
		ui->setStatusString("Running");
		speak(ui);
	}

}

void WindowQueue::on(DownloadManagerListener::BundleWaiting, const BundlePtr aBundle) noexcept{
	auto ui = new UpdateInfo(aBundle->getToken());
	ui->setStatusString(getStatusString(aBundle));
	ui->setSpeed(-1);
	//ui->setPos(aBundle->get);
	//aBundle->
	speak(ui, true);
}

WindowQueue::~WindowQueue() {
	DownloadManager::getInstance()->removeListener(this);
	QueueManager::getInstance()->removeListener(this);
}

std::string WindowQueue::get_infobox_line(unsigned int n) {
	auto text = get_text(0, get_selected_row());
	auto b = get_bundle(text);
	if (!b) {
		return "";
	}

	std::ostringstream oss;
	switch (n) {
	case 1:
	{
		//oss << "%21Elapsed:%21 " << Util::formatSeconds((GET_TICK() - b->m_started) / 1000);
		if (b->getDownloadedBytes() != -1)
			oss << "%21Downloaded:%21 " << Util::formatBytes(b->getDownloadedBytes())
			<< "/" << Util::formatBytes(b->getSize());
		if (b->getSecondsLeft() > 0 && b->getSpeed() > 0)
			oss << " %21Left:%21 " << Util::formatSeconds(b->getSecondsLeft());
		break;
	}
	case 2:
		oss << "%21Target:%21 " << b->getTarget();
		break;
	case 3:
	{
		RLock l(QueueManager::getInstance()->getCS());
		oss << "%21Files:%21 " + Util::toString(b->getQueueItems().size());
		oss << " %21Sources:%21 " + Util::toString(b->getSources().size());
		break;
	}
	case 4:
	{
		if (b->getDownloadedBytes() == -1)
			break;

		double percent = static_cast<double>(b->getDownloadedBytes()) / b->getSize();
		int width = display::Screen::get_width() - 2;
		oss << "%21#%21" << std::string(static_cast<int>(width*percent), '#') +
			std::string(static_cast<int>(width*(1 - percent)), ' ') + "%21#%21";
	}
	}
	return oss.str();
}

// Updating functions
void WindowQueue::speak(UpdateInfo* ui, bool added) {
	callAsync([=] { handleUpdateInfo(ui, added); });
}

void WindowQueue::handleUpdateInfo(UpdateInfo* ui, bool added) {
	int row = get_row(ui->token);
	auto b = added ? createBundle(ui->token) : get_bundle(ui->token);
	if (!b || row == -1) {
		//dcassert(0);
		return;
	}

	if (added) {
		updateTitle();
	}

	auto updateMask = ui->updateMask;
	if (updateMask & UpdateInfo::MASK_SIZE) {
		set_text(COLUMN_SIZE, row, Util::formatBytes(ui->size));
	}
	if (updateMask & UpdateInfo::MASK_POS) {
		set_text(COLUMN_PERCENT, row, b->getSize() > 0 && ui->pos >= 0 ? utils::to_string(static_cast<int>((ui->pos * 100.0) / b->getSize())) : "");
	}
	if (updateMask & UpdateInfo::MASK_STATUS_STRING) {
		set_text(COLUMN_STATUS, row, ui->statusString);
	}
	if (updateMask & UpdateInfo::MASK_SPEED) {
		set_text(COLUMN_SPEED, row, ui->speed > 0 ? Util::formatBytes(ui->speed) + "/s" : "");
	}
	if (updateMask & UpdateInfo::MASK_FILE) {
		//set_text(COLUMN_TARGET, row, ui->target);
		set_text(COLUMN_NAME, row, b->getName());
	}
	if (updateMask & UpdateInfo::MASK_PRIORITY) {
		set_text(COLUMN_PRIORITY, row, b->isFinished() ? "" : getPrioString(b));
	}

	delete ui;
}

}
