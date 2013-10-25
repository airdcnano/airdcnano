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
#include <stdexcept>
#include <functional>
#include <client/stdinc.h>

#include <client/AirUtil.h>
#include <client/ConnectionManager.h>
#include <client/ConnectivityManager.h>
#include <client/SearchManager.h>
#include <client/SettingHolder.h>
#include <client/SettingItem.h>
#include <client/SettingsManager.h>
#include <client/StringTokenizer.h>

#include <display/manager.h>
#include <core/events.h>
#include <core/argparser.h>
#include <utils/utils.h>
#include <input/help_handler.h>

using namespace dcpp;

namespace modules {

#define SETTINGCOMP(func) (std::bind(&func, placeholders::_1, placeholders::_2))

struct NamedSettingItem : public SettingItem {
	typedef function<void(const string&, StringList&)> SettingCompletion;
	enum Type {
		TYPE_GENERAL,
		TYPE_CONN_V4,
		TYPE_CONN_V6,
		TYPE_CONN_GEN,
		TYPE_LIMITS_DL,
		TYPE_LIMITS_UL,
		TYPE_LIMITS_MCN
	};

	NamedSettingItem(const string& aName, int aKey, ResourceManager::Strings aDesc, Type aType = TYPE_GENERAL, SettingCompletion aCompletion = nullptr) :
		name(aName), type(aType), completionF(aCompletion), SettingItem({ aKey, aDesc }) {

	}

	NamedSettingItem(ResourceManager::Strings aDesc) : isTitle(true), SettingItem({ 0, aDesc }) {

	}

	string printHelp() const {
		if (isTitle) {
			return "	---[ " + getDescription() + " ]---";
		}

		return getDisplayName() + ": " + toString();
		//return name + " (cur: " + currentToString() + "): " + getDescription();
	}

	string getDisplayName() const {
		string add;
		if (key == SettingsManager::INCOMING_CONNECTIONS || key == SettingsManager::INCOMING_CONNECTIONS6) {
			add = " (-1=disabled, 0=active, 1=upnp, 2=passive)";
		} else if (key == SettingsManager::TLS_MODE) {
			add = " (0=disabled, 1=enabled, 2=forced in ADC hubs)";
		} else if (key == SettingsManager::SETTINGS_PROFILE) {
			add = " (0=normal, 1=rar, 2=lan)";
		} else if (key == SettingsManager::DL_AUTO_DISCONNECT_MODE) {
			add = " (0=file, 1=bundle, 2=whole queue)";
		} else if (key == SettingsManager::MONITORING_MODE) {
			add = " (0=disabled, 1=incoming dirs, 2=all dirs)";
		} else if (key == SettingsManager::DELAY_COUNT_MODE) {
			add = " (0=per-volume, 1=per-directory, 2=from any change)";
		} else if (key == SettingsManager::BLOOM_MODE) {
			add = " (0=disabled, 1=enabled, 2=auto)";
		} else if (key == SettingsManager::REFRESH_THREADING) {
			add = " (0=disabled, 1=manual refreshes, 2=all)";
		} else if (key == SettingsManager::OUTGOING_CONNECTIONS) {
			add = " (0=direct, 1=socks5)";
		}

		return name + add;
	}

	string toString() const {
		optional<SettingValue> v;
		if ((type == TYPE_CONN_V4 && SETTING(AUTO_DETECT_CONNECTION)) || (type == TYPE_CONN_V6 && SETTING(AUTO_DETECT_CONNECTION6)) ||
			(type == TYPE_CONN_GEN && (SETTING(AUTO_DETECT_CONNECTION) || SETTING(AUTO_DETECT_CONNECTION6)))) {

			if (key == SettingsManager::TCP_PORT) {
				v = ConnectionManager::getInstance()->getPort();
			} else if (key == SettingsManager::UDP_PORT) {
				v = SearchManager::getInstance()->getPort();
			} else if (key == SettingsManager::TLS_PORT) {
				v = ConnectionManager::getInstance()->getSecurePort();
			} else {
				if (key >= SettingsManager::STR_FIRST && key < SettingsManager::STR_LAST) {
					v = ConnectivityManager::getInstance()->get(static_cast<SettingsManager::StrSetting>(key));
				} else if (key >= SettingsManager::INT_FIRST && key < SettingsManager::INT_LAST) {
					v = ConnectivityManager::getInstance()->get(static_cast<SettingsManager::IntSetting>(key));
				} else if (key >= SettingsManager::BOOL_FIRST && key < SettingsManager::BOOL_LAST) {
					v = ConnectivityManager::getInstance()->get(static_cast<SettingsManager::BoolSetting>(key));
				} else {
					dcassert(0);
				}
			}
		} else if (type == TYPE_LIMITS_DL && SETTING(DL_AUTODETECT)) {
			if (key == SettingsManager::DOWNLOAD_SLOTS)
				v = AirUtil::getSlots(true);
			else if (key == SettingsManager::MAX_DOWNLOAD_SPEED)
				v = AirUtil::getSpeedLimit(true);
			//else
			//	v = AirUtil::getMaxAutoOpened();
		} else if (type == TYPE_LIMITS_UL && SETTING(UL_AUTODETECT)) {
			if (key == SettingsManager::SLOTS)
				v = AirUtil::getSlots(false);
			else if (key == SettingsManager::MIN_UPLOAD_SPEED)
				v = AirUtil::getSpeedLimit(false);
			else if (key == SettingsManager::AUTO_SLOTS)
				v = AirUtil::getMaxAutoOpened();
		} else if (type == TYPE_LIMITS_MCN && SETTING(MCN_AUTODETECT)) {
			v = AirUtil::getSlotsPerUser(key == SettingsManager::MAX_MCN_DOWNLOADS);
		}

		string value;
		if (v) {
			value = boost::apply_visitor(ToString(key), *v);
		} else {
			value = currentToString();
		}

		if (value.empty())
			value = "Undefined";

		return value + (v ? " (auto)" : Util::emptyString);
	}

	void setCurValue(const std::string& aValue) const {
		if ((type == TYPE_CONN_V4 && SETTING(AUTO_DETECT_CONNECTION)) ||
			(type == TYPE_CONN_V6 && SETTING(AUTO_DETECT_CONNECTION6))) {
				display::Manager::get()->cmdMessage("Note: Connection autodetection is enabled for the edited protocol. The changed setting won't take effect before auto detection has been disabled.");
		}

		if ((type == TYPE_LIMITS_DL && SETTING(DL_AUTODETECT)) ||
			(type == TYPE_LIMITS_UL && SETTING(UL_AUTODETECT)) ||
			(type == TYPE_LIMITS_MCN && SETTING(MCN_AUTODETECT))) {

				display::Manager::get()->cmdMessage("Note: auto detection is enabled for the edited settings group. The changed setting won't take effect before auto detection has been disabled.");
		}

		if (key >= SettingsManager::STR_FIRST && key < SettingsManager::STR_LAST) {
			SettingsManager::getInstance()->set(static_cast<SettingsManager::StrSetting>(key), aValue);
		} else if (key >= SettingsManager::INT_FIRST && key < SettingsManager::INT_LAST) {
			size_t end;
			int multiplier;
			auto hasType = [&, this](string&& id) {
				end = Util::findSubString(aValue, id, aValue.size() - id.size());
				return end != string::npos;
			};

			if (hasType("g")) {
				multiplier = 1024 * 1024 * 1024;
			} else if (hasType("m")) {
				multiplier = 1024 * 1024;
			} else if (hasType("k")) {
				multiplier = 1024;
			} else {
				multiplier = 1;
			}

			SettingsManager::getInstance()->set(static_cast<SettingsManager::IntSetting>(key), static_cast<int>(Util::toInt(aValue)*multiplier));
		} else if (key >= SettingsManager::BOOL_FIRST && key < SettingsManager::BOOL_LAST) {
			bool val = Util::stricmp(aValue, "true") == 1 || aValue == "1" || Util::stricmp(aValue, "enabled") == 1 || Util::stricmp(aValue, "yes") == 1;
			SettingsManager::getInstance()->set(static_cast<SettingsManager::BoolSetting>(key), val);
		} else {
			dcassert(0);
		}
	}

	bool isTitle = false;
	string name;
	Type type;
	SettingCompletion completionF;
};

static void suggestBind(const string&, StringList& suggest_, bool v6) {
	AirUtil::IpList ips;
	AirUtil::getIpAddresses(ips, v6);
	for (const auto& info : ips) {
		suggest_.push_back(info.ip);
	}
}

static void suggestBind4(const string& aStr, StringList& suggest_) {
	suggestBind(aStr, suggest_, false);
}

static void suggestBind6(const string& aStr, StringList& suggest_) {
	suggestBind(aStr, suggest_, true);
}

static NamedSettingItem settings [] = {
	{ ResourceManager::SETTINGS_GENERAL },
	{ "profile", SettingsManager::SETTINGS_PROFILE, ResourceManager::SETTINGS_PROFILE },
	{ "nick", SettingsManager::NICK, ResourceManager::NICK },
	{ "description", SettingsManager::DESCRIPTION, ResourceManager::DESCRIPTION },
	{ "email", SettingsManager::EMAIL, ResourceManager::EMAIL },
	{ "ul_speed", SettingsManager::UPLOAD_SPEED, ResourceManager::SETCZDC_UPLOAD_SPEED },
	{ "dl_speed", SettingsManager::DOWNLOAD_SPEED, ResourceManager::SETCZDC_DOWNLOAD_SPEED },
	{ "away_message", SettingsManager::DEFAULT_AWAY_MESSAGE, ResourceManager::SETTINGS_DEFAULT_AWAY_MSG },
	{ "away_idle_time", SettingsManager::AWAY_IDLE_TIME, ResourceManager::AWAY_IDLE_TIME_BEGIN },
	{ "away_no_bots", SettingsManager::NO_AWAYMSG_TO_BOTS, ResourceManager::SETTINGS_NO_AWAYMSG_TO_BOTS },
	{ "nmdc_encoding", SettingsManager::NMDC_ENCODING, ResourceManager::INVALID_ENCODING },

	{ ResourceManager::SETTINGS_DOWNLOADS },
	{ "dl_dir", SettingsManager::DOWNLOAD_DIRECTORY, ResourceManager::SETTINGS_DOWNLOAD_DIRECTORY, NamedSettingItem::TYPE_GENERAL, SETTINGCOMP(input::Completion::getDiskPathSuggestions) },
	{ "temp_dir", SettingsManager::TEMP_DOWNLOAD_DIRECTORY, ResourceManager::SETTINGS_UNFINISHED_DOWNLOAD_DIRECTORY, NamedSettingItem::TYPE_GENERAL, SETTINGCOMP(input::Completion::getDiskPathSuggestions) },
	{ "temp_use_dest", SettingsManager::DCTMP_STORE_DESTINATION, ResourceManager::UNFINISHED_STORE_DESTINATION },
	{ "segmented_dl", SettingsManager::MULTI_CHUNK, ResourceManager::SETTINGS_SEGMENTED_DOWNLOADS },
	{ "min_segment_size", SettingsManager::MIN_SEGMENT_SIZE, ResourceManager::SETTINGS_AIRDOWNLOADS_SEGMENT_SIZE },
	{ "new_segment_min_speed", SettingsManager::DONT_BEGIN_SEGMENT_SPEED, ResourceManager::DONT_ADD_SEGMENT_TEXT },
	{ "allow_slow_overlap", SettingsManager::OVERLAP_SLOW_SOURCES, ResourceManager::SETTINGS_OVERLAP_SLOW_SOURCES },
	{ "add_finished", SettingsManager::ADD_FINISHED_INSTANTLY, ResourceManager::ADD_FINISHED_INSTANTLY },
	{ "finished_no_hash", SettingsManager::FINISHED_NO_HASH, ResourceManager::SETTINGS_FINISHED_NO_HASH },

	{ ResourceManager::SETTINGS_SKIPPING_OPTIONS },
	{ "dont_dl_shared", SettingsManager::DONT_DL_ALREADY_SHARED, ResourceManager::SETTINGS_DONT_DL_ALREADY_SHARED },
	{ "dont_dl_queued", SettingsManager::DONT_DL_ALREADY_QUEUED, ResourceManager::SETTING_DONT_DL_ALREADY_QUEUED },
	{ "dupe_min_size", SettingsManager::MIN_DUPE_CHECK_SIZE, ResourceManager::MIN_DUPE_CHECK_SIZE },
	{ "skip_zero_byte", SettingsManager::SKIP_ZERO_BYTE, ResourceManager::SETTINGS_SKIP_ZERO_BYTE },
	{ "dl_skiplist", SettingsManager::SKIPLIST_DOWNLOAD, ResourceManager::SETTINGS_SKIPLIST_DOWNLOAD },
	{ "dl_skiplist_regex", SettingsManager::DOWNLOAD_SKIPLIST_USE_REGEXP, ResourceManager::USE_REGEXP },

	{ ResourceManager::SETTINGS_SEARCH_MATCHING },
	{ "auto_add_sources", SettingsManager::AUTO_ADD_SOURCE, ResourceManager::AUTO_ADD_SOURCE },
	{ "alt_search_auto", SettingsManager::AUTO_SEARCH, ResourceManager::SETTINGS_AUTO_BUNDLE_SEARCH },
	{ "alt_search_max_sources", SettingsManager::AUTO_SEARCH_LIMIT, ResourceManager::SETTINGS_AUTO_SEARCH_LIMIT },
	{ "max_sources_match_queue", SettingsManager::MAX_AUTO_MATCH_SOURCES, ResourceManager::SETTINGS_SB_MAX_SOURCES },
	
	{ ResourceManager::PROXIES },
	{ "http_proxy", SettingsManager::HTTP_PROXY, ResourceManager::SETTINGS_HTTP_PROXY },
	{ "outgoing_mode", SettingsManager::OUTGOING_CONNECTIONS, ResourceManager::SETTINGS_OUTGOING },
	{ "socks_server", SettingsManager::SOCKS_SERVER, ResourceManager::SETTINGS_SOCKS5_IP },
	{ "socks_user", SettingsManager::SOCKS_USER, ResourceManager::SETTINGS_SOCKS5_RESOLVE },
	{ "socks_password", SettingsManager::SOCKS_PASSWORD, ResourceManager::PASSWORD },
	{ "socks_port", SettingsManager::SOCKS_PORT, ResourceManager::PORT },
	{ "socks_resolve", SettingsManager::SOCKS_RESOLVE, ResourceManager::SETTINGS_SOCKS5_RESOLVE },

	{ ResourceManager::IP_V4 },
	{ "connection4_auto", SettingsManager::AUTO_DETECT_CONNECTION, ResourceManager::ALLOW_AUTO_DETECT_V4 },
	{ "connection4_bind", SettingsManager::BIND_ADDRESS, ResourceManager::SETTINGS_BIND_ADDRESS, NamedSettingItem::TYPE_CONN_V4, SETTINGCOMP(suggestBind4) },
	{ "connection4_mode", SettingsManager::INCOMING_CONNECTIONS, ResourceManager::CONNECTIVITY, NamedSettingItem::TYPE_CONN_V4 },
	{ "connection4_ip", SettingsManager::EXTERNAL_IP, ResourceManager::SETTINGS_EXTERNAL_IP, NamedSettingItem::TYPE_CONN_V4 },

	{ ResourceManager::IP_V6 },
	{ "connection6_auto", SettingsManager::AUTO_DETECT_CONNECTION6, ResourceManager::ALLOW_AUTO_DETECT_V6 },
	{ "connection6_bind", SettingsManager::BIND_ADDRESS6, ResourceManager::SETTINGS_BIND_ADDRESS, NamedSettingItem::TYPE_CONN_V6, SETTINGCOMP(suggestBind6) },
	{ "connection6_mode", SettingsManager::INCOMING_CONNECTIONS6, ResourceManager::CONNECTIVITY, NamedSettingItem::TYPE_CONN_V6 },
	{ "connection6_ip", SettingsManager::EXTERNAL_IP6, ResourceManager::SETTINGS_EXTERNAL_IP, NamedSettingItem::TYPE_CONN_V6 },

	{ ResourceManager::SETTINGS_PORTS },
	{ "tcp_port", SettingsManager::TCP_PORT, ResourceManager::SETTINGS_TCP_PORT, NamedSettingItem::TYPE_CONN_GEN },
	{ "udp_port", SettingsManager::UDP_PORT, ResourceManager::SETTINGS_UDP_PORT, NamedSettingItem::TYPE_CONN_GEN },
	{ "tls_port", SettingsManager::TLS_PORT, ResourceManager::SETTINGS_TLS_PORT, NamedSettingItem::TYPE_CONN_GEN },

	{ ResourceManager::DOWNLOAD_LIMITS },
	{ "dl_auto_limits", SettingsManager::DL_AUTODETECT, ResourceManager::AUTODETECT },
	{ "dl_slots", SettingsManager::DOWNLOAD_SLOTS, ResourceManager::SETTINGS_DOWNLOADS_MAX, NamedSettingItem::TYPE_LIMITS_DL },
	{ "dl_max_slot_speed", SettingsManager::MAX_DOWNLOAD_SPEED, ResourceManager::SETTINGS_DOWNLOADS_SPEED_PAUSE, NamedSettingItem::TYPE_LIMITS_DL },
	{ "dl_highest_prio_slots", SettingsManager::EXTRA_DOWNLOAD_SLOTS, ResourceManager::SETTINGS_CZDC_EXTRA_DOWNLOADS },

	{ ResourceManager::UPLOAD_LIMITS },
	{ "ul_auto_limits", SettingsManager::UL_AUTODETECT, ResourceManager::AUTODETECT },
	{ "ul_grant_speed", SettingsManager::MIN_UPLOAD_SPEED, ResourceManager::SETTINGS_UPLOADS_MIN_SPEED, NamedSettingItem::TYPE_LIMITS_UL },
	{ "ul_max_granted", SettingsManager::AUTO_SLOTS, ResourceManager::SETTINGS_AUTO_SLOTS, NamedSettingItem::TYPE_LIMITS_UL },
	{ "ul_slots", SettingsManager::SLOTS, ResourceManager::SETTINGS_UPLOADS_SLOTS, NamedSettingItem::TYPE_LIMITS_UL },
	{ "ul_minislot_size", SettingsManager::SET_MINISLOT_SIZE, ResourceManager::SETCZDC_SMALL_FILES },
	{ "ul_minislot_ext", SettingsManager::FREE_SLOTS_EXTENSIONS, ResourceManager::ST_MINISLOTS_EXT },

	{ ResourceManager::TRASFER_RATE_LIMITING },
	{ "limit_ul_max", SettingsManager::MAX_UPLOAD_SPEED_MAIN, ResourceManager::UPLOAD_LIMIT },
	{ "limit_dl_max", SettingsManager::MAX_DOWNLOAD_SPEED_MAIN, ResourceManager::DOWNLOAD_LIMIT },
	{ "limit_use_alt", SettingsManager::TIME_DEPENDENT_THROTTLE, ResourceManager::ALTERNATE_LIMITING },
	{ "limit_alt_start_hour", SettingsManager::BANDWIDTH_LIMIT_START, ResourceManager::SET_ALTERNATE_LIMITING },
	{ "limit_alt_end_hour", SettingsManager::BANDWIDTH_LIMIT_END, ResourceManager::SET_ALTERNATE_LIMITING },
	{ "limit_ul_alt_max", SettingsManager::MAX_UPLOAD_SPEED_ALTERNATE, ResourceManager::UPLOAD_LIMIT },
	{ "limit_dl_alt_max", SettingsManager::MAX_UPLOAD_SPEED_ALTERNATE, ResourceManager::DOWNLOAD_LIMIT },
	{ "limit_use_with_auto_values", SettingsManager::AUTO_DETECTION_USE_LIMITED, ResourceManager::DOWNLOAD_LIMIT },

	{ ResourceManager::SETTINGS_MCNSLOTS },
	{ "mcn_auto_limits", SettingsManager::MCN_AUTODETECT , ResourceManager::AUTODETECT },
	{ "mcn_down", SettingsManager::MAX_MCN_DOWNLOADS, ResourceManager::SETTINGS_MAX_MCN_DL, NamedSettingItem::TYPE_LIMITS_MCN },
	{ "mcn_up", SettingsManager::MAX_MCN_UPLOADS, ResourceManager::SETTINGS_MAX_MCN_UL, NamedSettingItem::TYPE_LIMITS_MCN },

	{ ResourceManager::HASHING_OPTIONS },
	{ "max_hash_speed", SettingsManager::MAX_HASH_SPEED, ResourceManager::SETTINGS_MAX_HASHER_SPEED },
	{ "max_total_hashers", SettingsManager::MAX_HASHING_THREADS, ResourceManager::MAX_HASHING_THREADS },
	{ "max_vol_hashers", SettingsManager::HASHERS_PER_VOLUME, ResourceManager::MAX_VOL_HASHERS },
	{ "report_each_hashed_file", SettingsManager::LOG_HASHING, ResourceManager::LOG_HASHING },

	{ ResourceManager::REFRESH_OPTIONS },
	{ "refresh_time", SettingsManager::AUTO_REFRESH_TIME, ResourceManager::SETTINGS_AUTO_REFRESH_TIME },
	{ "refresh_time_incoming", SettingsManager::INCOMING_REFRESH_TIME, ResourceManager::SETTINGS_INCOMING_REFRESH_TIME },
	{ "refresh_startup", SettingsManager::STARTUP_REFRESH, ResourceManager::SETTINGS_STARTUP_REFRESH },
	{ "refresh_report_scheduled_refreshes", SettingsManager::LOG_SCHEDULED_REFRESHES, ResourceManager::SETTINGS_LOG_SCHEDULED_REFRESHES },

	{ ResourceManager::SETTINGS_SHARING_OPTIONS },
	{ "share_skiplist", SettingsManager::SKIPLIST_SHARE, ResourceManager::ST_SKIPLIST_SHARE },
	{ "share_skiplist_regex", SettingsManager::SHARE_SKIPLIST_USE_REGEXP, ResourceManager::USE_REGEXP },
	{ "share_hidden", SettingsManager::SHARE_HIDDEN, ResourceManager::SETTINGS_SHARE_HIDDEN },
	{ "share_no_empty_dirs", SettingsManager::SKIP_EMPTY_DIRS_SHARE, ResourceManager::DONT_SHARE_EMPTY_DIRS },
	{ "share_no_zero_byte", SettingsManager::NO_ZERO_BYTE, ResourceManager::SETTINGS_NO_ZERO_BYTE },
	{ "share_max_size", SettingsManager::MAX_FILE_SIZE_SHARED, ResourceManager::DONT_SHARE_BIGGER_THAN },
	{ "share_follow_symlinks", SettingsManager::SHARE_FOLLOW_SYMLINKS, ResourceManager::FOLLOW_SYMLINKS },
	{ "share_report_duplicates", SettingsManager::FL_REPORT_FILE_DUPES, ResourceManager::REPORT_DUPLICATE_FILES },
	{ "share_report_skiplist", SettingsManager::REPORT_SKIPLIST, ResourceManager::REPORT_SKIPLIST },

	{ ResourceManager::SETTINGS_LOGGING },
	{ "log_dir", SettingsManager::LOG_DIRECTORY, ResourceManager::SETTINGS_LOG_DIR, NamedSettingItem::TYPE_GENERAL, SETTINGCOMP(input::Completion::getDiskPathSuggestions) },
	{ "log_main", SettingsManager::LOG_MAIN_CHAT, ResourceManager::SETTINGS_LOG_MAIN_CHAT },
	{ "log_main_file", SettingsManager::LOG_FILE_MAIN_CHAT, ResourceManager::FILENAME },
	{ "log_main_format", SettingsManager::LOG_FORMAT_MAIN_CHAT, ResourceManager::SETTINGS_FORMAT },
	{ "log_pm", SettingsManager::LOG_PRIVATE_CHAT, ResourceManager::SETTINGS_LOG_PRIVATE_CHAT },
	{ "log_pm_file", SettingsManager::LOG_FILE_PRIVATE_CHAT, ResourceManager::FILENAME },
	{ "log_pm_format", SettingsManager::LOG_FORMAT_PRIVATE_CHAT, ResourceManager::SETTINGS_FORMAT },
	{ "log_downloads", SettingsManager::LOG_DOWNLOADS, ResourceManager::SETTINGS_LOG_DOWNLOADS },
	{ "log_downloads_file", SettingsManager::LOG_FILE_DOWNLOAD, ResourceManager::FILENAME },
	{ "log_downloads_format", SettingsManager::LOG_FORMAT_POST_DOWNLOAD, ResourceManager::SETTINGS_FORMAT },
	{ "log_uploads", SettingsManager::LOG_UPLOADS, ResourceManager::SETTINGS_LOG_UPLOADS },
	{ "log_uploads_file", SettingsManager::LOG_FILE_UPLOAD, ResourceManager::FILENAME },
	{ "log_uploads_format", SettingsManager::LOG_FORMAT_POST_UPLOAD, ResourceManager::SETTINGS_FORMAT },
	{ "log_syslog", SettingsManager::LOG_SYSTEM, ResourceManager::SETTINGS_LOG_SYSTEM_MESSAGES },
	{ "log_syslog_file", SettingsManager::LOG_FILE_SYSTEM, ResourceManager::FILENAME },
	{ "log_syslog_format", SettingsManager::LOG_FORMAT_SYSTEM, ResourceManager::SETTINGS_FORMAT },
	{ "log_status", SettingsManager::LOG_STATUS_MESSAGES, ResourceManager::SETTINGS_LOG_STATUS_MESSAGES },
	{ "log_status_file", SettingsManager::LOG_FILE_STATUS, ResourceManager::FILENAME },
	{ "log_status_format", SettingsManager::LOG_FORMAT_STATUS, ResourceManager::SETTINGS_FORMAT },
	{ "log_list_transfers", SettingsManager::LOG_FILELIST_TRANSFERS, ResourceManager::SETTINGS_FORMAT },
	{ "single_log_per_cid", SettingsManager::PM_LOG_GROUP_CID, ResourceManager::LOG_COMBINE_ADC_PM },

	{ ResourceManager::HISTORIES },
	{ "history_search_max", SettingsManager::HISTORY_SEARCH_MAX, ResourceManager::SEARCH },
	{ "history_search_clear_exit", SettingsManager::HISTORY_SEARCH_CLEAR, ResourceManager::CLEAR_EXIT },
	{ "history_download_max", SettingsManager::HISTORY_DIR_MAX, ResourceManager::DIRECTORY },
	{ "history_download_clear_exit", SettingsManager::HISTORY_DIR_CLEAR, ResourceManager::CLEAR_EXIT },
	{ "history_last_pm_lines", SettingsManager::SHOW_LAST_LINES_LOG, ResourceManager::MAX_LOG_LINES },

	{ ResourceManager::SETTINGS_AIR_TABSPAGE },
	{ "open_transfers", SettingsManager::SHOW_TRANSFERVIEW, ResourceManager::MENU_TRANSFERS },
	{ "open_hublist", SettingsManager::OPEN_PUBLIC, ResourceManager::MAX_HASHING_THREADS },
	{ "open_favorites", SettingsManager::OPEN_FAVORITE_HUBS, ResourceManager::MAX_VOL_HASHERS },

	{ ResourceManager::SETTINGS_ADVANCED },
	{ "socket_read_buffer", SettingsManager::SOCKET_IN_BUFFER, ResourceManager::SETTINGS_SOCKET_IN_BUFFER },
	{ "socket_write_buffer", SettingsManager::SOCKET_OUT_BUFFER, ResourceManager::SETTINGS_SOCKET_OUT_BUFFER },
	{ "buffer_size", SettingsManager::BUFFER_SIZE, ResourceManager::SETTINGS_WRITE_BUFFER },
	{ "compress_transfers", SettingsManager::COMPRESS_TRANSFERS, ResourceManager::SETTINGS_COMPRESS_TRANSFERS },
	{ "max_compression", SettingsManager::MAX_COMPRESSION, ResourceManager::SETTINGS_MAX_COMPRESS },
	{ "tls_mode", SettingsManager::TLS_MODE, ResourceManager::TRANSFER_ENCRYPTION },
	{ "bloom_mode", SettingsManager::BLOOM_MODE, ResourceManager::BLOOM_MODE },
	{ "accept_failovers_fav", SettingsManager::ACCEPT_FAILOVERS, ResourceManager::ACCEPT_FAILOVERS_GLOBAL },
};


class DcSet {

public:
	HelpHandler::CommandList commands = {
		{ "connectioninfo", std::bind(&DcSet::connection, this), nullptr },
		{ "dcset", std::bind(&DcSet::handleDcset, this), COMPLETION(DcSet::handleSuggest) },
		{ "dcreset", std::bind(&DcSet::handleDcreset, this), COMPLETION(DcSet::handleSuggest) },
		{ "nick", std::bind(&DcSet::set_nick, this), nullptr }
	};

	HelpHandler help;

	DcSet() : help(&commands, "Config") {
        /*events::add_listener("command dcset",
                std::bind(&DcSet::set, this));

        events::add_listener("command nick",
                std::bind(&DcSet::set_nick, this));*/
    }

	void handleSuggest(const StringList& aArgs, int pos, StringList& suggest_, bool& appendSpace_) {
		if (pos == 1) {
			for (const auto& s : settings) {
				if (!s.isTitle)
					suggest_.push_back(s.name);
			}
			appendSpace_ = true;
		} else if (pos == 2) {
			auto p = find_setting(aArgs[1]);
			if (p > 0 && settings[p].completionF) {
				settings[p].completionF(aArgs[2], suggest_);
			}
			appendSpace_ = false;
		}
	}

	void connection() {
		auto info = ConnectivityManager::getInstance()->getInformation();
		StringTokenizer<string> lines(info, "\n");
		for (const auto& l : lines.getTokens()) {
			if (!l.empty())
				display::Manager::get()->cmdMessage(l);
		}
	}

    /** "command nick" event handler. */
    void set_nick() {
        if(events::args() < 1) {
			display::Manager::get()->cmdMessage("Not enough parameters given");
            return;
        }

        SettingsManager::getInstance()->set(SettingsManager::NICK, events::arg<std::string>(0));
		events::emit("nick changed");
		SettingsManager::getInstance()->save();
    }

    // Find position corresponding the given setting.
    int find_setting(const std::string& aName)
    {
		int pos = 0;
		for (const auto& s : settings) {
			if (Util::stricmp(s.name, aName) == 0) {
				return pos;
			}
			pos++;
		}

        return -1;
    }

	void handleDcreset() {
		core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
		parser.parse();

		if (parser.args() != 1) {
			display::Manager::get()->cmdMessage("Usage: /dcreset <name>");
			return;
		}

		auto var = parser.arg(0);
		auto p = find_setting(var);
		if (p > 0) {
			auto old = settings[p].currentToString();
			settings[p].unset();
			display::Manager::get()->cmdMessage("\"" + settings[p].name + "\" was reset to the default value \"" + settings[p].currentToString() + "\" (old: \"" + old + "\")");
		}
	}

    /** "command dcset" event handler. */
    void handleDcset() {
        //bool set = events::args() > 1;
		core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
		parser.parse();

		if (parser.args() == 0) {
			//string print;
			for (const auto& s : settings) {
				//print += s.printHelp() + "\r";
				if (s.isTitle)
					display::Manager::get()->cmdMessage("");
				display::Manager::get()->cmdMessage(s.printHelp());
			}

			display::Manager::get()->cmdMessage("");
			display::Manager::get()->cmdMessage("Usage: /dcset <name> [value]");
		} else {
			auto var = parser.arg(0);
			auto p = find_setting(var);
			if (p > 0) {
				SettingHolder h([this](const std::string& e) { display::Manager::get()->cmdMessage("Failed to set the port: " + e); });
				auto old = settings[p].currentToString();
				if (parser.args() == 1) {
					display::Manager::get()->cmdMessage("The current value of \"" + settings[p].name + "\" is \"" + settings[p].currentToString() + "\"");
				} else {
					auto val = parser.arg(1);
					settings[p].setCurValue(val);
					display::Manager::get()->cmdMessage("\"" + settings[p].name + "\" was set to \"" + settings[p].currentToString() + "\" (old: \"" + old + "\")");

					if (settings[p].key == SettingsManager::NICK) {
						events::emit("nick changed");
					}
				}
			} else {
				display::Manager::get()->cmdMessage("Setting " + var + " not found");
			}

			SettingsManager::getInstance()->save();
		}
    }
};

} // namespace modules

static modules::DcSet initialize;

