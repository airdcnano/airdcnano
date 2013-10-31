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

#ifndef _WINDOWLOG_H_
#define _WINDOWLOG_H_

#include <string>
#include <utils/instance.h>
#include <core/log.h>
#include <display/scrolled_window.h>

#include <client/stdinc.h>
#include <client/LogManagerListener.h>

using namespace dcpp;

namespace ui {

class WindowLog:
    public display::ScrolledWindow,
    public utils::Instance<ui::WindowLog>,
    public LogManagerListener
{
public:
    WindowLog();

    void handle_line(const std::string &line);

    /** Log message from core::Log. */
    void on_log(const std::string &message, core::MessageType mt = core::MT_MSG);

    /** Log message from LogManager. */
	void on(LogManagerListener::Message, time_t, const std::string &message, uint8_t) noexcept;

    /** Override close and don't allow closing the log window. */
    void close();

    ~WindowLog();

	void onClientCreated();
};

} // namespace ui

#endif // _WINDOWLOG_H_
