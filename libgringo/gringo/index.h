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

class StaticIndex : public Index
{
public:
	virtual bool first(Grounder *grounder, int binder) = 0;
	virtual bool next(Grounder *grounder, int binder);
	virtual Match firstMatch(Grounder *grounder, int binder);
	virtual Match nextMatch(Grounder *grounder, int binder);
	virtual void reset();
	virtual void finish();
	virtual bool hasNew() const;
	virtual bool init(Grounder* g);
	virtual ~StaticIndex();
};

class Matchable
{
public:
	virtual bool match(Grounder *grounder) = 0;
	virtual ~Matchable();
};

class MatchIndex : public StaticIndex
{
public:
	MatchIndex(Matchable *m);
	virtual bool first(Grounder *grounder, int binder);
	virtual bool next(Grounder *grounder, int binder);
	virtual ~MatchIndex();

private:
	Matchable *m_;
};
