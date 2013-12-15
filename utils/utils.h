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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <iterator>
#include <stdexcept>
#include <cctype>
#include <glib.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <time.h>

namespace utils {

static std::string empty_string;

/** Returns the length of formatted string without color codes. */
template <typename String>
static size_t length(String str) {
	size_t l = 0;
	for (size_t i = 0; i < str.length(); ++i) {
		if (str.at(i) == '%') {
			if (i + 1 < str.length() && str.at(i + 1) != '%')
				i += 2;
			continue;
		}
		l++;
	}
	return l;
}

template <typename String>
static String escape(String str) {
	String escaped;
	typename String::size_type i;
	for (i = 0; i < str.length(); ++i) {
		if (str[i] == '%')
			escaped.append(1, '%');
		escaped.append(1, str[i]);
	}
	return escaped;
}

template <typename String>
static String remove_formatting(String str) {
	String lusikka;
	lusikka.reserve(str.length());

	typename String::size_type i;
	for (i = 0; i < str.length(); ++i) {
		if (str[i] == '%') {
			if (i + 1 < str.length() && str.at(i + 1) != '%')
				i += 2;
			continue;
		}
		lusikka.append(1, str[i]);
	}
	return lusikka;
}

/** Convert \ -> / */
std::string linux_separator(const std::string &ps);

/** Convert / -> \ */
std::string windows_separator(const std::string &ps);

template<class T> std::string to_string(T s)
{
    std::ostringstream oss;
    oss << s;
    return oss.str();
}

/**  */
template <typename Iterator>
bool find_in_string(const std::string &str, Iterator start, Iterator end)
{
    while(start != end) {
        if(str.find(*start) != std::string::npos)
            return true;
        ++start;
    }
    return false;
}

/** Delete. */
template <typename T>
struct delete_functor
{
    void operator()(T *t) {
        delete t;
        t = 0;
    }
};

inline long strlen(const std::string &str) noexcept
{
    return g_utf8_strlen(str.c_str(), -1);
}

/** Returns the time since the Epoch in milliseconds. */
inline
uint32_t get_millisecs() {
    timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)((tv.tv_sec) * 1000 ) + ( (tv.tv_usec) / 1000);
}

/** Converts an IP-address to hostname. Eg. 66.35.250.150 -> slashdot.org */
std::string ip_to_host(const std::string &ip);

/** Creates a vector from a list of strings. */
std::vector<std::string> make_vector(int n, ...);

int utf16_char_to_utf8(int c, char *outbuf);

/** gettid() system call number */
//const unsigned int SYS_gettid = 224;

/** Returns thread id of the calling thread.
 * Implemented only if \c __i386__ is defined. */
inline static pid_t gettid() {
    pid_t tid = 0;
#ifdef __i386__
    __asm__("int $0x80" :
            "=a" (tid) :
            "0" (SYS_gettid));
#endif
    return tid;
}

template<typename T>
static void slide(T& list, int oldPos, int newPos) {
	auto cur = std::begin(list) + newPos;
	auto p = std::begin(list) + oldPos;
	if (p < cur) {
		std::rotate(p, cur, cur + 1);
	} else if (cur < p) {
		std::rotate(cur, cur + 1, p + 1);
	}
}


} // namespace utils

#endif // _UTILS_H_
