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

#ifndef _STATUSHASH_H_
#define _STATUSHASH_H_

#include <client/stdinc.h>
#include <client/DCPlusPlus.h>
#include <client/TimerManager.h>
#include <client/HashManager.h>
#include <display/status_bar.h>
#include <display/status_item.h>
#include <string>
#include <sstream>

using namespace dcpp;

namespace ui {

class StatusHash:
    public display::StatusItem,
    public TimerManagerListener
{
public:
    StatusHash();
    void update();
    void on(Second, uint64_t) noexcept;
    ~StatusHash();
private:
    int64_t m_startBytes;
};

} // namespace ui

#endif // _STATUSHASH_H_
