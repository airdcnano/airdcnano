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

#ifndef _SIGNAL_HANDLER_H_
#define _SIGNAL_HANDLER_H_

#include <sys/signal.h>
#include <functional>
#include <map>
#include <string>

namespace utils {

/** Signal handler class. */
class SignalHandler
{
public:
    typedef unsigned int uint32_t;
    typedef std::function<void (uint32_t)> SigHandler;

    /** Ignore the signal.
     * @param num The signal to ignore*/
    static void ignore(uint32_t num);

    /** Add signal handler for a signal. If the signal already
     * has a handler, it'll be replaced.
     * @param signal The signal to handle
     * @param handler The signal handler */
    static void add_handler(uint32_t signal, SigHandler handler);

    /** Returns a string representation of the signal.
     * @param signal The signal
     * @return Name of the signal or "unknown"*/
    static const std::string to_string(uint32_t signal);
private:
    /** Calls the correct signal handler. */
    static void handle(int signal);
    static std::map<uint32_t, SigHandler> m_handlers; //!< List of signal handlers
};

} // utils

#endif // _SIGNAL_HANDLER_H_
