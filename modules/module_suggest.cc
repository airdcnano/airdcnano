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

#include <sstream>
#include <stdexcept>
#include <functional>
#include <core/events.h>
#include <core/log.h>
#include <core/argparser.h>

#include <display/manager.h>
#include <ui/window_hub.h>
#include <input/help_handler.h>
#include <input/completion.h>

#include <boost/algorithm/cxx11/copy_if.hpp>
#include <boost/move/algorithm.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace modules {
	class Completion {
	public:
		Completion(input::Comparator* comp, StringList&& aItems, string aAppend = "") : append(move(aAppend)) {
			//m_items.erase(remove_if(m_items.begin(), m_items.end(), comp), m_items.end());
			//auto uniqueEnd = unique(items.begin(), items.end());
			if (comp)
				copy_if(aItems.begin(), aItems.end(), back_inserter(m_items), *comp);
			else
				m_items = move(aItems);

			sort(m_items.begin(), m_items.end());

			//fix whitespaces
			for (auto& i : m_items) {
				boost::replace_all(i, " ", "\\ ");
			}
		}

		optional<string> next() throw(std::out_of_range) {
			if (m_items.size() == 0)
				return nullptr;

			if (++m_currentItem > static_cast<int>(m_items.size() - 1) || m_currentItem == -1)
				m_currentItem = 0;

			return m_items.at(m_currentItem);
		}

		StringList m_items;
		int m_currentItem = -1;
		string append;
	};


	class Suggest {
	public:
		Suggest() {
			events::add_listener("key pressed",
				std::bind(&Suggest::key_pressed, this));
		}

		void key_pressed() {
			auto key = events::arg<wint_t>(1);
			if (key == 0x09) {
				handleTab();
			} else {
				c.reset(nullptr);
			}
		}

		void handleTab() {
			auto line = display::Window::m_input.str();
			auto pos = display::Window::m_input.get_pos();

			if (!c) {
				createComparator(line, pos);
			}

			if (!c)
				return;

			auto next = c->next();
			if (!next) {
				return;
			}

			// erase the last suggestion or the incomplete word
			line.erase(startPos, lastLen);
			if (line.empty()) {
				// add ": " or similar at the end
				*next += c->append;
			}

			line.insert(startPos, *next);

			// save the last suggestion length
			lastLen = (*next).length();
			display::Window::m_input.setText(line, false);
			display::Window::m_input.set_pos(startPos + lastLen);
		}

		void createComparator(const string& aLine, int pos) {
			bool isCommand = !aLine.empty() && aLine.front() == '/';

			// parse the words
			auto parser = core::ArgParser(aLine, pos);
			parser.parse(false);
			auto args = parser.getList();
			auto wordPos = parser.getWordListPos();


			if (static_cast<int>(aLine.length()) < pos) {
				// are we beyond the end of the line?
				wordPos = static_cast<int>(args.size() > 0 ? args.size() - 1 : 0);
			}

			if (isCommand && aLine.back() == ' ') {
				// get clean suggestions in those cases
				if (wordPos == args.size() - 1)
					wordPos++;
				args.push_back("");
			} else if (args.empty()) {
				wordPos = 0;
				args.push_back("");
			}

			// get the current word pos
			//core::Log::get()->log("wordpos: " + Util::toString(wordPos) + " pos: " + Util::toString(pos) + " linelen: " + 
			//	Util::toString(aLine.length()) + " args: " + 
			//	Util::toString(args.size()) + " startPos: " + Util::toString(parser.getWordStartPos()));

			if (wordPos == -1) {
				// we are somewhere within sequential whitespaces...
				return;
			}

			// get the word from the position
			//auto word = args[wordPos];

			auto mger = display::Manager::get();
			auto cur = mger->get_current_window();

			if (isCommand) {
				bool defaultSug = true;
				StringList suggest;
				if (args.size() <= 1 || wordPos == 0) {
					// list all commands
					HelpHandler::getCommandSuggestions(suggest);
				} else {
					// list suggestions based on the command
					auto word = args[0];
					auto c = HelpHandler::getCommand(word.erase(0, 1));
					if (c) {
						// drop the actual command
						args.erase(args.begin());
						wordPos--;

						//get the suggestions
						c->completionF(args, wordPos, suggest);
						defaultSug = c->defaultComp;
					}
				}

				auto comp = input::Comparator(args[wordPos]);
				c.reset(new Completion(defaultSug ? &comp : nullptr, move(suggest)));
			} else if (cur->get_type() == display::TYPE_HUBWINDOW) {
				// nick completion
				auto hub = static_cast<ui::WindowHub*>(cur);
				StringList suggest;
				hub->complete({ args[wordPos] }, wordPos, suggest);
				c.reset(new Completion(nullptr, move(suggest), ": "));
			}

			lastLen = args[wordPos].length();
			startPos = parser.getWordStartPos();
		}

		unique_ptr<Completion> c;
		size_t lastLen = 0;
		size_t startPos = 0;
	};

} // namespace modules

static modules::Suggest initialize;

