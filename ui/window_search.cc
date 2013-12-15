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

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <functional>
#include <ui/window_search.h>

#include <utils/utils.h>
#include <utils/ncurses.h>
#include <input/completion.h>

#include <client/DirectoryListingManager.h>
#include <client/GeoManager.h>
#include <client/QueueManager.h>
#include <client/StringTokenizer.h>

namespace ui {

WindowSearch::WindowSearch(const std::string &aStr):
	ListView(display::TYPE_SEARCHWINDOW, aStr)
{
    SearchManager::getInstance()->addListener(this);

    set_title("Search");

    // hidden column for identifying the rows
    insert_column(new display::Column("User-TTH"));
	insert_column(new display::Column("User", 10, 15, 30));
    insert_column(new display::Column("Slots", 6, 7, 8));
    insert_column(new display::Column("Size", 11, 11, 15));
	insert_column(new display::Column("Date", 11, 11, 11));
    insert_column(new display::Column("File name", 50, 200, 300));
    resize();

	if (!aStr.empty())
		search(aStr);

    // download
    m_bindings['d'] = std::bind(&WindowSearch::download, this, SETTING(DOWNLOAD_DIRECTORY));

    // download to..
    m_bindings['D'] = std::bind(&WindowSearch::set_property, this, PROP_FILETARGET);

    // download directory
    m_bindings['s'] = std::bind(&WindowSearch::download, this, SETTING(DOWNLOAD_DIRECTORY));

    // download directory to..
    m_bindings['S'] = std::bind(&WindowSearch::set_property, this, PROP_DIRECTORYTARGET);

    // browse
	m_bindings['b'] = std::bind(&WindowSearch::handleGetList, this);
    // match queue
	m_bindings['M'] = std::bind(&WindowSearch::handleMatchQueue, this);
    // search
    m_bindings['r'] = std::bind(&WindowSearch::search, this, std::string());

    m_bindings['l'] = std::bind(&WindowSearch::toggle_slots, this);
    m_bindings['n'] = std::bind(&WindowSearch::set_property, this, PROP_MINSIZE);
    m_bindings['m'] = std::bind(&WindowSearch::set_property, this, PROP_MAXSIZE);
    m_bindings['e'] = std::bind(&WindowSearch::set_property, this, PROP_EXTENSION);
    m_bindings['f'] = std::bind(&WindowSearch::set_property, this, PROP_SEARCHFILTER);
    m_bindings['/'] = m_bindings['f'];
    m_bindings['c'] = std::bind(&WindowSearch::free_results, this);
}

void WindowSearch::toggle_slots() {
	m_freeSlots = !m_freeSlots; 
	create_list();
	set_prompt_timed("Free slots only: " + string(m_freeSlots ? "enabled" : "disabled"));
}

void WindowSearch::handleGetList() {
	if (get_selected_row() == -1)
		return;

	try {
		QueueManager::getInstance()->addList(get_user(), QueueItem::FLAG_CLIENT_VIEW, get_result()->getPath());
	} catch (...) {

	}
}

void WindowSearch::handleMatchQueue() {
	if (get_selected_row() == -1)
		return;

	try {
		QueueManager::getInstance()->addList(get_user(), QueueItem::FLAG_MATCH_QUEUE);
	} catch (...) {

	}
}

SearchResultPtr WindowSearch::get_result() {
	int row = get_selected_row();
	if (row == -1) {
		return nullptr;
	}

    auto cid = get_text(0, row);
    auto n = cid.find('-');
    auto tth = cid.substr(n+1);
    cid = cid.substr(0, n);

    for(unsigned int i=0; i<m_results.size(); ++i) {
        auto result = m_results[i];
        if(cid == result->getUser().user->getCID().toBase32() &&
           tth == result->getTTH().toBase32())
        {
            return result;
        }
    }
    return nullptr;
}

void WindowSearch::set_property(Property property)
{
    m_property = property;
	setInsertMode(true);
    const char *text[] = {
        "",
        "Search string?",
        "Minimum size?",
        "Maximum size?",
        "File extension?",
        "Download file to:",
        "Download directory to:"
    };
    m_prompt = text[m_property];
}

void WindowSearch::search(const std::string &str)
{
    /*
    if(m_lastSearch && m_lastSearch + 3*1000 < TimerManager::getInstance()->getTick()) {
        core::Log::get()->log("Wait a moment before a new search");
        return;
    }
    */

	if (str.length() < MIN_SEARCH  && m_searchStr.length() < MIN_SEARCH) {
        //core::Log::get()->log("Too short search");
		set_prompt_timed("Too short search", 3);
        return;
    }

	auto type = str.size() == 39 && Encoder::isBase32(str.c_str()) ? SearchManager::TYPE_TTH : SearchManager::TYPE_ANY;

    // new search
	auto newSearch = AdcSearch::getSearch(str, Util::emptyString, 0, type, SearchManager::SIZE_DONTCARE, StringList(), AdcSearch::MATCH_FULL_PATH, false);
	if (!newSearch) {
		return;
	}
	
	m_searchStr = str;
	m_results.clear();
	curSearch.reset(newSearch);
	filtering = false;

	SettingsManager::getInstance()->addToHistory(str, SettingsManager::HISTORY_SEARCH);
    m_lastSearch = GET_TICK();
	token = Util::toString(Util::rand());

	SearchManager::getInstance()->search(str, 0, type, SearchManager::SIZE_DONTCARE, token, Search::MANUAL);
	updateTitle();
}

void WindowSearch::handle_line(const std::string &line)
{
	if (!getInsertMode())
		return;

    if(!line.empty()) {
        if(m_property == PROP_FILETARGET) {
            download(line);
        } else if(m_property == PROP_DIRECTORYTARGET) {
            download_directory(line);
		} else if (curSearch) {
			filtering = true;
			if (m_property == PROP_SEARCHFILTER) {
				if (line.length() >= MIN_SEARCH) {
					curSearch.reset(new AdcSearch(line, Util::emptyString, StringList(), AdcSearch::MATCH_NAME));
				} else {
					m_property = PROP_NONE;
					return;
				}
			} else if (m_property == PROP_MINSIZE || m_property == PROP_MAXSIZE) {
				size_t end;
				int multiplier;
				auto hasType = [&, this](string&& id) {
					end = Util::findSubString(line, id, line.size() - id.size());
					return end != string::npos;
				};

				if (hasType("g")) {
					multiplier = 1024 * 1024 * 1024;
				} else if (hasType("m")) {
					multiplier = 1024 * 1024;
				} else if (hasType("k")) {
					multiplier = 1024;
				} else {
					multiplier = 1024 * 1024;
				}

				auto size = Util::toInt64(line)*multiplier;
				if (m_property == PROP_MINSIZE) {
					curSearch->gt = size;
					//curSearch->lt = std::numeric_limits<int64_t>::max();
				} else {
					curSearch->lt = size;
					//curSearch->gt = std::numeric_limits<int64_t>::min();
				}
			} else if (m_property == PROP_EXTENSION) {
				curSearch->ext = StringTokenizer<string>(Text::toLower(line), ',').getTokens();
			}

			create_list();
		}
	} else if (m_property != PROP_FILETARGET && m_property != PROP_DIRECTORYTARGET && filtering) {
		curSearch.reset(new AdcSearch(m_searchStr, Util::emptyString, StringList(), AdcSearch::MATCH_NAME));
		filtering = false;
		create_list();
	}

	setInsertMode(false);
    set_prompt("");
    m_property = PROP_NONE;
}

void WindowSearch::complete(const std::vector<std::string>& aArgs, int /*pos*/, std::vector<std::string>& suggest_, bool& appendSpace_) {
	if (m_property == PROP_FILETARGET || m_property == PROP_DIRECTORYTARGET) {
		if (!aArgs[0].empty()) {
			input::Completion::getDiskPathSuggestions(aArgs[0], suggest_);
		} else {
			suggest_ = SettingsManager::getInstance()->getHistory(SettingsManager::HISTORY_DIR);
		}

		appendSpace_ = false;
	}
}

void WindowSearch::handleEscape() {
	m_property = PROP_NONE;
}

void WindowSearch::create_list()
{
    delete_all();

	for (const auto& sr: m_results) {
		if (matches(sr)) {
			add_result(sr);
		}
	}

	updateTitle();
}

void WindowSearch::updateTitle() {
	auto display = curSearch && curSearch->root ? m_searchStr + " (TTH)" : m_searchStr;
	if (m_results.size() > 0) {
		std::ostringstream oss;
		oss << "Search: " << display << " with " << get_size()
			<< "/" << m_results.size() << " results";
		set_title(oss.str());
	} else {
		set_title("Search window: " + display);
	}

	set_name("Search:" + display);
}

void WindowSearch::on(SearchManagerListener::SR, const SearchResultPtr& aSR)
    noexcept
{
	if (!matches(aSR)) {
		return;
	}

	callAsync([=] {
		m_results.push_back(aSR);
		add_result(aSR);
		updateTitle();
	});
}

void WindowSearch::add_result(const SearchResultPtr& result)
{
    int row = insert_row();
    set_text(0, row, result->getUser().user->getCID().toBase32() + "-" + result->getTTH().toBase32());
	set_text(1, row, ClientManager::getInstance()->getFormatedNicks(result->getUser()));
    set_text(2, row, utils::to_string(result->getFreeSlots())
        + "/" + utils::to_string(result->getSlots()));
    set_text(3, row, Util::formatBytes(result->getSize()));
	set_text(4, row, Util::getDateTime(result->getDate()));
	set_text(5, row, utils::escape(result->getFileName()));
}

bool WindowSearch::matches(const SearchResultPtr& aResult)
{
	if (!curSearch) {
		return false;
	}

	if (m_freeSlots && aResult->getFreeSlots() == 0) {
		return false;
	}

	if (!filtering && !aResult->getUser().user->isNMDC()) {
		// ADC
		return aResult->getToken() == token;
	}

	// NMDC / filtering
	if (aResult->getType() == SearchResult::TYPE_DIRECTORY) {
		if (!curSearch->matchesDirectory(aResult->getPath())) {
			return false;
		}
	} else {
		if (!(curSearch->root ? *curSearch->root == aResult->getTTH() : curSearch->matchesFileLower(Text::toLower(aResult->getPath()), aResult->getSize(), aResult->getDate()))) {
			return false;
		}
	}

	return true;
}

void WindowSearch::free_results()
{
    delete_all();
    m_results.clear();
	updateTitle();
}

void WindowSearch::download(const std::string& aPath)
{
    auto result = get_result();
	if (!result)
		return;

	auto target = Util::validatePath(aPath, true);

    try {
        if(result->getType() == SearchResult::TYPE_FILE) {
            QueueManager::getInstance()->createFileBundle(
                target + result->getFileName(),
                result->getSize(), result->getTTH(),
                result->getUser(), result->getDate(), 0, QueueItemBase::DEFAULT);
        } else {
			DirectoryListingManager::getInstance()->addDirectoryDownload(result->getPath(), result->getFileName(),
				result->getUser(), target, TargetUtil::TARGET_PATH, NO_CHECK);
        }

		if (!aPath.empty())
			SettingsManager::getInstance()->addToHistory(target, SettingsManager::HISTORY_DIR);
    } catch(const Exception &e) {
		set_prompt_timed("Error downloading the file: " + e.getError());
    }
}

void WindowSearch::download_directory(const std::string& aPath)
{
    auto result = get_result();
	if (!result)
		return;

	auto target = Util::validatePath(aPath);
    try {
        if(result->getType() == SearchResult::TYPE_FILE)
        {
			DirectoryListingManager::getInstance()->addDirectoryDownload(result->getPath(),
                Util::getNmdcLastDir(result->getFilePath()), result->getUser(),
                target.empty() ? SETTING (DOWNLOAD_DIRECTORY) : target + "/", TargetUtil::TARGET_PATH, NO_CHECK);
        }
        else
        {
			DirectoryListingManager::getInstance()->addDirectoryDownload(result->getPath(), result->getFileName(),
				result->getUser(), target, TargetUtil::TARGET_PATH, NO_CHECK);
        }
    }
    catch(Exception &e)
    {
		set_prompt_timed("Error downloading the directory: " + e.getError());
    }

	if (!aPath.empty())
		SettingsManager::getInstance()->addToHistory(target, SettingsManager::HISTORY_DIR);
}

std::string WindowSearch::get_infobox_line(unsigned int n)
{
    auto result = get_result();
	if (!result)
		return "";

    std::stringstream ss;
    switch(n) {
		case 1:
		{
			auto ip = result->getIP();
			const auto& country = GeoManager::getInstance()->getCountry(ip);
			if (!country.empty()) {
				ip = country + " (" + ip + ")";
			}

			ss << "%21User:%21 " << std::left << std::setw(18)
				<< ClientManager::getInstance()->getFormatedNicks(result->getUser())
				<< " %21IP:%21 " << ip;
			break;
		}
        case 2:
			ss << "%21Size:%21 " << std::left << std::setw(9)
				<< Util::formatBytes(result->getSize())
				<< " %21Slots:%21 " << result->getFreeSlots() << "/" << result->getSlots()
				<< " %21TTH:%21 " << (result->getType() == SearchResult::TYPE_FILE ? result->getTTH().toBase32() : "-");
            break;
        case 3:
			ss << "%21Hub:%21 " << std::left << std::setw(18) 
				<< ClientManager::getInstance()->getFormatedHubNames(result->getUser())
				<< " %21Date:%21 " << Util::getDateTime(result->getDate());
            break;
        case 4:
        {
			ss << utils::escape(Util::toAdcFile(result->getPath()));
            break;
        }
    }
    return ss.str();
}

WindowSearch::~WindowSearch()
{
    SearchManager::getInstance()->removeListener(this);
}

} // namespace ui
