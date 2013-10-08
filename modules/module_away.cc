/* vim:set ts=4 sw=4 sts=4 et cindent: */
/*
* AirDC++ nano
* Copyright © 2013 maksis@adrenaline-network.com
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

#include <core/events.h>
#include <core/log.h>
#include <display/manager.h>

#include <input/help_handler.h>

#include <client/typedefs.h>
#include <client/AirUtil.h>
#include <client/SettingsManager.h>
#include <client/TimerManager.h>

using namespace dcpp;

namespace modules {

class Away : private TimerManagerListener {
public:
	HelpHandler::CommandList commands = {
		{ "away", std::bind(&Away::handleAway, this) }
	};

	HelpHandler help;

	Away() : help(&commands, "Away mode") {
		events::add_listener("key pressed",
			std::bind(&Away::key_pressed, this));
		events::add_listener("client created", [this] { TimerManager::getInstance()->addListener(this); });
		events::add_listener("command quit", [this] { TimerManager::getInstance()->removeListener(this); });
	}

	void handleAway() {
		if (AirUtil::getAway()) {
			AirUtil::setAway(AWAY_OFF);
			display::Manager::get()->cmdMessage(STRING(AWAY_MODE_OFF));
		} else {
			AirUtil::setAway(AWAY_MANUAL);

			ParamMap sm;
			display::Manager::get()->cmdMessage(STRING(AWAY_MODE_ON) + " " + AirUtil::getAwayMessage(SETTING(DEFAULT_AWAY_MESSAGE), sm));
		}
	}

	void key_pressed() {
		lastTick = GET_TICK();
	}

	~Away() {

	}

	void on(TimerManagerListener::Second, uint64_t aTick) noexcept{
		checkAwayIdle(aTick);
	}

	void checkAwayIdle(uint64_t aTick) {
		if (SETTING(AWAY_IDLE_TIME) && (AirUtil::getAwayMode() <= AWAY_IDLE)) {
			bool awayIdle = (AirUtil::getAwayMode() == AWAY_IDLE);
			if ((static_cast<int>(aTick - lastTick) > SETTING(AWAY_IDLE_TIME) * 60 * 1000) ^ awayIdle) {
				awayIdle ? AirUtil::setAway(AWAY_OFF) : AirUtil::setAway(AWAY_IDLE);
			}
		}
	} 
private:
	uint64_t lastTick = 0;
};

} // namespace modules

static modules::Away initialize;

