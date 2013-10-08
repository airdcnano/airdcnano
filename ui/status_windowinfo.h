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

#include <display/status_bar.h>
#include <display/status_item.h>
#include <display/window.h>
#include <display/manager.h>
#include <string>

#ifndef _STATUSWINDOWINFO_H_
#define _STATUSWINDOWINFO_H_

namespace ui {

class StatusWindowInfo:
    public display::StatusItem
{
public:
    StatusWindowInfo();
    virtual void update();
};

} // namespace ui

#endif // _STATUSWINDOWINFO_H_
