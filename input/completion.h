
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

#ifndef _COMPLETION_H_
#define _COMPLETION_H_

#include <string>

#include <client/stdinc.h>
#include <client/Util.h>

using namespace std;

namespace input {

class Comparator {
	/* Case insensitive string comparison classes */
public:
	Comparator(const string& compareTo) : a(compareTo) {}
	bool operator()(const string& p) const;
protected:
	Comparator& operator=(const Comparator&) const;
	const string a;
};

class PrefixComparator : public input::Comparator {
public:
	string stripNick(const string& nick) const;

	PrefixComparator(const string& compareTo);
	bool operator()(const string& p) const;
};

class Completion {
public:
	static void getDiskPathSuggestions(const string& aPath, std::vector<std::string>& suggest_);
};

} // namespace input

#endif // _COMPLETION_H_
