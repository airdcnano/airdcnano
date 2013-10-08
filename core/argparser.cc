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

#include <core/argparser.h>
#include <core/log.h>
#include <utils/utils.h>

namespace core {

ArgParser::ArgParser(const std::string &line):
    m_line(line)
{
}

void ArgParser::parse()
{
    bool quote = false;
    unsigned int end = 0;
    unsigned int start = 0;

    do {
        char ch = m_line[end];

        /* end of a word if we aren't inside quotes */
        if(ch == ' ' && !quote) {
            std::string arg = m_line.substr(start, end-start);

            /* skip whitespace */
            while(m_line[end+1] == ' ' && end++);

            if(!arg.empty()) {
               // core::Log::get()->log("'" + arg + "'");
                m_args.push_back(arg);
            }
            start = end+1;
        }
        /* start of quoted string */
        else if(ch == '"' && !quote) {
            quote = true;
            /* don't include the quotation mark */
            start++;
        }
        /* end of quoted string */
        else if(ch == '"' && quote) {
            std::string arg = m_line.substr(start, end-start);
            //core::Log::get()->log("'" + arg + "'");
            quote = false;
            start = end+1;
        }
    } while(end++ < m_line.length());

    std::string arg = m_line.substr(start, end-start);
    //core::Log::get()->log("'" + arg + "'");

    /* save rest of the string if there's any */
    if(!arg.empty()) {
        m_args.push_back(arg);
    }
}

std::string ArgParser::get_text(unsigned int n)
{
    unsigned int start = 0;
    while(n--) {
        unsigned int pos = m_line.find(' ', start);
        if(pos != std::string::npos)
            start = pos+1;
    }
    return m_line.substr(start);
}

ArgParser::~ArgParser()
{
}

} // namespace core

