/* 
 * Copyright (C) 2009-2013 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef _THROTTLEMANAGER_H
#define _THROTTLEMANAGER_H

#include "Singleton.h"
#include "Socket.h"
#include "TimerManager.h"
#include "SettingsManager.h"

namespace dcpp
{
	/**
	 * Manager for throttling traffic flow.
	 * Inspired by Token Bucket algorithm: http://en.wikipedia.org/wiki/Token_bucket
	 */
	class ThrottleManager :
		public Singleton<ThrottleManager>, private TimerManagerListener
	{
	public:

		/*
		 * Throttles traffic and reads a packet from the network
		 */
		int read(Socket* sock, void* buffer, size_t len);

		/*
		 * Throttles traffic and writes a packet to the network
		 * Handle this a little bit differently than downloads due to OpenSSL stupidity 
		 */
		int write(Socket* sock, void* buffer, size_t& len);

		void shutdown();

		static SettingsManager::IntSetting getCurSetting(SettingsManager::IntSetting setting);

		static int getUpLimit();
		static int getDownLimit();

		static void setSetting(SettingsManager::IntSetting setting, int value);

		static const int MAX_LIMIT = 1024 * 1024; // 1 GiB/s

	private:
		// stack up throttled read & write threads
		CriticalSection stateCS;
		CriticalSection waitCS[2];
		long activeWaiter;

		// download limiter
		CriticalSection	downCS;
		int64_t			downTokens;

		// upload limiter
		CriticalSection	upCS;
		int64_t			upTokens;

		friend class Singleton<ThrottleManager>;

		ThrottleManager() : activeWaiter(-1), downTokens(0), upTokens(0)
		{
			TimerManager::getInstance()->addListener(this);
		}

		virtual ~ThrottleManager();

		bool getCurThrottling();
		void waitToken();

		// TimerManagerListener
		void on(TimerManagerListener::Second, uint64_t /* aTick */) noexcept;
	};

}	// namespace dcpp
#endif	// _THROTTLEMANAGER_H
