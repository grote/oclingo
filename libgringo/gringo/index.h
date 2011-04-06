// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <gringo/gringo.h>

class Index
{
public:
	typedef BoolPair Match;
public:
	virtual Match firstMatch(Grounder *grounder, int binder) = 0;
	virtual Match nextMatch(Grounder *grounder, int binder) = 0;
	virtual void reset() = 0;
	virtual void finish() = 0;
	virtual bool hasNew() const = 0;
	virtual bool init(Grounder* g);
	virtual ~Index();
};

inline Index *new_clone(const Index &) { return 0; }

class NewOnceIndex : public Index
{
public:
	NewOnceIndex();
	virtual bool first(Grounder *grounder, int binder);
	virtual bool next(Grounder *grounder, int binder);
	virtual Match firstMatch(Grounder *grounder, int binder);
	virtual Match nextMatch(Grounder *grounder, int binder);
	virtual void reset();
	virtual void finish();
	virtual bool hasNew() const;
	virtual ~NewOnceIndex();
private:
	bool finished_;
};

class Matchable
{
public:
	virtual bool match(Grounder *grounder) = 0;
	virtual ~Matchable();
};

class MatchIndex : public NewOnceIndex
{
public:
	MatchIndex(Matchable *m);
	virtual bool first(Grounder *grounder, int binder);
private:
	Matchable *m_;
};

// ========================= Index =========================
inline bool Index::init(Grounder*)
{
	return hasNew();
}

inline Index::~Index() { }

// ========================= NewOnceIndex =========================
inline NewOnceIndex::NewOnceIndex()
	: finished_(false)
{
}

inline bool NewOnceIndex::first(Grounder *, int) { return true; }
inline bool NewOnceIndex::next(Grounder *, int) { return false; }

inline Index::Match NewOnceIndex::firstMatch(Grounder *grounder, int binder)
{
	bool match = first(grounder, binder);
	return Match(match, !finished_ && match);
}

inline Index::Match NewOnceIndex::nextMatch(Grounder *grounder, int binder)
{
	bool match = next(grounder, binder);
	return Match(match, !finished_ && match);
}

inline void NewOnceIndex::reset()        { finished_ = false; }
inline void NewOnceIndex::finish()       { finished_ = true; }
inline bool NewOnceIndex::hasNew() const { return !finished_; }

inline NewOnceIndex::~NewOnceIndex() { }

// ========================= Matchable =========================

inline Matchable::~Matchable() { }

// ========================= MatchIndex =========================
inline MatchIndex::MatchIndex(Matchable *m) : m_(m)
{
}
