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

//#include <input/completion.h>
#include <display/manager.h>
#include <ui/window_hub.h>
#include <input/help_handler.h>
#include <client/StringTokenizer.h>

#include <boost/algorithm/cxx11/copy_if.hpp>
#include <boost/move/algorithm.hpp>

namespace modules {
	class Comparator {
		/* Case insensitive string comparison classes */
		public:
			Comparator(const string& compareTo) : a(compareTo) {}
			virtual bool operator()(const string& p) { 
				return Util::strnicmp(p.c_str(), a.c_str(), a.length()) == 0; 
			}
		protected:
			Comparator& operator=(const Comparator&);
			const string a;
	};

	class PrefixComparator : public Comparator {
	public:
		string stripNick(const string& nick) const {
			if (nick.substr(0, 1) != "[") return nick;
			string::size_type x = nick.find(']');
			string ret;
			// Avoid full deleting of [IMCOOL][CUSIHAVENOTHINGELSETHANBRACKETS]-type nicks
			if ((x != string::npos) && (nick.substr(x + 1).length() > 0)) {
				ret = nick.substr(x + 1);
			} else {
				ret = nick;
			}
			return ret;
		}

		PrefixComparator(const string& compareTo) : Comparator(stripNick(compareTo)) {}
		bool operator()(const string& p) { 
			return Util::strnicmp(stripNick(p).c_str(), a.c_str(), a.length()) == 0;
		}
	};

	class Completion {
	public:
		template<class T>
		Completion(T&& c, StringList&& items, string aAppend = "") : append(move(aAppend)) {
			//m_items.erase(remove_if(m_items.begin(), m_items.end(), comp), m_items.end());
			auto uniqueEnd = unique(items.begin(), items.end());
			copy_if(items.begin(), uniqueEnd, back_inserter(m_items), c);
			sort(items.begin(), items.end());
		}

		string next() throw(std::out_of_range) {
			if (m_items.size() == 0)
				throw std::out_of_range("Completion::next()");

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
				// give new suggestions on the next tab
				c.reset(nullptr);
			}
		}

		void handleTab() {
			auto line = display::Window::m_input.str();
			auto p = line.rfind(" ");

			if (!c) {
				bool isCommand = !line.empty() && line.front() == '/';

				// get the last word from the line
				auto word = p != string::npos ? line.substr(p + 1, line.length() - p - 1) : line;

				auto mger = display::Manager::get();
				auto cur = mger->get_current_window();

				if (isCommand) {
					StringList suggest;
					auto args = StringTokenizer<string>(line.substr(1, line.length()-1), " ").getTokens();
					if (args.size() <= 1 && line.back() != ' ') {
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
							if (h->completionF && boost::find_if(*h->handlers, [&](const HelpHandler::Command& c) { return c.command == args[0]; }) != h->handlers->end()) {
								h->completionF(args, suggest);
								break;
							}
						}
					}

					c.reset(new Completion(Comparator(word), move(suggest)));
				} else if (cur->get_type() == display::TYPE_HUBWINDOW) {
					auto hub = static_cast<ui::WindowHub*>(cur);
					c.reset(new Completion(PrefixComparator(word), hub->complete(word), ": "));
				}

				lastLen = word.length();
			}

			if (!c)
				return;

			try {
				auto next = c->next();
				if (!next.empty()) {
					// erase the last suggestion or the incomplete word
					line.erase(line.length() - lastLen, lastLen);
					if (line.empty()) {
						// add ": " or similar at the end
						next += c->append;
					}

					line += next;

					// save the last suggestion length
					lastLen = next.length();
					display::Window::m_input.setText(line, false);
				}
			} catch (...) {}
		}

		unique_ptr<Completion> c;
		size_t lastLen;
	};

} // namespace modules

static modules::Suggest initialize;

