
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
	bool operator()(const string& p) {
		return dcpp::Util::strnicmp(p.c_str(), a.c_str(), a.length()) == 0;
	}
protected:
	Comparator& operator=(const Comparator&);
	const string a;
};

class PrefixComparator : public input::Comparator {
public:
	string stripNick(const string& nick) const {
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

	PrefixComparator(const string& compareTo) : input::Comparator(stripNick(compareTo)) {}
	bool operator()(const string& p) {
		return dcpp::Util::strnicmp(stripNick(p).c_str(), a.c_str(), a.length()) == 0;
	}
};

} // namespace input

#endif // _COMPLETION_H_
