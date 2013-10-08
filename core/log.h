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

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <string>
#include <vector>
#include <utils/instance.h>
#include <utils/mutex.h>
#include <boost/signals2.hpp>
#include <functional>
#include <fstream>

namespace core {

/** Whether to log debug messages. */
const bool LOG_DEBUG = true;

/** Different message types. */
enum MessageType {
    MT_MSG, //!< Default, normal message
    MT_DEBUG //!< Debug messages, only shown when LOG_DEBUG is true
};

/** A general logger class.
 * Reads the following settings using core::Settings
 *  - log_filename with default value of "%Y-%m-%d"
 *  - log_to_file with default value of "true"
 *  - log_timestamp with default value of "%H:%M:%S"
 */
class Log:
    public utils::Instance<core::Log>
{
public:
    /** Constructor. Opens the output file. */
    Log();

    /** Log a message. */
    void log(const std::string &message, MessageType mt = MT_MSG);

    /** Add a log event listener which is called when a message is logged. */
    boost::signals2::connection add_listener(
        std::function<void (const std::string &, MessageType)> listener
    ) {
        return m_logSig.connect(listener);
    }

    /** Destructor. Closes the output file. */
    virtual ~Log();
private:
    /** Called when settings are changed. */
    void update_config();

    std::ofstream m_file; //!< Output file
    std::string m_logFilename; //!< Format of the log file name
    std::string m_realFilename; //!< The real file name
    std::string m_logTimestamp; //!< Timestamp format in log file
    bool m_logToFile; //!< Whether to log to file
    boost::signals2::signal<void (const std::string &, MessageType)> m_logSig;

    utils::Mutex m_mutex;
};

} // namespace core

#endif // _LOGGER_H_


