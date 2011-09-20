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

#include <gringo/instantiator.h>
#include <gringo/formula.h>
#include <gringo/index.h>
#include <gringo/grounder.h>

struct NewOnceIndex : public Index
{
public:
	NewOnceIndex()
		: finished(false)
	{
	}

	Match firstMatch(Grounder *, int)
	{
		return Match(true, !finished);
	}
	Match nextMatch(Grounder *, int)
	{
		return Match(false, false);
	}
	void reset()
	{
		finished = false;
	}

	void finish()
	{
		finished = true;
	}

	bool hasNew() const
	{
		return !finished;
	}

	bool finished;
};


Instantiator::Instantiator(const VarVec &vars, const GroundedCallback &grounded)
	: vars_(vars)
	, grounded_(grounded)
	, new_(1, false)
{
	append(new NewOnceIndex());
}

void Instantiator::callback(const GroundedCallback &grounded)
{
	grounded_ = grounded;
}

void Instantiator::append(Index *i)
{
	if(i)
	{
		indices_.push_back(i);
		new_.push_back(false);
	}
}

bool Instantiator::ground(Grounder *g)
{
	bool ret    = true;
	int lastNew = -1;
	int numNew  = 0;
	
	for(int i = 0; i < (int)indices_.size(); ++i)
	{
		if(indices_[i].hasNew()) { lastNew = i; }
	}
	
	int l            = -1;
	BoolPair matched = std::make_pair(true, false);
	for(;;)
	{
		if(!matched.first || (!numNew && l == lastNew))
		{
			l-= !matched.first;
			if(l == -1) { break; }
			matched = indices_[l].nextMatch(g, l);
			numNew -= new_[l];
			new_[l] = matched.second;
			numNew += matched.first && matched.second;
		}
		else
		{
			if(++l == static_cast<int>(indices_.size()))
			{
				if(!grounded_(g))
				{
					ret = false;
					break;
				}
				matched.first = false;
			}
			else
			{
				matched = indices_[l].firstMatch(g, l);
				new_[l] = matched.second;
				numNew += matched.first && matched.second;
			}
		}
	}
	foreach(uint32_t var, vars_) { g->unbind(var); }
	return ret;
}

void Instantiator::finish()
{
	foreach(Index &idx, indices_) { idx.finish(); }
}

void Instantiator::reset()
{
	#pragma message "remove this function"
	foreach(Index &index, indices_) { index.reset(); }
}

bool Instantiator::init(Grounder *g)
{
	bool modified = false;
	foreach(Index &idx, indices_)
	{
		modified = idx.init(g) || modified;
	}
	return modified;
}

Instantiator::~Instantiator()
{
}
