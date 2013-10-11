/* vim:set ts=4 sw=4 sts=4 et cindent: */
/*
* AirDC++ nano
* Copyright � 2013 maksis@adrenaline-network.com
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

//#include <input/completion.h>
#include <display/manager.h>
#include <ui/window_hub.h>
#include <input/help_handler.h>
#include <client/StringTokenizer.h>
#include <input/completion.h>

#include <boost/algorithm/cxx11/copy_if.hpp>
#include <boost/move/algorithm.hpp>

namespace modules {
	class Completion {
	public:
		Completion(input::Comparator* comp, StringList&& items, string aAppend = "") : append(move(aAppend)) {
			//m_items.erase(remove_if(m_items.begin(), m_items.end(), comp), m_items.end());
			//auto uniqueEnd = unique(items.begin(), items.end());
			if (comp)
				copy_if(items.begin(), items.end(), back_inserter(m_items), *comp);
			else
				m_items = move(items);

			sort(items.begin(), items.end());
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


			if (!c) {
				createComparator(line);
			}

			if (!c)
				return;

			auto next = c->next();
			if (!next) {
				return;
			}

			// erase the last suggestion or the incomplete word
			line.erase(line.length() - lastLen, lastLen);
			if (line.empty()) {
				// add ": " or similar at the end
				*next += c->append;
			}

			line += *next;

			// save the last suggestion length
			lastLen = (*next).length();
			display::Window::m_input.setText(line, false);
		}

		void createComparator(const string& aLine) {
			lastLine = aLine;
			bool isCommand = !aLine.empty() && aLine.front() == '/';

			// get the last word from the line
			auto p = aLine.rfind(" ");
			auto word = p != string::npos ? aLine.substr(p + 1, aLine.length() - p - 1) : aLine;

			auto mger = display::Manager::get();
			auto cur = mger->get_current_window();

			if (isCommand) {
				bool defaultSug = true;
				StringList suggest;
				auto args = StringTokenizer<string>(aLine.substr(1, aLine.length() - 1), " ").getTokens();
				if (aLine.back() == ' ') {
					// we want suggestions also in this case
					args.push_back("");
				}


				if (args.size() <= 1) {
					// list all commands
					for (auto& h : HelpHandler::list) {
						if (h->window && cur != h->window)
							continue;

						for (auto& c : *h->handlers) {
							suggest.push_back(c.command);
						}
					}

					word.erase(0, 1);

					// add the common commands
					suggest.push_back("quit");
					suggest.push_back("help");
				} else {
					// list suggestions based on the command
					for (auto& h : HelpHandler::list) {
						auto p = boost::find_if(*h->handlers, [&](const HelpHandler::Command& c) { return c.command == args[0]; });
						if (p != h->handlers->end()) {
							if (!(*p).completionF)
								return;

							args.erase(args.begin()); // drop the actual command
							(*p).completionF(args, suggest);
							defaultSug = (*p).defaultComp;
							break;
						}
					}
				}

				auto comp = input::Comparator(word);
				c.reset(new Completion(defaultSug ? &comp : nullptr, move(suggest)));
			} else if (cur->get_type() == display::TYPE_HUBWINDOW) {
				auto hub = static_cast<ui::WindowHub*>(cur);
				StringList suggest;
				hub->complete({ word }, suggest);
				c.reset(new Completion(nullptr, move(suggest), ": "));
			}

			lastLen = word.length();
		}

		unique_ptr<Completion> c;
		size_t lastLen;
		string lastLine;
	};

} // namespace modules

static modules::Suggest initialize;

