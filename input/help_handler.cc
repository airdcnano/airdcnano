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

#include <sstream>
#include <core/settings.h>
#include <core/events.h>
#include <display/manager.h>
#include <utils/utils.h>
#include <core/log.h>
#include <input/text_input.h>
#include <input/manager.h>
#include <input/help_handler.h>
#include <ui/window_hub.h>

HelpHandler::HelpHandler(const CommandList* aHandlers, const string& aTitle) : handlers(aHandlers), title(aTitle) {
	for (const auto& c : *handlers) {
		events::add_listener("command " + c.command, c.eF);
	}

	 
	events::add_listener("command help",
		std::bind(&HelpHandler::handleHelp, this));
}
 
void HelpHandler::handleHelp() {
	string help = "---[ " + title + ":";
	for (const auto& c : *handlers) {
		help += " /" + c.command;
	}
	help += " ]---";

	display::Manager::get()->cmdMessage(help);
}