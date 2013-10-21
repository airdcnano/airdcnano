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

#include <core/log.h>
#include <core/events.h>
#include <core/argparser.h>
#include <ui/window_hub.h>
#include <display/manager.h>

#include <client/FavoriteManager.h>
#include <client/HashManager.h>
#include <client/QueueManager.h>
#include <client/ShareManager.h>

#include <input/help_handler.h>
#include <input/manager.h>

#include <boost/range/algorithm/copy.hpp>

namespace modules {

	class Share {
	public:
		HelpHandler::CommandList commands = {
			{ "cancelhash", [] { HashManager::getInstance()->stop(); } },
			{ "optimizedb", [] { HashManager::getInstance()->startMaintenance(false); } },
			{ "refresh", std::bind(&Share::handleRefresh, this), COMPLETION(Share::handleSuggestRefresh) },
			{ "share", std::bind(&Share::handleShare, this), COMPLETION(Share::handleSuggestShare) },
			{ "shareprofile", std::bind(&Share::handleProfile, this), COMPLETION(Share::handleSuggestShareProfile) },
			{ "verifydb", [] { HashManager::getInstance()->startMaintenance(true); } }
		};

		HelpHandler::CommandList shareCommands = {
			{ "add", std::bind(&Share::handleRefresh, this), COMPLETION(Share::handleShareAddSuggest) },
			{ "remove", std::bind(&Share::handleShare, this), COMPLETION(Share::handleShareRemoveSuggest) },
			{ "list", std::bind(&Share::handleProfile, this), COMPLETION(Share::getProfileSuggestionsDefault) }
		};

		HelpHandler::CommandList shareProfileCommands = {
			{ "add", std::bind(&Share::handleRefresh, this), nullptr },
			{ "remove", std::bind(&Share::handleShare, this), COMPLETION(Share::getProfileSuggestionsRemove) },
			{ "list", std::bind(&Share::handleProfile, this), nullptr},
			{ "rename", std::bind(&Share::handleProfile, this), COMPLETION(Share::getProfileSuggestionsDefault) },
		};

		HelpHandler help;
		Share() : help(&commands, "Share") {
			events::add_listener("command allow", std::bind(&Share::handleAllow, this));
			events::add_listener("key pressed", std::bind(&Share::keyPressed, this));
		}

		void keyPressed() {
			auto key = events::arg<wint_t>(1);
			if (key == INPUT_CTRL('E')) {
				ShareManager::getInstance()->refresh(false, ShareManager::TYPE_MANUAL);
			}
		}

		void handleAllow() {
			if (events::args() == 0) {
				log("Usage: /allow <bundle>");
				return;
			}

			QueueManager::getInstance()->shareBundle(events::arg<string>(0));
		}

		void handleProfile() {
			core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
			parser.parse();
			if (parser.args() < 1) {
				log("Usage: /shareprofile <add|remove|rename|list>");
				return;
			}

			auto param = parser.arg(0);

			if (param == "add") {
				if (parser.args() < 2) {
					log("Usage: /shareprofile add <profile>");
					return;
				}

				auto profileName = parser.arg(1);
				auto profileToken = ShareManager::getInstance()->getProfileByName(profileName);
				if (profileToken) {
					log("Profile exists");
					return;
				}

				ShareProfileInfo::List profiles{ new ShareProfileInfo(profileName, *profileToken) };
				ShareManager::getInstance()->addProfiles(profiles);
				log("The profile " + profileName + " has been added");
				SettingsManager::getInstance()->save();
			} else if (param == "remove") {
				if (parser.args() < 2) {
					log("Usage: /shareprofile remove <profile>");
					return;
				}

				auto profileName = parser.arg(1);
				auto profileToken = ShareManager::getInstance()->getProfileByName(profileName);
				if (!profileToken) {
					log("Profile not found");
					return;
				}

				if (*profileToken == SP_HIDDEN || *profileToken == SETTING(DEFAULT_SP)) {
					log("This profile can't be removed");
					return;
				}

				ShareProfileInfo::List profiles { new ShareProfileInfo(Util::emptyString, *profileToken) };
				ShareManager::getInstance()->removeProfiles(profiles);

				auto reset = FavoriteManager::getInstance()->resetProfiles(profiles, SETTING(DEFAULT_SP));
				ClientManager::getInstance()->resetProfiles(profiles, SETTING(DEFAULT_SP));

				log("The profile " + profileName + " has been removed (" + Util::toString(reset) + " favorite hubs have been reset to use the default profile)");
				SettingsManager::getInstance()->save();
			} else if (param == "list") {
				auto& profiles = ShareManager::getInstance()->getProfiles();
				for (const auto& p : profiles) {
					log(p->getPlainName());
				}
			} else if (param == "rename") {
				if (parser.args() < 3) {
					log("Usage: /shareprofile rename <profile> <name>");
					return;
				}

				auto profileName = parser.arg(1);
				auto profileToken = ShareManager::getInstance()->getProfileByName(profileName);
				if (!profileToken) {
					log("Profile not found");
					return;
				}

				if (*profileToken == SP_HIDDEN) {
					log("This profile can't be renamed");
					return;
				}

				auto newName = parser.arg(1);
				if (ShareManager::getInstance()->getProfileByName(profileName)) {
					log("The profile " + newName + " exists already!");
					return;
				}

				ShareProfileInfo::List profiles{ new ShareProfileInfo(newName, *profileToken) };
				ShareManager::getInstance()->renameProfiles(profiles);

				log("The profile " + profileName + " has been renamed to " + newName);
				SettingsManager::getInstance()->save();
			} else {
				log("no param");
			}
		}

		void handleRefresh() {
			if (events::args() > 0) {
				auto param = events::arg<std::string>(0);
				if (Util::stricmp(param.c_str(), "incoming") == 0) {
					ShareManager::getInstance()->refresh(true, ShareManager::TYPE_MANUAL);
				} else if (ShareManager::REFRESH_PATH_NOT_FOUND == ShareManager::getInstance()->refresh(Text::fromT(param))) {
					log("Directory not found");
				}
			} else {
				ShareManager::getInstance()->refresh(false, ShareManager::TYPE_MANUAL);
			}
		}

		void log(const string& aLine) {
			display::Manager::get()->cmdMessage(aLine);
		}

		bool existsInProfile(const string aPath, const optional<ProfileToken> aToken) {
			ShareDirInfo::Map shares;
			ShareManager::getInstance()->getShares(shares);
			auto& list = shares[*aToken];
			auto p = find_if(list, ShareDirInfo::PathCompare(aPath));
			return p != list.end();
		}

		void handleShare() {
			core::ArgParser parser(events::args() > 0 ? events::arg<std::string>(0) : "");
			parser.parse();
			if (parser.args() < 1) {
				log("Usage: /share <add|remove|list>");
				return;
			}
			
			auto param = parser.arg(0);
			if (param == "add") {
				if (parser.args() < 3) {
					log("Usage: /share add <path> <virtual name> [<profile>] [<incoming>] (example: /share add Incoming /home/ftp/ myshareprofile 1)");
					return;
				}

				auto dir = parser.arg(1);
				auto vname = parser.arg(2);

				string profileName;
				if (parser.args() >= 4)
					profileName = parser.arg(3);

				bool incoming = false;
				if (parser.args() >= 5) {
					auto val = parser.arg(4);
					incoming = Util::stricmp(val, "true") == 0 || val == "1" || Util::stricmp(val, "enabled") == 0 || Util::stricmp(val, "yes") == 1;
				}

				if (dir.empty() || vname.empty()) {
					log("Invalid input");
					return;
				}

				if (dir.back() != PATH_SEPARATOR)
					dir += PATH_SEPARATOR;

				if (!Util::fileExists(dir)) {
					log("Directory doesn't exist on disk!");
					return;
				}

				auto profileToken = ShareManager::getInstance()->getProfileByName(profileName);
				if (!profileToken) {
					log("Profile not found");
					return;
				}

				if (*profileToken == SP_HIDDEN) {
					log("You can't add directories in the hidden profile!");
					return;
				}

				ShareDirInfo::List dirs = { new ShareDirInfo(vname, *profileToken, dir, incoming) };

				// in case we fell back to the default...
				auto realProfileName = ShareManager::getInstance()->getProfile(*profileToken)->getPlainName();

				// are we modifying an existing dir?
				if (existsInProfile(dir, profileToken)) {
					ShareManager::getInstance()->changeDirectories(dirs);
					log("The directory " + dir + " in profile " + realProfileName + " has been edited");
				} else {
					ShareManager::getInstance()->addDirectories(dirs);
					log("The directory " + dir + " has been added in profile " + realProfileName);
				}
				SettingsManager::getInstance()->save();
			} else if (param == "remove") {
				if (parser.args() < 2) {
					log("Usage: /share remove <path> [<profile>]");
					return;
				}

				auto dir = parser.arg(1);
				string profileName;
				if (parser.args() >= 3)
					profileName = parser.arg(2);

				if (dir.empty()) {
					log("Invalid input");
					return;
				}

				if (dir.back() != PATH_SEPARATOR)
					dir += PATH_SEPARATOR;

				auto profileToken = ShareManager::getInstance()->getProfileByName(profileName);
				if (!profileToken) {
					log("Profile not found");
					return;
				}

				// in case we fell back to the default...
				auto realProfileName = ShareManager::getInstance()->getProfile(*profileToken)->getPlainName();

				if (!existsInProfile(dir, profileToken)) {
					log("The directory " + dir + " isn't shared in profile " + realProfileName);
					return;
				}

				ShareDirInfo::List dirs = { new ShareDirInfo(Util::emptyString, *profileToken, dir, false) };
				ShareManager::getInstance()->removeDirectories(dirs);
				log("The directory " + dir + " has been removed from profile " + realProfileName);
				SettingsManager::getInstance()->save();
			} else if (param == "list") {
				if (parser.args() < 2) {
					log("Usage: /share list [<profile>]");
					return;
				}

				auto profileName = parser.arg(1);
				auto profileToken = ShareManager::getInstance()->getProfileByName(profileName);
				if (!profileToken) {
					log("Profile not found");
					return;
				}

				ShareDirInfo::Map shares;
				ShareManager::getInstance()->getShares(shares);

				auto& dirs = shares[*profileToken];
				for (const auto& sdi : dirs) {
					auto fmt = sdi->vname + ": " + sdi->path + " (" + Util::formatBytes(sdi->size) + ", incoming: " + (sdi->incoming ? "yes" : "no") + ")";
					log(fmt);
				}
			} else {
				log("no param");
			}
		}


		/* SUGGESTIONS */

		void handleSuggestList(const HelpHandler::CommandList& list, const StringList& aArgs, int pos, StringList& suggest_) {
			auto action = aArgs[1];
			auto s = boost::find_if(list, HelpHandler::CommandCompare(action));
			if (s != list.end()) {
				if ((*s).completionF)
					(*s).completionF(aArgs, pos, suggest_);
			} else {
				for (const auto& c : list) {
					suggest_.push_back(c.command);
				}
			}
		}

		void handleSuggestShare(const StringList& aArgs, int pos, StringList& suggest_) {
			handleSuggestList(shareCommands, aArgs, pos, suggest_);
		}

		void handleSuggestShareProfile(const StringList& aArgs, int pos, StringList& suggest_) {
			handleSuggestList(shareProfileCommands, aArgs, pos, suggest_);
		}

		void getProfileSuggestions(StringList& suggest_, bool listDefault = true) {
			auto& profiles = ShareManager::getInstance()->getProfiles();
			for (const auto& p : profiles) {
				if (p->getToken() != SP_HIDDEN && (listDefault || p->getToken() != SETTING(DEFAULT_SP)))
					suggest_.push_back(p->getPlainName());
			}
		}

		void getProfileSuggestionsDefault(const StringList& /*aArgs*/, int pos, StringList& suggest_) {
			if (pos == 2) {
				getProfileSuggestions(suggest_, true);
			}
		}

		void getProfileSuggestionsRemove(const StringList& /*aArgs*/, int pos, StringList& suggest_) {
			if (pos == 2) {
				getProfileSuggestions(suggest_, false);
			}
		}

		void handleShareAddSuggest(const StringList& aArgs, int pos, StringList& suggest_) {
			if (pos == 2) {
				// path
				input::Completion::getDiskPathSuggestions(aArgs[2], suggest_);
			} else if (pos == 3) {
				// vname
				auto l = ShareManager::getInstance()->getGroupedDirectories();
				boost::copy(l | map_keys, back_inserter(suggest_));
			} else if (pos == 4) {
				// profile
				getProfileSuggestions(suggest_);
			} else if (pos == 5) {
				// incoming
				suggest_.push_back("0");
				suggest_.push_back("1");
			}
		}

		void handleShareRemoveSuggest(const StringList& aArgs, int pos, StringList& suggest_) {
			if (pos == 2) {
				// path
				ShareManager::getInstance()->getParentPaths(suggest_);
			} else if (pos == 3) {
				// profile
				auto& profiles = ShareManager::getInstance()->getProfiles();
				for (const auto& p : profiles) {
					if (existsInProfile(aArgs[2], p->getToken())) {
						suggest_.push_back(p->getPlainName());
					}
				}
			}
		}

		void handleSuggestRefresh(const StringList& aArgs, int pos, StringList& suggest_) {
			if (pos == 1) {
				auto list = ShareManager::getInstance()->getGroupedDirectories();
				for (const auto& p : list) {
					suggest_.push_back(p.first);
					boost::copy(p.second, back_inserter(suggest_));
				}
			}
		}
	};

} // namespace modules

static modules::Share initialize;

