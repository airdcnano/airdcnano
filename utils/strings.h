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

#ifndef _STRINGS_H_
#define _STRINGS_H_

#include <sstream>
#include <string>
#include <utils/utils.h> // utils::remove_last

namespace strings {

/** Split a string. */
template<class OutputIterator>
void split(const std::string& str, const std::string& sep, OutputIterator dest)
{
    std::string::size_type left = str.find_first_not_of(sep);
    std::string::size_type right = str.find_first_of(sep, left);

    while(left < right) {
        *dest = str.substr(left, right-left);
        ++dest;
        left = str.find_first_not_of(sep, right);
        right = str.find_first_of(sep, left);
    }
}

template<class Iterator>
std::string join(Iterator start, Iterator end, const char *sep)
{
    std::ostringstream oss;
    std::copy(start, end,
        std::ostream_iterator<std::string>(oss, sep));
    std::string temp = oss.str();
    return utils::remove_last(temp);
}

/** Returns the length of formatted string without color codes. */
template <typename String>
size_t length(String str) {
    size_t l = 0;
    for(size_t i=0; i < str.length(); ++i) {
        if(str.at(i) == '%'){
            if(i+1 < str.length() && str.at(i+1) != '%')
                i += 2;
            continue;
        }
        l++;
    }
    return l;
}

template <typename String>
String escape(String str)
{
    String escaped;
    typename String::size_type i;
    for(i=0; i < str.length(); ++i) {
        if(str[i] == '%')
            escaped.append(1, '%');
        escaped.append(1, str[i]);
    }
    return escaped;
}

template <typename String>
String remove_formatting(String str) {
    String lusikka;
    lusikka.reserve(str.length());

    typename String::size_type i;
    for(i=0; i < str.length(); ++i) {
        if(str[i] == '%') {
            if(i+1 < str.length() && str.at(i+1) != '%')
                i += 2;
            continue;
        }
        lusikka.append(1, str[i]);
    }
    return lusikka;
}

} // namespace strings

#endif // _STRINGS_H_
