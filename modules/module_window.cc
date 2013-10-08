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
#include <display/manager.h>
#include <input/help_handler.h>

#include <ui/window_favorites.h>
#include <ui/window_log.h>
#include <ui/window_publichubs.h>
#include <ui/window_transfers.h>

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
		{ "favorites", [this] { handleOpenTab<ui::WindowFavorites>(display::TYPE_FAVORITES); } },
		{ "syslog", [this] { handleOpenTab<ui::WindowLog>(display::TYPE_LOGWND); } },
		{ "hubs", [this] { handleOpenTab<ui::WindowHubs>(display::TYPE_HUBLIST); } },
		{ "transfers", [this] { handleOpenTab<ui::WindowTransfers>(display::TYPE_TRANSFERS); } },
		{ "window", std::bind(&Window::window_callback, this) },
		{ "wc", std::bind(&Window::close, this) }
	};

	HelpHandler help;

    Window() : help(&commands, "Window") {
        /*events::add_listener("command window",
            std::bind(&Window::window_callback, this));

        events::add_listener("command wc",
			std::bind(&Window::close, this));*/
    }

    void window_callback() {
		if (events::args() == 0) {
			display::Manager::get()->cmdMessage("Usage: /window <close|prev|next|[0-9]>");
			return;
		}

        auto command = events::arg<std::string>(0);
        auto mger = display::Manager::get();
        //core::Log::get()->log(command);
        auto current = mger->get_current();

        if(command == "close") {
			close();
        } else if (command == "move") {
			core::Log::get()->log("Not implemented");
		} else if (command == "next" || command == "prev") {
			if (command == "next")
				current = (current == mger->end() - 1 ? mger->begin() : current + 1);
			else
				current = (current == mger->begin() ? mger->end() - 1 : current - 1);

			mger->set_current(current);
		} else if (!command.empty() && (Util::toInt(command) >= 0 && Util::toInt(command) <= 9)) {
			auto pos = Util::toInt(command) - 1;
			if (pos <= mger->size()) {
				auto current = mger->begin() + pos;
				mger->set_current(current);
			}
		} /*else {
			display::Manager::get()->cmdMessage("Usage: /window <close|prev|next|[0-9]>");
		}*/
    }

	void close() {
		auto it = display::Manager::get()->get_current();
		display::Manager::get()->remove(*it);
	}

   /* void print_help(const Parameter &param) {
        if(param.get_command() == "window") {
            core::Log::get()->log("Usage: /window n|move|close");
        }
    }

    std::vector<std::string> complete(int n, const Parameter &param)
    {
        if(param.get_command() == "window") {
            if(n == 0) {
                return utils::make_vector(4, "move", "close", "next", "prev");
            }
            else if(n == 1) {
                if(param.get_param(0) == "move") {
                    return utils::make_vector(4, "next", "prev", "first", "last");
                }
            }
        }
        return std::vector<std::string>();
    }*/
//#endif
};
} // namespace modules

static modules::Window initialize;


