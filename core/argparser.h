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

#ifndef _ARGPARSER_H_
#define _ARGPARSER_H_

#include <string>
#include <vector>
#include <utils/instance.h>
#include <functional>
#include <fstream>

namespace core {

/** ArgParser is used to parse command line options. */
class ArgParser
{
public:
    /** Constructs new parser with the given command line. */
	ArgParser(const std::string &line, size_t cursorPos = 0);

    /** Parse the given command line. */
    void parse(bool unescapeWhitespaces = true);

    /** Get nth argument (starting from 0). */
    std::string arg(unsigned int n) { return m_args.at(n); }

    /** Get the number of arguments. */
    size_t args() { return m_args.size(); }

    /** Returns everything after nth space character. Eg. if the line is
     * "aa bb cc" get_text(1) returns "bb cc". */
    std::string get_text(unsigned int n);

    /** Destructor. */
    ~ArgParser();

	const std::vector<std::string>& getList() const { return m_args; }
	int getWordListPos() const { return wordListPos; }
	int getWordStartPos() const { return wordStartPos; }
private:
    std::string m_line; //!< The original command line
    std::vector<std::string> m_args;
	size_t cursorPos;

	// position of the word under the cursor in the arg list
	int wordListPos = -1;

	// start position of the word under the cursor in the original string
	int wordStartPos = 0;
};

} // namespace core

#endif // _ARGPARSER_H_


