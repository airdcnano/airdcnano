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

#include <client/stdinc.h>

using namespace dcpp;

vector<HelpHandler*> HelpHandler::list;

HelpHandler::HelpHandler(const CommandList* aHandlers, const string& aTitle, display::Window* aWindow) : 
		handlers(aHandlers), title(aTitle), window(aWindow) {


	for (const auto& c : *handlers) {
		conns.push_back(events::add_listener("command " + c.command, std::bind(&HelpHandler::handleCommand, this, c.eF)));
	}

	 
	events::add_listener("command help",
		std::bind(&HelpHandler::handleHelp, this));

	list.push_back(this);
}

void HelpHandler::handleCommand(events::EventFunc& aEvent) {
	if (window && display::Manager::get()->get_current_window() != window) {
		return;
	}

	aEvent();
}

HelpHandler::~HelpHandler() {
	if (window) {
		for (auto& c : conns) {
			c.disconnect();
		}

		list.erase(find(list.begin(), list.end(), this));
	}
}
 
void HelpHandler::handleHelp() {
	if (window && display::Manager::get()->get_current_window() != window) {
		return;
	}

	string help = "---[ " + title + ":";
	for (const auto& c : *handlers) {
		help += " /" + c.command;
	}
	help += " ]---";

	display::Manager::get()->cmdMessage(help);
}

void HelpHandler::getCommandSuggestions(vector<string>& suggest_) {
	for (auto& h : HelpHandler::list) {
		if (h->window && display::Manager::get()->get_current_window() != h->window) {
			continue;
		}

		for (auto& c : *h->handlers) {
			suggest_.push_back("/" + c.command);
		}
	}

	// add the common commands
	suggest_.push_back("/quit");
	suggest_.push_back("/help");
}

const HelpHandler::Command* HelpHandler::getCommand(const string& aName) {
	for (auto& h : list) {
		if (h->window && display::Manager::get()->get_current_window() != h->window) {
			continue;
		}

		auto p = boost::find_if(*h->handlers, HelpHandler::CommandCompare(aName));
		if (p != h->handlers->end()) {
			return &(*p);
		}
	}

	return nullptr;
}