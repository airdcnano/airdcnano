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

#include <sstream>
#include <string>
#include <vector>
#include <iterator>
#include <stdexcept>
#include <cctype>
#include <glib.h>
#include <sys/time.h>
#include <time.h>

namespace utils {

static std::string empty_string;

/*class rot13_traits:
    public std::char_traits<char>
{
public:
    static char *copy(char *s1, const char *s2, std::size_t n)
    {
        while(n--) {
            if(std::isalpha(*s2)) {
                if(toupper(*s2) < ('A' + 13)) 
                    *s1 = *s2+13;
                else
                    *s1 = *s2-13;
            }
            else {
                *s1 = *s2;
            }
            *s1++;
            *s2++;
        }
        return s1-n;
    }
};*/

//typedef std::basic_string<char, rot13_traits> rot13_string;

/** Sleep 
    @param usec Microseconds to sleep */
void sleep(int usec);

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

/** Removes the last element of the container.
 * Does nothing if the container is empty. */
template<typename Container>
Container &remove_last(Container &container)
{
    if(container.length() != 0)
        container.erase(container.size()-1);
    return container;
}

/** Convert a string to T. */
template <class T>
T to(const std::string &str)
{
    T t;
    std::istringstream iss(str);
    iss >> t;
    return t;
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

/**
    Get string representation of current time.
    @param format Format eg. "[%H:%M:%S]"
*/
std::string time_to_string(const std::string &format);
std::string time_to_string(const std::string &format, time_t time);

/** Delete. */
template <typename T>
struct delete_functor
{
    void operator()(T *t) {
        delete t;
        t = 0;
    }
};

std::string to_utf8(const std::string &str) noexcept;

std::string from_utf8(const std::string &str) noexcept;

std::string convert(const std::string &str, const std::string &from, const std::string &to) throw(std::runtime_error);

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

std::string tolower(const std::string &str) noexcept;

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


} // namespace utils

#endif // _UTILS_H_
