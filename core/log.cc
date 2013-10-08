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

#include <ui/window_log.h>
#include <core/log.h>
#include <utils/utils.h>
#include <utils/lock.h>
#include <iostream>

namespace core {

Log::Log()
{
    core::Settings *settings = core::Settings::get();

    m_logFilename = settings->find("log_filename", "%Y-%m-%d");
    m_logToFile = settings->find_bool("log_to_file", true);
    m_logTimestamp = settings->find("log_timestamp", "%H:%M:%S");

    m_realFilename = utils::time_to_string(m_logFilename);
    m_file.open(m_realFilename.c_str(), std::ios::out | std::ios::app);
}

void Log::update_config()
{
    core::Settings *settings = core::Settings::get();
    m_logFilename = settings->find("log_filename", "%Y-%m-%d");
    m_logToFile = settings->find_bool("log_to_file", true);
    m_logTimestamp = settings->find("log_timestamp", "%H:%M:%S");
}

void Log::log(const std::string &message, MessageType mt /* = MT_MSG */)
{
    if(mt == MT_DEBUG && !LOG_DEBUG)
        return;

    {
        utils::Lock lock(m_mutex);
        if(m_logToFile) {
            std::string temp = m_realFilename;
            m_realFilename = utils::time_to_string(m_logFilename);

            // day or format string changed?
            if(temp != m_realFilename) {
                m_file.close();
                m_file.open(m_realFilename.c_str(), std::ios::out | std::ios::app);
            }

            m_file << utils::time_to_string(m_logTimestamp) 
                   << " " << message << std::endl;
        }
    }

    m_logSig(message, mt);
}

Log::~Log()
{
    m_file.close();
}

} // namespace core

