/* vim:set ts=4 sw=4 sts=4 et cindent: */
/*
 * * AirDC++ nano
 * * Copyright © 2015 maksis@adrenaline-network.com
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

#include <display/async.h>


#include <core/events.h>
#include <client/Util.h>

using namespace dcpp;

namespace display {

Async::Async() : async_id(Util::rand()) {
    asyncConn = events::add_listener("async" + Util::toString(async_id), std::bind(&Async::handleAsync, this));
}

void Async::callAsync(std::function<void()> aF) {
    events::emit("async" + Util::toString(async_id), aF);
}

void Async::handleAsync() {
    events::arg<std::function<void()>>(0)();
}

}
