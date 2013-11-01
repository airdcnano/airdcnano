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

#ifndef _CORENANODC_H_
#define _CORENANODC_H_

#include <utils/instance.h>
#include <string>

namespace core {

/** This class is created in main() and this checks
 *  for pidfile or if the user is root and warns user
 *  about it. */
class Nanodc:
    public utils::Instance<core::Nanodc>
{
public:
    /** Constructor. */
    Nanodc(int argc, char **argv);

    /** Checks for pidfile and if the user is root and
     * then starts core::Manager.
     * @return 0 on clean shutdown, nonzero error code otherwise.*/
    int run();

    ~Nanodc();
private:
    /** Add signal handler for some signals. */
    void add_signal_handlers();

    /** The default signal handler for signals.
     * Prints stack trace if it's enabled. Calls abort() if m_crash is true. */
    void handle_crash(int signal);

    /** Check if a pidfile exists.
     * @return true if the file exists and the user didn't
     * use -d command line parameter */
    bool check_pidfile();

    /** Check if the user is root.
     * @return true if the user isn't root or used -root switch */
    bool check_root();

    std::string m_pidfile; //!< Holds the path to the pidfile
    int m_argc; //!< The number of command line arguments
    char **m_argv; //!< Command line arguments
    bool m_crash; //!< Set to true when the signal handler is called.

	void printHelp();
	void printVersion();
};

} // namespace core

#endif // _CORENANODC_H_

