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

#include "sumaggregate.h"
#include "conditionalliteral.h"
#include "term.h"
#include "value.h"
#include "output.h"

using namespace NS_GRINGO;

SumAggregate::SumAggregate(ConditionalLiteralVector *literals) : AggregateLiteral(literals)
{
}

void SumAggregate::match(Grounder *g, int &lower, int &upper, int &fixed)
{
	fact_ = true;
	lower = 0;
	upper = 0;
	fixed = 0;
	maxUpperBound_ = 0;
	minLowerBound_ = 0;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		ConditionalLiteral *p = *it;
		p->ground(g, GROUND);
		for(p->start(); p->hasNext(); p->next())
		{
			int weight = p->getWeight();
			if(!p->match())
			{
				p->remove();
				continue;
			}
			if(p->isFact())
				fixed+= weight;
			else
			{
				fact_ = false;
				if(weight > 0)
					upper+= weight;
				else
					lower+= weight;
			}
			if(weight > 0)
				maxUpperBound_+= weight;
			else
				minLowerBound_+= weight;
		}
	}
	lower+= fixed;
	upper+= fixed;
}

void SumAggregate::print(const GlobalStorage *g, std::ostream &out) const
{
	if(lower_)
		out << pp(g, lower_) << " ";
	out << "sum [";
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

SumAggregate::SumAggregate(const SumAggregate &a) : AggregateLiteral(a)
{
}

NS_OUTPUT::Object *SumAggregate::convert()
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
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::SUM, lowerBound_, lits, weights, upperBound_);
	else if(hasLower)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::SUM, lowerBound_, lits, weights);
	else if(hasUpper)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::SUM, lits, weights, upperBound_);
	else
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::SUM, lits, weights);

	return a;
}

Literal *SumAggregate::clone() const
{
	return new SumAggregate(*this);
}

SumAggregate::~SumAggregate()
{
}

