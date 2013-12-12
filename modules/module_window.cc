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

#include <utils/utils.h>
#include <core/events.h>
#include <core/log.h>
#include <core/argparser.h>
#include <display/manager.h>
#include <input/help_handler.h>
#include <input/manager.h>

#include <ui/window_favorites.h>
#include <ui/window_log.h>
#include <ui/window_publichubs.h>
#include <ui/window_transfers.h>
#include <ui/window_queue.h>

#include <client/Util.h>

using namespace dcpp;

namespace modules {

class Window
{
public:
	template <class T>
	static void handleOpenTab(display::Type aType) {
		auto mger = display::Manager::get();
		auto it = mger->find(aType);
		if (it == mger->end()) {
			auto w = new T();
			mger->push_back(w);
		}

		mger->set_current(it);
	}

	HelpHandler::CommandList commands = {
		{ "clear", std::bind(&Window::handleClear, this), nullptr },
		{ "window", std::bind(&Window::window_callback, this), COMPLETION(Window::handleSuggestWindow) },
		{ "open", std::bind(&Window::handleOpen, this), COMPLETION(Window::handleSuggestOpen) },
		{ "wc", std::bind(&Window::close, this), nullptr }
	};

	HelpHandler help;

    Window() : help(&commands, "Window") {
		events::add_listener("key pressed", std::bind(&Window::keyPressed, this));
	}

	void keyPressed() {
		auto key = events::arg<wint_t>(1);
		if (key == INPUT_CTRL('P')) {
			handleOpenTab<ui::WindowHubs>(display::TYPE_HUBLIST);
		} else if (key == INPUT_CTRL('H')) {
			handleOpenTab<ui::WindowFavorites>(display::TYPE_FAVORITES);
		} else if (key == INPUT_CTRL('T')) {
			handleOpenTab<ui::WindowTransfers>(display::TYPE_TRANSFERS);
		} else if (key == INPUT_CTRL('S')) {
			handleOpenTab<ui::WindowLog>(display::TYPE_LOGWND);
		} else if (key == INPUT_CTRL('Q')) {
			handleOpenTab<ui::WindowQueue>(display::TYPE_QUEUE);
		}
	}

	void handleOpen() {
		if (events::args() == 0) {
			display::Manager::get()->cmdMessage("Usage: /open <hublist|transfers|syslog|favorites>");
			return;
		}

		auto w = events::arg<std::string>(0);
		if (w == "hublist") {
			handleOpenTab<ui::WindowHubs>(display::TYPE_HUBLIST);
		} else if (w == "favorites") {
			handleOpenTab<ui::WindowFavorites>(display::TYPE_FAVORITES);
		} else if (w == "transfers") {
			handleOpenTab<ui::WindowTransfers>(display::TYPE_TRANSFERS);
		} else if (w == "syslog") {
			handleOpenTab<ui::WindowLog>(display::TYPE_LOGWND);
		} else if (w == "queue") {
			handleOpenTab<ui::WindowQueue>(display::TYPE_QUEUE);
		}
	}

	void handleSuggestOpen(const StringList& aArgs, int pos, StringList& suggest_, bool& appendSpace_) {
		if (pos == 1) {
			StringList ret{ "hublist", "transfers", "syslog", "favorites", "queue" };
			suggest_.swap(ret);
		}
	}

	void handleSuggestWindow(const StringList& aArgs, int pos, StringList& suggest_, bool& appendSpace_) {
		if (pos == 1) {
			StringList ret = { "move", "list", "close", "prev", "next" };
			suggest_.swap(ret);
			appendSpace_ = true;
		} else if (pos == 2) {
			if (aArgs[1] == "move") {
				StringList ret = { "prev", "next", "first", "last" };
				suggest_.swap(ret);
				appendSpace_ = false;
			}
		}
	}

	void handleClear() {
		auto cur = *display::Manager::get()->get_current();
		if (cur->get_type() == display::TYPE_LOGWND || cur->get_type() == display::TYPE_HUBWINDOW || cur->get_type() == display::TYPE_PRIVMSG)
			static_cast<display::ScrolledWindow*>(cur)->clear();
	}

    void window_callback() {
		if (events::args() == 0) {
			display::Manager::get()->cmdMessage("Usage: /window <list|close|move|prev|next|[0-X]>");
			return;
		}

		core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
		parser.parse();
		auto command = parser.arg(0);
        auto mger = display::Manager::get();
        auto current = mger->get_current();

        if(command == "close") {
			close();
        } else if (command == "move") {
			if (parser.args() < 2) {
				mger->cmdMessage("Usage: /window move <first|last|prev|next|[0-X]>");
				return;
			}

			int newPos = 0;
			auto pos = parser.arg(1);
			if (pos == "next") {
				auto newCur = current == mger->end() - 1 ? mger->begin() : current + 1;
				newPos = distance(mger->begin(), newCur);
			} else if (pos == "prev") {
				auto newCur = current == mger->begin() ? mger->end() - 1 : current - 1;
				newPos = distance(mger->begin(), newCur);
			} else if (pos == "first") {
				newPos = 0;
			} else if (pos == "last") {
				newPos = distance(mger->begin(), mger->end()-1);
			} else {
				newPos = Util::toInt(parser.arg(1));
				if (newPos < 1 || newPos > static_cast<int>(mger->size())) {
					mger->cmdMessage("Invalid position");
					return;
				}

				newPos--;
			}

			auto p = mger->begin() + newPos;
			if (p < current) {
				std::rotate(p, current, current+1);
			} else if (current < p) {
				std::rotate(current, current+1, p + 1);
			}
			mger->set_current(mger->begin() + newPos);
		} else if (command == "next") {
			mger->next();
		} else if(command == "prev") {
			mger->prev();
		} else if (command == "list") {
			mger->cmdMessage("Open windows:");
			int pos = 1;
			for (auto i = mger->begin(); i != mger->end(); ++i, ++pos) {
				mger->cmdMessage(Util::toString(pos) + ": " + (*i)->get_name());
			}
		} else if (!command.empty() && (Util::toInt(command) > 0 && Util::toInt(command) <= static_cast<int>(mger->size()))) {
			auto pos = Util::toInt(command) - 1;
			if (pos <= static_cast<int>(mger->size())) {
				auto newCurrent = mger->begin() + pos;
				mger->set_current(newCurrent);
			}
		}
    }

	void close() {
		auto it = display::Manager::get()->get_current();
		display::Manager::get()->remove(*it);
	}
};
} // namespace modules

static modules::Window initialize;


