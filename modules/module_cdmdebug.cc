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

#include <client/stdinc.h>
#include <client/DebugManager.h>

#include <core/events.h>
#include <core/log.h>

using namespace dcpp;

namespace modules {

	class CDMDebug : private DebugManagerListener {
	public:
		CDMDebug() {
			events::add_listener("command cdm", [this] { onCDM(); });
			events::add_listener("command quit", [this] { if (enabled) DebugManager::getInstance()->removeListener(this); });
		}

		void onCDM() {
			if (enabled) {
				core::Log::get()->log("CDM Debug messages disabled");
				DebugManager::getInstance()->removeListener(this);
			} else {
				core::Log::get()->log("CDM Debug messages enabled");
				DebugManager::getInstance()->addListener(this);
			}

			enabled = !enabled;
		}

		void on(DebugManagerListener::DebugCommand, const string& aLine, uint8_t aType, uint8_t aDirection, const string& ip) noexcept{
			string cmd;
			switch (aType) {
			case DebugManager::TYPE_HUB:
				//if (!showHubCommands)
				//	return;
				cmd = "Hub:";
				break;
			case DebugManager::TYPE_CLIENT:
				//if (!showTCPCommands)
				//	return;
				cmd = "Client (TCP):";
				break;
			case DebugManager::TYPE_CLIENT_UDP:
				//if (!showUDPCommands)
				//	return;
				cmd = "Client (UDP):";
				break;
			default: dcassert(0);
			}

			cmd += "\t";

			if (aDirection == DebugManager::INCOMING) {
				cmd += "[Incoming]";
			} else {
				cmd += "[Outgoing]";
			}

			cmd += "[" + ip + "]\t" + aLine;

			core::Log::get()->log(cmd, core::MT_DEBUG);
		}
	private:
		bool enabled = false;
	};

} // namespace modules

static modules::CDMDebug initialize;

