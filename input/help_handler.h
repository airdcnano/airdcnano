/* vim:set ts=4 sw=4 sts=4 et cindent: */
/*
* nanodc - The ncurses DC++ client
* Copyright � 2005-2006 Markus Lindqvist <nanodc.developer@gmail.com>
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

#ifndef _HELPHANDLER_H_
#define _HELPHANDLER_H_

#include <functional>

#include <core/events.h>
#include <display/window.h>
#include <client/stdinc.h>
#include <input/completion.h>

using namespace std;
using namespace dcpp;

#define COMPLETION(func) (std::bind(&func, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4))

class HelpHandler {
public:
	typedef function < void(const vector<string>& /*arguments*/, int /* current pos */, vector<string>& /*complete*/, bool& /*appendSpace*/)> CommandCompletionF;

	struct Command {
		Command(string aCommand, events::EventFunc aF, CommandCompletionF c = nullptr, bool aDefaultComp = true) : command(move(aCommand)), eF(aF), completionF(c), defaultComp(aDefaultComp) {}
		string command;
		events::EventFunc eF;
		CommandCompletionF completionF;
		bool defaultComp;
	};

	class CommandCompare {
	public:
		CommandCompare(const string& compareTo) : a(compareTo) {}
		bool operator()(const Command& c) {
			return compare(a, c.command) == 0;
		}
	protected:
		CommandCompare& operator=(const CommandCompare&);
		const string& a;
	};

	typedef vector<Command> CommandList;
	HelpHandler(const CommandList* aHandlers, const string& aTitle, display::Window* aWindow = nullptr);
	~HelpHandler();

	const CommandList* handlers;
	display::Window* window;


	// Global
	static void getCommandSuggestions(vector<string>& suggest_);
	static const Command* getCommand(const string& aName);
private:
	void handleHelp();
	string title;
	vector <boost::signals2::connection> conns;
	void handleCommand(events::EventFunc& aEvent);

	static vector<HelpHandler*> list;
};

#endif


