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

#include <stdint.h>
#include <cassert>
#include <boost/functional/hash/hash.hpp>
#include <vector>

class Storage;

struct Val;

typedef std::vector<Val> ValVec;

struct Val
{
	enum Type { INF, NUM, ID, FUNC, STRING, FAIL, UNDEF, SUP };

	static inline Val inf();
	static inline Val number(int32_t num);
	static inline Val id(uint32_t id);
	static inline Val func(uint32_t id);
	static inline Val string(uint32_t id);
	static inline Val fail();
	static inline Val undef();
	static inline Val sup();

	inline size_t hash() const;
	inline bool operator==(const Val &v) const;
	bool operator!=(const Val &v) const { return !operator ==(v); }
	int compare(const Val &v, Storage *s) const;
	int compare(const int64_t &v, Storage *s) const;
	static int compare(Storage *s, ValVec const &a, ValVec const &b);
	void print(Storage *sto, std::ostream &out) const;
	int32_t  number() const;
	uint32_t type;
	Val invert(Storage *s) const;
	union
	{
		int32_t  num;
		uint32_t index;
	};

private:
	static inline Val create();
	static inline Val create(uint32_t t, int32_t num);
	static inline Val create(uint32_t t, uint32_t index);
};

Val Val::create()
{
	Val v;
	v.type = FAIL;
	v.num  = 0;
	return v;
}

Val Val::create(uint32_t t, int32_t n)
{
	Val v;
	v.type = t;
	v.num  = n;
	return v;
}

Val Val::create(uint32_t t, uint32_t i)
{
	Val v;
	v.type  = t;
	v.index = i;
	return v;
}

Val Val::inf()
{
	Val v;
	v.type = INF;
	v.num  = 0;
	return v;
}

Val Val::fail()
{
	return create(FAIL, 0);
}

Val Val::number(int32_t num)
{
	return create(NUM, num);
}

Val Val::id(uint32_t id)
{
	return create(ID, id);
}

Val Val::func(uint32_t id)
{
	return create(FUNC, id);
}

Val Val::string(uint32_t id)
{
	return create(STRING, id);
}

Val Val::undef()
{
	return create(UNDEF, 0);
}

Val Val::sup()
{
	return create(SUP, 0);
}


size_t Val::hash() const
{
	size_t hash = static_cast<size_t>(type);
	switch(type)
	{
		case INF:
		case SUP:
		case UNDEF: { return hash; }
		case NUM:   { boost::hash_combine(hash, num); return hash; }
		default:    { boost::hash_combine(hash, index); return hash; }
	}
}

bool Val::operator==(const Val &v) const
{
	if(type != v.type) { return false; }
	switch(type)
	{
		case INF:
		case SUP:
		case UNDEF: { return true; }
		case NUM:   { return num == v.num; }
		default:    { return index == v.index; }
	}
}

inline int Val::number() const
{
	if(type == NUM) { return num; }
	else            { throw(this); }
}

inline size_t hash_value(const Val &v) { return v.hash(); }
