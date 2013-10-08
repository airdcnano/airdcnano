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

#include "stdinc.h"
#include "ThrottleManager.h"

#include "DownloadManager.h"
#include "Singleton.h"
#include "Socket.h"
#include "Thread.h"
#include "TimerManager.h"
#include "UploadManager.h"
#include "ClientManager.h"

namespace dcpp {
/**
 * Manager for throttling traffic flow.
 * Inspired by Token Bucket algorithm: http://en.wikipedia.org/wiki/Token_bucket
 */

/*
 * Throttles traffic and reads a packet from the network
 */
int ThrottleManager::read(Socket* sock, void* buffer, size_t len)
{
	int64_t readSize = -1;
	size_t downs = DownloadManager::getInstance()->getDownloadCount();
	auto downLimit = getDownLimit(); // avoid even intra-function races
	if(!getCurThrottling() || downLimit == 0 || downs == 0)
		return sock->read(buffer, len);

	{
		Lock l(downCS);

		if(downTokens > 0)
		{
			int64_t slice = (downLimit * 1024) / downs;
			readSize = min(slice, min(static_cast<int64_t>(len), downTokens));

			// read from socket
			readSize = sock->read(buffer, static_cast<size_t>(readSize));

			if(readSize > 0)
				downTokens -= readSize;
		}
	}

	if(readSize != -1)
	{
		Thread::yield(); // give a chance to other transfers to get a token
		return readSize;
	}

	waitToken();
	return -1;	// from BufferedSocket: -1 = retry, 0 = connection close
}

/*
 * Throttles traffic and writes a packet to the network
 * Handle this a little bit differently than downloads due to OpenSSL stupidity 
 */
int ThrottleManager::write(Socket* sock, void* buffer, size_t& len)
{
	bool gotToken = false;
	size_t ups = UploadManager::getInstance()->getUploadCount();
	auto upLimit = getUpLimit(); // avoid even intra-function races
	if(!getCurThrottling() || upLimit == 0 || ups == 0)
		return sock->write(buffer, len);

	{
		Lock l(upCS);

		if(upTokens > 0)
		{
			size_t slice = (upLimit * 1024) / ups;
			len = min(slice, min(len, static_cast<size_t>(upTokens)));
			upTokens -= len;

			gotToken = true; // token successfuly assigned
		}
	}

	if(gotToken)
	{
		// write to socket			
		int sent = sock->write(buffer, len);

		Thread::yield(); // give a chance to other transfers get a token
		return sent;
	}

	waitToken();
	return 0;	// from BufferedSocket: -1 = failed, 0 = retry
}

SettingsManager::IntSetting ThrottleManager::getCurSetting(SettingsManager::IntSetting setting) {
	SettingsManager::IntSetting upLimit   = SettingsManager::MAX_UPLOAD_SPEED_MAIN;
	SettingsManager::IntSetting downLimit = SettingsManager::MAX_DOWNLOAD_SPEED_MAIN;
	//SettingsManager::IntSetting slots     = SettingsManager::SLOTS_PRIMARY;

	if(SETTING(TIME_DEPENDENT_THROTTLE)) {
		time_t currentTime;
		time(&currentTime);
		int currentHour = localtime(&currentTime)->tm_hour;
		if((SETTING(BANDWIDTH_LIMIT_START) < SETTING(BANDWIDTH_LIMIT_END) &&
			currentHour >= SETTING(BANDWIDTH_LIMIT_START) && currentHour < SETTING(BANDWIDTH_LIMIT_END)) ||
			(SETTING(BANDWIDTH_LIMIT_START) > SETTING(BANDWIDTH_LIMIT_END) &&
			(currentHour >= SETTING(BANDWIDTH_LIMIT_START) || currentHour < SETTING(BANDWIDTH_LIMIT_END))))
		{
			upLimit   = SettingsManager::MAX_UPLOAD_SPEED_ALTERNATE;
			downLimit = SettingsManager::MAX_DOWNLOAD_SPEED_ALTERNATE;
			//slots     = SettingsManager::SLOTS_ALTERNATE_LIMITING;
		}
	}

	switch (setting) {
		case SettingsManager::MAX_UPLOAD_SPEED_MAIN:
			return upLimit;
		case SettingsManager::MAX_DOWNLOAD_SPEED_MAIN:
			return downLimit;
		//case SettingsManager::SLOTS:
		//	return slots;
		default:
			return setting;
	}
}

int ThrottleManager::getUpLimit() {
	return SettingsManager::getInstance()->get(getCurSetting(SettingsManager::MAX_UPLOAD_SPEED_MAIN));
}

int ThrottleManager::getDownLimit() {
	return SettingsManager::getInstance()->get(getCurSetting(SettingsManager::MAX_DOWNLOAD_SPEED_MAIN));
}

void ThrottleManager::setSetting(SettingsManager::IntSetting setting, int value) {
	if(value < 0 || value > MAX_LIMIT)
		value = 0;
	SettingsManager::getInstance()->set(setting, value);
	ClientManager::getInstance()->infoUpdated();
}

bool ThrottleManager::getCurThrottling() {
	Lock l(stateCS);
	return activeWaiter != -1;
}

void ThrottleManager::waitToken() {
	// no tokens, wait for them, so long as throttling still active
	// avoid keeping stateCS lock on whole function
	CriticalSection *curCS = 0;
	{
		Lock l(stateCS);
		if (activeWaiter != -1)
			curCS = &waitCS[activeWaiter];
	}
	// possible post-CS aW shifts: 0->1/1->0: lock lands in wrong place, will
	// either fall through immediately or wait depending on whether in
	// stateCS-protected transition elsewhere; 0/1-> -1: falls through. Both harmless.
	if (curCS)
		Lock l(*curCS);
}

ThrottleManager::~ThrottleManager(void)
{
	shutdown();
	TimerManager::getInstance()->removeListener(this);
}

void ThrottleManager::shutdown() {
	Lock l(stateCS);
	if (activeWaiter != -1) {
		waitCS[activeWaiter].unlock();
		activeWaiter = -1;
	}
}

// TimerManagerListener
void ThrottleManager::on(TimerManagerListener::Second, uint64_t /* aTick */) noexcept
{
	//int newSlots = SettingsManager::getInstance()->get(getCurSetting(SettingsManager::SLOTS));
	//if(newSlots != SETTING(SLOTS)) {
	//	setSetting(SettingsManager::SLOTS, newSlots);
	//}

	{
		Lock l(stateCS);
		if (activeWaiter == -1)
			// This will create slight weirdness for the read/write calls between
			// here and the first activeWaiter-toggle below.
			waitCS[activeWaiter = 0].lock();
	}

	// readd tokens
	{
		Lock l(downCS);
		downTokens = getDownLimit() * 1024;
	}

	{
		Lock l(upCS);
		upTokens = getUpLimit() * 1024;
	}

	// let existing events drain out (fairness).
	// www.cse.wustl.edu/~schmidt/win32-cv-1.html documents various
	// fairer strategies, but when only broadcasting, irrelevant
	{
		Lock l(stateCS);

		dcassert(activeWaiter == 0 || activeWaiter == 1);
		waitCS[1-activeWaiter].lock();
		activeWaiter = 1-activeWaiter;
		waitCS[1-activeWaiter].unlock();
	}
}

}	// namespace dcpp
