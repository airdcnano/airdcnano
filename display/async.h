/* vim:set ts=4 sw=4 sts=4 et cindent: */
/*
 * * AirDC++ nano
 * * Copyright © 2013 maksis@adrenaline-network.com
 * *
 * * This program is free software; you can redistribute it and/or modify
 * * it under the terms of the GNU General Public License as published by
 * * the Free Software Foundation; either version 2 of the License, or
 * * (at your option) any later version.
 * *
 * * This program is distributed in the hope that it will be useful,
 * * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * * GNU General Public License for more details.
 * *
 * * You should have received a copy of the GNU General Public License
 * * along with this program; if not, write to the Free Software
 * * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * *
 * * Contributor(s):
 * *
 * */

#ifndef _ASYNC_H_
#define _ASYNC_H_

#include <client/stdinc.h>
#include <boost/signals2.hpp>

#include <functional>

namespace display {

class Async {

public:
    Async();
    void callAsync(std::function<void()> aF);

private:
    void handleAsync();
    boost::signals2::scoped_connection asyncConn;
    uint32_t async_id;
};

}

#endif

