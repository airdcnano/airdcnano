/*
* Copyright (C) 2011-2015 AirDC++ Project
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
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef MESSAGE_MANAGER_H_
#define MESSAGE_MANAGER_H_

#include "forward.h"

#include "CriticalSection.h"
#include "Exception.h"

#include "Pointer.h"
#include "Singleton.h"
#include "UserConnection.h"
#include "ConnectionManager.h"
#include "ClientManager.h"
#include "MessageManagerListener.h"
#include "PrivateChat.h"
#include "SettingsManager.h"
#include "SimpleXML.h"
#include "StringMatch.h"

namespace dcpp {

	class ChatFilterItem {
	public:
		ChatFilterItem(const string& aNickMatch, const string& aTextMatch, StringMatch::Method aNickMethod,
			StringMatch::Method aTextMethod, bool aMainchat, bool aPM, bool aEnabled = true) : matchPM(aPM), matchMainchat(aMainchat), enabled(aEnabled)
		{
			nickMatcher.setMethod(aNickMethod);
			nickMatcher.pattern = aNickMatch;
			nickMatcher.prepare();

			textMatcher.setMethod(aTextMethod);
			textMatcher.pattern = aTextMatch;
			textMatcher.prepare();
		}
		~ChatFilterItem() {}

		enum Context {
			PM, // Private chat
			MC, // Main chat
			ALL // Both
		};

		const string& getNickPattern() const { return nickMatcher.pattern; }
		const string&  getTextPattern() const { return textMatcher.pattern; }
		StringMatch::Method getNickMethod() const { return nickMatcher.getMethod(); }
		StringMatch::Method getTextMethod() const { return textMatcher.getMethod(); }

		bool match(const string& aNick, const string& aText, Context aContext) {
			if (!getEnabled())
				return false;

			if ((aContext == PM && !matchPM) || (aContext == MC && !matchMainchat))
				return false;

			if (!nickMatcher.pattern.empty() && nickMatcher.match(aNick)){
				//nick matched, match the text in case we just want to ignore some messages of the user
				return (textMatcher.pattern.empty() || textMatcher.match(aText));
			}
			//General text match ignore type, no nick pattern just match the text
			if (nickMatcher.pattern.empty() && !textMatcher.pattern.empty() && textMatcher.match(aText))
				return true;

			return false;
		}
		void updateItem(const string& aNickMatch, const string& aTextMatch, StringMatch::Method aNickMethod, StringMatch::Method aTextMethod) {
			nickMatcher.setMethod(aNickMethod);
			nickMatcher.pattern = aNickMatch;
			nickMatcher.prepare();

			textMatcher.setMethod(aTextMethod);
			textMatcher.pattern = aTextMatch;
			textMatcher.prepare();
		}

		GETSET(bool, enabled, Enabled)

		bool matchPM;
		bool matchMainchat;

	private:
		StringMatch nickMatcher;
		StringMatch textMatcher;
	};



	class MessageManager : public Speaker<MessageManagerListener>,
		public Singleton<MessageManager>, private SettingsManagerListener,
		private UserConnectionListener, private ConnectionManagerListener {

	public:

		MessageManager() noexcept;
		~MessageManager() noexcept;

		PrivateChat* addChat(const HintedUser& user);
		PrivateChat* getChat(const UserPtr& aUser);

		void DisconnectCCPM(const UserPtr& aUser);
		bool sendPrivateMessage(const HintedUser& aUser, const tstring& msg, string& _error, bool thirdPerson);
		void onPrivateMessage(const ChatMessage& message, UserConnection* aUc = nullptr);
		void removeChat(const UserPtr& aUser);
		void closeAll(bool Offline);

		//IGNORE
		// store & remove ignores through/from hubframe
		void storeIgnore(const UserPtr& aUser);
		void removeIgnore(const UserPtr& aUser);
		bool isIgnored(const UserPtr& aUser);
		bool isIgnoredOrFiltered(const ChatMessage& msg, Client* client, bool PM);

		// chat filter
		bool isChatFiltered(const string& aNick, const string& aText, ChatFilterItem::Context aContext = ChatFilterItem::ALL);
		vector<ChatFilterItem>& getIgnoreList() { return ChatFilterItems; }
		void replaceList(vector<ChatFilterItem>& newList) {
			ChatFilterItems = newList;
		}
		//IGNORE

	private:
		unordered_map<UserPtr, PrivateChat*, User::Hash> chats;
		SharedMutex cs;

		unordered_map<UserPtr, UserConnection*, User::Hash> ccpms;
		UserConnection* getPMConn(const UserPtr& user); //LOCK usage!!

		//IGNORE
		SharedMutex Ignorecs;
		typedef unordered_set<UserPtr, User::Hash> IgnoredUsersList;
		IgnoredUsersList ignoredUsers;
		// save & load
		void load(SimpleXML& aXml);
		void save(SimpleXML& aXml);
		void saveUsers();
		void loadUsers();
		bool dirty;
		// contains the ignored nicks and patterns 
		vector<ChatFilterItem> ChatFilterItems;
		//IGNORE

		// SettingsManagerListener
		virtual void on(SettingsManagerListener::Load, SimpleXML& xml) noexcept;
		virtual void on(SettingsManagerListener::Save, SimpleXML& xml) noexcept;

		// ConnectionManagerListener
		void on(ConnectionManagerListener::Connected, const ConnectionQueueItem* cqi, UserConnection* uc) noexcept;
		void on(ConnectionManagerListener::Removed, const ConnectionQueueItem* cqi) noexcept;

		// UserConnectionListener
		virtual void on(UserConnectionListener::PrivateMessage, UserConnection* uc, const ChatMessage& message) noexcept;
		virtual void on(AdcCommand::PMI, UserConnection* uc, const AdcCommand& cmd) noexcept;
	};

}
#endif