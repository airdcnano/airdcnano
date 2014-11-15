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
#include <client/ShareManager.h>
#include <client/SettingsManager.h>
#include <client/version.h>

#include <input/help_handler.h>
#include <display/manager.h>

using namespace dcpp;

namespace modules {

class Motd {
public:
	HelpHandler::CommandList commands = {
		{ "motd", std::bind(&Motd::handleMotd, this), nullptr }
	};

	HelpHandler help;

	Motd() : help(&commands, "Motd") {

	}

	void handleMotd() {
		uint64_t totalAge = 0;
		size_t totalFiles = 0, lowerCaseFiles = 0, totalDirs = 0, totalStrLen = 0, roots = 0;
		int64_t totalSize = 0;

		ShareManager::getInstance()->countStats(totalAge, totalDirs, totalSize, totalFiles, lowerCaseFiles, totalStrLen, roots);

		auto log = [](const string& msg) {
			display::Manager::get()->cmdMessage(msg);
		};

		log("");
		log(shortVersionString + " (" + Util::formatTime("%c", getVersionDate()) + ")");
		log("Running on " + Util::getOsVersion());
		log("AirDC++ core: 2.90 Git");
		log("");
		log("Total transferred (up/down): " + Util::formatBytes(SETTING(TOTAL_UPLOAD)) + " / " + Util::formatBytes(SETTING(TOTAL_DOWNLOAD)));
		log("Shared: " + Util::formatBytes(totalSize) + " (" + Util::toString(totalFiles) + " files in " + Util::toString(totalDirs) + " directories)");
		//log("Average file age: " + totalFiles == 0 ? "-" : Util::formatTime(GET_TIME() - (totalAge / totalFiles), false, true) + "");
		log("");
		log("Homepage http://www.airdcpp.net");
		log("User guide http://www.airdcpp.net/nano-guide");
		log("");
	}
};

}

static modules::Motd initialize;
