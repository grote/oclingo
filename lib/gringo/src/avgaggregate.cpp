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

#include "avgaggregate.h"
#include "conditionalliteral.h"
#include "term.h"
#include "value.h"
#include "output.h"

using namespace NS_GRINGO;

AvgAggregate::AvgAggregate(ConditionalLiteralVector *literals) : AggregateLiteral(literals)
{
}

void AvgAggregate::match(Grounder *g, int &lower, int &upper, int &fixed)
{
	undefined_ = false;
	fact_ = false;
	lower = INT_MAX;
	upper = INT_MIN;
	int n = 0;
	int nFixed = 0;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		ConditionalLiteral *p = *it;
		p->ground(g, GROUND);
		for(p->start(); p->hasNext(); p->next())
		{
			if(!p->match())
			{
				p->remove();
				continue;
			}
			int weight = p->getWeight();
			n++;
			if(p->isFact())
			{
				nFixed++;
				fixed+= weight;
			}
			else
			{
				lower = std::min(lower, weight);
				upper = std::max(upper, weight);
			}
		}
	}
	// the aggregate is possibly empty and undefinded
	if(nFixed == 0)
	{
		undefined_ = true;
	}
	if(n == 0)
	{
		// the aggregate is trivially satisfied
		lower = INT_MIN;
		upper = INT_MAX;
		fact_ = true;
	}
	else if(nFixed > 0)
	{
		// raw estimate where the solution will be (not exact)
		lower = std::min(fixed / nFixed, lower);
		// could be more accurate but test has to be implemented carefully cause upper maybe INT_MIN
		upper = std::max((fixed + nFixed - 1) / nFixed, upper);
		// TODO: if nFixed==n a more sophisticated test could be implemented
		// but the current bounds of the aggregate have to be taken into account
	}
	maxUpperBound_ = upper;
	minLowerBound_ = lower;
}

bool AvgAggregate::checkBounds(Grounder *g, int lower, int upper)
{
	// an undefined avg aggregate is always satisfied no matter how the bounds look
	bool res = AggregateLiteral::checkBounds(g, lower, upper);
	if(undefined_)
		return true;
	else
	{
		return res;
	}
}

void AvgAggregate::print(const GlobalStorage *g, std::ostream &out) const
{
	if(lower_)
		out << pp(g, lower_) << " ";
	out << "avg [";
	bool comma = false;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		if(comma)
			out << ", ";
		else
			comma = true;
		out << pp(g, *it);
	}
	out << "]";
	if(upper_)
		out << " " << pp(g, upper_);
}

AvgAggregate::AvgAggregate(const AvgAggregate &a) : AggregateLiteral(a)
{
}

NS_OUTPUT::Object *AvgAggregate::convert()
{
	NS_OUTPUT::ObjectVector lits;
	IntVector weights;
	for(ConditionalLiteralVector::iterator it = getLiterals()->begin(); it != getLiterals()->end(); it++)
	{
		ConditionalLiteral *p = *it;
		for(p->start(); p->hasNext(); p->next())
		{
			lits.push_back(p->convert());
			weights.push_back(p->getWeight());
		}
	}
	NS_OUTPUT::Aggregate *a;
	bool hasUpper = upper_ && (upperBound_ < maxUpperBound_);
	bool hasLower = lower_ && (lowerBound_ > minLowerBound_);

	if(hasLower && hasUpper)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::AVG, lowerBound_, lits, weights, upperBound_);
	else if(hasLower)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::AVG, lowerBound_, lits, weights);
	else if(hasUpper)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::AVG, lits, weights, upperBound_);
	else
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::AVG, lits, weights);

	return a;
}

Literal *AvgAggregate::clone() const
{
	return new AvgAggregate(*this);
}

AvgAggregate::~AvgAggregate()
{
}

