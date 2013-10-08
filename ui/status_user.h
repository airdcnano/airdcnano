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

#include <client/stdinc.h>

#include <core/log.h>
#include <display/status_bar.h>
#include <display/status_item.h>
#include <display/window.h>
#include <display/manager.h>
#include <ui/window_hub.h>
#include <ui/window_privatemessage.h>
#include <core/settings.h>
#include <vector>
#include <string>

#ifndef _STATUSUSER_H_
#define _STATUSUSER_H_

namespace ui {

class StatusUser:
    public display::StatusItem
{
public:
    StatusUser();
    virtual void update();
};

} // namespace ui

#endif // _STATUSUSER_H_
