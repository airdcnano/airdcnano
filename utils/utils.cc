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

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utils/utils.h>
#include <sys/select.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

namespace utils {
    
void sleep(int usec)
{
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = usec;

    select(0, 0, 0, 0, &tv);
}

size_t real_length(const std::string &str)
{
    size_t l = 0;
    for(size_t i=0;i<str.length();++i) {
        if(str.at(i) == '%') {
            if(i+1 < str.length() && str.at(i+1) != '%') {
                i += 2;
            }
            continue;
        }
        l++;
    }
    return l;

}

std::string time_to_string(const std::string &format)
{
    return time_to_string(format, time(0));
}

std::string time_to_string(const std::string &format, time_t _tt)
{
    const int BUF_SIZE = 1024;
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);

    tm* _tm = localtime(&_tt);

    //if(_tm == NULL)
    //    throw std::runtime_error("_tm == NULL");

    strftime(buf, BUF_SIZE, format.c_str(), _tm);

    return std::string(buf);
}

std::string linux_separator(const std::string &ps)
{
    std::string str = ps;
    std::replace_if(str.begin(), str.end(),
        std::bind1st(std::equal_to<char>(), '\\'), '/');
    return str;
}

std::string windows_separator(const std::string &ps)
{
    std::string str = ps;
    std::replace_if(str.begin(), str.end(),
        std::bind2nd(std::equal_to<char>(), '/'), '\\');
    return str;
}

std::string to_utf8(const std::string &str) throw()
{
    std::string utf8;
    gchar *cstring = g_locale_to_utf8(str.c_str(), -1, NULL, NULL, NULL);
    if(cstring == NULL) {
        cstring = g_convert_with_fallback(str.c_str(), -1,
            "UTF-8", "ISO-8859-15", "", NULL, NULL, NULL);
        if(cstring == NULL)
            return utf8;
    }
    utf8 = std::string(cstring);
    g_free(cstring);
    return utf8;
}

std::string from_utf8(const std::string &str) throw()
{
    std::string result;
    gchar *cstring = g_locale_from_utf8(str.c_str(), -1, NULL, NULL, NULL);
    if (cstring == NULL)
        return result;
    result = std::string(cstring);
    g_free(cstring);
    return result;
}

std::string convert(const std::string &str, const std::string &from, const std::string &to)
    throw(std::runtime_error)
{
    gchar *converted = g_convert(str.c_str(), -1, to.c_str(), from.c_str(), 0, 0, 0);
    if(converted == 0)
        throw std::runtime_error("Character set conversion failed");
    std::string ret(converted);
    g_free(converted);
    return ret;
}

std::string tolower(const std::string &str) throw()
{
    std::string lowerString = to_utf8(str);
    gchar *lowerCString;
    if(!lowerString.empty()) {
        lowerCString = g_utf8_strdown(lowerString.c_str(), -1);
        lowerString = std::string(lowerCString);
       g_free(lowerCString);
    }
    return lowerString;
}

std::string ip_to_host(const std::string &ip)
{
    in_addr_t addr = inet_addr(ip.c_str());
    if(addr == INADDR_NONE)
        throw std::runtime_error("Invalid IP-address.");

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    struct hostent * host = gethostbyaddr((const char*)&addr, sizeof(in_addr_t), AF_INET);
#else
    struct hostent * host = gethostbyaddr(&addr, sizeof(in_addr_t), AF_INET);
#endif

    if(host)
        return host->h_name;

    switch(h_errno)
    {
        case HOST_NOT_FOUND:
            throw std::runtime_error("The specified host is unknown.");
        case NO_ADDRESS:
            throw std::runtime_error("The requested name is valid but does not have an IP address.");
        case NO_RECOVERY:
            throw std::runtime_error("A non-recoverable name server error occurred.");
        case TRY_AGAIN:
            throw std::runtime_error("A temporary error occurred on an authoritative name server.");
        default:
            throw std::runtime_error("Unknown error.");
    }
}


int utf16_char_to_utf8(int c, char *outbuf)
{
    int len, first;

    len = 0;
    if(c < 0x80) {
        first = 0;
        len = 1;
    } else if (c < 0x800) {
        first = 0xc0;
        len = 2;
    } else if (c < 0x10000) {
        first = 0xe0;
        len = 3;
    } else if (c < 0x200000) {
        first = 0xf0;
        len = 4;
    } else if (c < 0x4000000) {
        first = 0xf8;
        len = 5;
    } else {
        first = 0xfc;
        len = 6;
    }

    if(outbuf) {
        for(int i=len-1; i > 0;--i) {
            outbuf[i] = (c & 0x3f) | 0x80;
            c >>= 6;
        }
        outbuf[0] = c | first;
    }

    return len;
}

std::vector<std::string> make_vector(int n, ...)
{
    va_list ap;
    va_start(ap, n);

    std::vector<std::string> ret;
    for(int i=0; i<n; ++i)
        ret.push_back(va_arg(ap, char*));

    va_end(ap);
    return ret;
}

} // namespace utils

