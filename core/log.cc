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
}

void Log::log(const std::string &message, MessageType mt /* = MT_MSG */)
{
    if(mt == MT_DEBUG && !LOG_DEBUG)
        return;

    m_logSig(message, mt);
}

Log::~Log()
{
}

} // namespace core

