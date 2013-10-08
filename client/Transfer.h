/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#ifndef DCPLUSPLUS_DCPP_TRANSFER_H_
#define DCPLUSPLUS_DCPP_TRANSFER_H_

#include "forward.h"

#include "CriticalSection.h"
#include "GetSet.h"
#include "MerkleTree.h"
#include "Segment.h"
#include "TimerManager.h"

namespace dcpp {

class Transfer : private boost::noncopyable {
public:
	enum Type {
		TYPE_FILE,
		TYPE_FULL_LIST,
		TYPE_PARTIAL_LIST,
		TYPE_TREE,
		TYPE_LAST
	};
	
	static const string names[TYPE_LAST];

	static const string USER_LIST_NAME;
	static const string USER_LIST_NAME_BZ;

	Transfer(UserConnection& conn, const string& path, const TTHValue& tth);
	virtual ~Transfer() { };

	int64_t getPos() const { return pos; }

	int64_t getStartPos() const { return getSegment().getStart(); }
	
	void resetPos();
	void addPos(int64_t aBytes, int64_t aActual);

	enum { MIN_SAMPLES = 15, MIN_SECS = 15 };
	
	/** Record a sample for average calculation */
	void tick();

	int64_t getActual() const { return actual; }

	int64_t getSize() const { return getSegment().getSize(); }
	void setSize(int64_t size) { segment.setSize(size); }

	bool getOverlapped() const { return getSegment().getOverlapped(); }
	void setOverlapped(bool overlap) { segment.setOverlapped(overlap); }

	double getAverageSpeed() const;

	int64_t getSecondsLeft(bool wholeFile = false) const;

	void getParams(const UserConnection& aSource, ParamMap& params) const;

	UserPtr getUser();
	const UserPtr getUser() const;
	HintedUser getHintedUser() const;
	
	//const string& getPath() const { return path; }
	const TTHValue& getTTH() const { return tth; }

	UserConnection& getUserConnection() { return userConnection; }
	const UserConnection& getUserConnection() const { return userConnection; }
	const string& getToken() const;

	GETSET(string, path, Path);
	GETSET(Segment, segment, Segment);
	GETSET(Type, type, Type);
	GETSET(uint64_t, start, Start);
private:
	
	typedef std::pair<uint64_t, int64_t> Sample;
	typedef deque<Sample> SampleList;
	
	SampleList samples;
	mutable SharedMutex cs;
	
	/** The file being transferred */
	//string path;
	/** TTH of the file being transferred */
	TTHValue tth;
	/** Bytes transferred over socket */
	int64_t actual;
	/** Bytes transferred to/from file */
	int64_t pos;

	UserConnection& userConnection;
};

} // namespace dcpp

#endif /*TRANSFER_H_*/
