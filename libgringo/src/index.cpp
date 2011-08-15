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

#include <gringo/index.h>
#include <gringo/lit.h>

// ========================= Index =========================
bool Index::init(Grounder*)
{
	return hasNew();
}

Index::~Index() { }

// ========================= StaticIndex =========================

bool StaticIndex::next(Grounder *, int)
{
	return false;
}

Index::Match StaticIndex::firstMatch(Grounder *grounder, int binder)
{
	bool match = first(grounder, binder);
	return Match(match, false);
}

Index::Match StaticIndex::nextMatch(Grounder *grounder, int binder)
{
	bool match = next(grounder, binder);
	return Match(match, false);
}

void StaticIndex::reset()
{
}

void StaticIndex::finish()
{
}

bool StaticIndex::hasNew() const
{
	return false;
}

bool StaticIndex::init(Grounder* g)
{
	return hasNew();
}

StaticIndex::~StaticIndex()
{
}


// ========================= Matchable =========================

Matchable::~Matchable()
{
}

// ========================= MatchIndex =========================
MatchIndex::MatchIndex(Matchable *m)\
	: m_(m)
{
}

bool MatchIndex::first(Grounder *grounder, int)
{
	return m_->match(grounder);
}

bool MatchIndex::next(Grounder *, int)
{
	return false;
}

MatchIndex::~MatchIndex()
{

}
