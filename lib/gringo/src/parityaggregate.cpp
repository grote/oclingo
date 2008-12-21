// Copyright (c) 2008, Roland Kaminski
//
// This file is part of GrinGo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

#include "parityaggregate.h"
#include "conditionalliteral.h"
#include "term.h"
#include "value.h"
#include "output.h"

using namespace NS_GRINGO;

namespace
{
	struct Hash
	{
		Value::VectorHash hash;
		size_t operator()(const std::pair<int, ValueVector> &k) const
		{
			return (size_t)k.first + hash(k.second);
		}
	};
	struct Equal
	{
		Value::VectorEqual equal;
		size_t operator()(const std::pair<int, ValueVector> &a, const std::pair<int, ValueVector> &b) const
		{
			return a.first == b.first && equal(a.second, b.second);
		}
	};
	typedef __gnu_cxx::hash_set<std::pair<int, ValueVector>, Hash, Equal> UidValueSet;
}

ParityAggregate::ParityAggregate(bool even, ConditionalLiteralVector *literals) : AggregateLiteral(literals), even_(even)
{
}

void ParityAggregate::match(Grounder *g, int &lower, int &upper, int &fixed)
{
	UidValueSet set;
	fact_ = true;
	fixed = even_;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		ConditionalLiteral *p = *it;
		p->ground(g, GROUND);
		for(p->start(); p->hasNext(); p->next())
		{
			// caution there is no -0
			if(!set.insert(std::make_pair(p->getNeg() ? -1 - p->getUid() : p->getUid(), p->getValues())).second)
			{
				p->remove();
				continue;
			}
			if(!p->match())
			{
				p->remove();
				continue;
			}
			if(p->isFact())
				fixed^= 1;
			else
			{
				fact_ = false;
				upper++;
			}
			maxUpperBound_++;
		}
	}
	if(!fact_ || fixed)
	{
		// make shure that the aggregate still matches
		if(getNeg())
			lower = 1, upper = 0;
		else
			lower = upper = 1;
	}
	else
	{
		lower = 1;
		upper = 0;
	}
}

void ParityAggregate::print(const GlobalStorage *g, std::ostream &out) const
{
	out << (even_ ? "even [" : "odd {");
	bool comma = false;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		if(comma)
			out << ", ";
		else
			comma = true;
		out << pp(g, *it);
	}
	out << "}";
}

ParityAggregate::ParityAggregate(const ParityAggregate &a) : AggregateLiteral(a), even_(a.even_)
{
}

NS_OUTPUT::Object *ParityAggregate::convert()
{
	NS_OUTPUT::ObjectVector lits;
	IntVector weights;
	for(ConditionalLiteralVector::iterator it = getLiterals()->begin(); it != getLiterals()->end(); it++)
	{
		ConditionalLiteral *p = *it;
		for(p->start(); p->hasNext(); p->next())
			lits.push_back(p->convert());
	}
	int bound = even_ ? 0 : 1;
	return new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::PARITY, bound, lits, weights, bound);
}

Literal *ParityAggregate::clone() const
{
	return new ParityAggregate(*this);
}

ParityAggregate::~ParityAggregate()
{
}

