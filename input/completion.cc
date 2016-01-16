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

#include <algorithm>
#include "completion.h"

#include <client/File.h>

using namespace dcpp;

namespace input {

	bool Comparator::operator()(const string& p) const {
		return dcpp::Util::strnicmp(p.c_str(), a.c_str(), a.length()) == 0;
	}

	string PrefixComparator::stripNick(const string& nick) const {
		if (nick.substr(0, 1) != "[") return nick;
		string::size_type x = nick.find(']');
		string ret;
		// Avoid full deleting of [IMCOOL][CUSIHAVENOTHINGELSETHANBRACKETS]-type nicks
		if ((x != string::npos) && (nick.substr(x + 1).length() > 0)) {
			ret = nick.substr(x + 1);
		} else {
			ret = nick;
		}
		return ret;
	}

	PrefixComparator::PrefixComparator(const string& compareTo) : input::Comparator(stripNick(compareTo)) {}
	bool PrefixComparator::operator()(const string& p) const {
		return dcpp::Util::strnicmp(stripNick(p).c_str(), a.c_str(), a.length()) == 0;
	}

	void Completion::getDiskPathSuggestions(const string& aPath, vector<string>& suggest_) {
		string pattern;
		string path;
		if (aPath.empty()) {
			path = PATH_SEPARATOR;
		} else {
			if (aPath.back() != PATH_SEPARATOR) {
				auto p = aPath.rfind(PATH_SEPARATOR);
				path = aPath.substr(0, p + 1);
				pattern = aPath.substr(p + 1) + "*";
			} else {
				path = aPath;
			}
		}

		suggest_ = File::findFiles(path, pattern, File::TYPE_DIRECTORY | (SETTING(SHARE_HIDDEN) ? File::FLAG_HIDDEN : 0));
	}
}
