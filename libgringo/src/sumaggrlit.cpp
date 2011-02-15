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

#include <gringo/sumaggrlit.h>
#include <gringo/term.h>
#include <gringo/predlit.h>
#include <gringo/index.h>
#include <gringo/instantiator.h>
#include <gringo/grounder.h>
#include <gringo/exceptions.h>
#include <gringo/index.h>
#include <gringo/output.h>

namespace
{
	class SumBoundAggrState : public BoundAggrState
	{
	public:
		SumBoundAggrState(Grounder *g, AggrLit &lit);
		void accumulate(Grounder *g, AggrLit &lit, int32_t weight, bool isNew, bool newFact);
		void doAccumulate(Grounder *g, AggrLit &lit, const ValVec &set, bool isNew, bool newFact);
	private:
		int64_t min_;
		int64_t max_;
	};

	class SumAssignAggrState : public AssignAggrState
	{
	public:
		SumAssignAggrState();
		void doAccumulate(Grounder *g, AggrLit &lit, const ValVec &set, bool isNew, bool newFact);
	};
}

//////////////////////////////////////// SumAggrLit ////////////////////////////////////////

SumAggrLit::SumAggrLit(const Loc &loc, CondLitVec &conds)
	: AggrLit(loc, conds)
{
}

AggrState *SumAggrLit::newAggrState(Grounder *g)
{
	if(assign()) { return new SumAssignAggrState(); }
	else         { return new SumBoundAggrState(g, *this); }
}

Lit *SumAggrLit::clone() const
{
	return new SumAggrLit(*this);
}

void SumAggrLit::print(Storage *sto, std::ostream &out) const
{
	if(sign_) out << "not ";
	if(lower_.get())
	{
		lower_->print(sto, out);
		out << (assign_ ? ":=" : " ");
	}
	bool comma = false;
	out << "#sum[";
	foreach(const CondLit &lit, conds_)
	{
		if(comma) { out << ","; }
		else      { comma = true; }
		lit.print(sto, out);
	}
	out << "]";
	if(upper_.get())
	{
		out << " ";
		upper_->print(sto, out);
	}
}

void SumAggrLit::accept(::Printer *v)
{
	Printer *printer = v->output()->printer<Printer>();
	printer->begin(domain_.last(), head(), sign_, complete());
	if(lower()) { printer->lower(valLower_.number()); }
	if(upper()) { printer->upper(valUpper_.number()); }
	printer->end();
}

//////////////////////////////////////// SumBoundAggrState ////////////////////////////////////////

SumBoundAggrState::SumBoundAggrState(Grounder *g, AggrLit &lit)
	: min_(0)
	, max_(0)
{
	accumulate(g, lit, 0, true, true);
}

void SumBoundAggrState::accumulate(Grounder *g, AggrLit &lit, int32_t weight, bool isNew, bool newFact)
{
	if(isNew)
	{
		if(newFact)
		{
			min_ += weight;
			max_ += weight;
		}
		else
		{
			if(weight < 0) { min_ += weight; }
			else           { max_ += weight; }
		}
	}
	else if(newFact)
	{
		if(weight < 0) { max_ += weight; }
		else           { min_ += weight; }
	}
	int64_t lower(lit.lower() ? (int64_t)lit.lower()->val(g).number() : std::numeric_limits<int64_t>::min());
	int64_t upper(lit.upper() ? (int64_t)lit.upper()->val(g).number() : std::numeric_limits<int64_t>::max());
	checkBounds(lit, min_, max_, lower, upper);
}

void SumBoundAggrState::doAccumulate(Grounder *g, AggrLit &lit, const ValVec &set, bool isNew, bool newFact)
{
	accumulate(g, lit, set[0].number(), isNew, newFact);
}

//////////////////////////////////////// SumAssignAggrState ////////////////////////////////////////

SumAssignAggrState::SumAssignAggrState()
{
	assign_.push_back(Val::create(Val::NUM, 0));
	fact_ = true;
}

void SumAssignAggrState::doAccumulate(Grounder *g, AggrLit &, const ValVec &set, bool isNew, bool newFact)
{
	if(isNew)
	{
		int32_t weight = set[0].number();
		if(!newFact) { fact_ = false; }
		size_t end = assign_.size();
		for(size_t i = 0; i != end; i++)
		{
			if(!fact_ || assign_[i].locked)
			{
				assign_.push_back(Val::create(Val::NUM, weight + assign_[i].val.num));
			}
			else
			{
				assign_[i].val.num  += weight;
			}
		}
		std::sort(assign_.begin(), assign_.end(), AssignAggrState::Assign::Less(g));
		assign_.erase(std::unique(assign_.begin(), assign_.end()), assign_.end());
	}
}
