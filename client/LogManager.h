/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef DCPLUSPLUS_DCPP_LOG_MANAGER_H
#define DCPLUSPLUS_DCPP_LOG_MANAGER_H

#include <deque>
#include <utility>

#include "typedefs.h"

#include "Singleton.h"
#include "Speaker.h"
#include "LogManagerListener.h"
#include "User.h"

namespace dcpp {

using std::deque;
using std::pair;

class LogManager : public Singleton<LogManager>, public Speaker<LogManagerListener>
{
public:
	enum Severity { LOG_INFO, LOG_WARNING, LOG_ERROR };
	enum Area { CHAT, PM, DOWNLOAD, UPLOAD, SYSTEM, STATUS, LAST };
	enum { FILE, FORMAT };

	struct MessageData {
		MessageData(time_t t, Severity sev) : time(t), severity(sev) { }

		time_t time;
		Severity severity;
	};

	typedef deque<pair<string, MessageData> > List;

	void log(Area area, ParamMap& params) noexcept;
	void message(const string& msg, Severity severity);

	List getLastLogs();
	void clearLastLogs();
	string getPath(Area area, ParamMap& params) const;
	string getPath(Area area) const;

	// PM functions
	string getPath(const UserPtr& aUser, ParamMap& params, bool addCache = false);
	void log(const UserPtr& aUser, ParamMap& params);
	void removePmCache(const UserPtr& aUser);

	const string& getSetting(int area, int sel) const;
	void saveSetting(int area, int sel, const string& setting);
private:
	void log(const string& area, const string& msg) noexcept;

	friend class Singleton<LogManager>;
	CriticalSection cs;
	List lastLogs;

	int options[LAST][2];

	LogManager();
	virtual ~LogManager();

	unordered_map<CID, string> pmPaths;
	void ensureParam(const string& aParam, string& aFile);
};

#define LOG(area, msg) LogManager::getInstance()->log(area, msg)

} // namespace dcpp

#endif // !defined(DCPLUSPLUS_DCPP_LOG_MANAGER_H)
