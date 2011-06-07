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

#include <gringo/minmaxaggrlit.h>
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
	class MinMaxBoundAggrState : public BoundAggrState
	{
	public:
		MinMaxBoundAggrState(Grounder *g, MinMaxAggrLit &lit);
		void doAccumulate(Grounder *g, AggrLit &lit, const ValVec &set, bool isNew, bool newFact);
		void accumulate(Grounder *g, AggrLit &lit, const Val &val, bool isNew, bool newFact);
	private:
		Val min_;
		Val max_;
	};

	class MinMaxAssignAggrState : public AssignAggrState
	{
	public:
		MinMaxAssignAggrState(MinMaxAggrLit::Type type);
		void doAccumulate(Grounder *g, AggrLit &lit, const ValVec &set, bool isNew, bool newFact);
	private:
		MinMaxAggrLit::Type type_;
		Val                 fixed_;
	};
}

//////////////////////////////////////// MinMaxAggrLit ////////////////////////////////////////

MinMaxAggrLit::MinMaxAggrLit(const Loc &loc, AggrCondVec &conds, Type type, bool set)
	: AggrLit(loc, conds, set)
	, type_(type)
{
}

AggrState *MinMaxAggrLit::newAggrState(Grounder *g)
{
	if(assign()) { return new MinMaxAssignAggrState(type_); }
	else         { return new MinMaxBoundAggrState(g, *this); }
}

Lit *MinMaxAggrLit::clone() const
{
	return new MinMaxAggrLit(*this);
}

void MinMaxAggrLit::print(Storage *sto, std::ostream &out) const
{
	if(sign_) out << "not ";
	if(lower_.get())
	{
		lower_->print(sto, out);
		out << (assign_ ? ":=" : " ");
	}
	bool comma = false;
	out << "#" << (type_ == MINIMUM ? "min" : "max") << "[";
	foreach(const AggrCond &lit, conds_)
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

template<uint32_t T>
void MinMaxAggrLit::_accept(::Printer *v)
{
	Printer<MinMaxAggrLit, T> *printer = v->output()->printer<Printer<MinMaxAggrLit, T> >();
	printer->begin(AggrCond::Printer::State(aggrUid(), domain_.lastId()), head(), sign_, complete(), set_);
	if(lower()) { printer->lower(valLower_, lowerEq_); }
	if(upper()) { printer->upper(valUpper_, upperEq_); }
	printer->end();
}

void MinMaxAggrLit::accept(::Printer *v)
{
	if(type_ == MINIMUM) { _accept<MINIMUM>(v); }
	else                 { _accept<MAXIMUM>(v); }
}

Lit::Monotonicity MinMaxAggrLit::monotonicity() const
{
	if(!head())
	{
		if(lower()  && !upper()) { return ( sign() ^ (type_ == MINIMUM)) ? ANTIMONOTONE : MONOTONE; }
		if(!lower() &&  upper()) { return (!sign() ^ (type_ == MINIMUM)) ? ANTIMONOTONE : MONOTONE; }
	}
	return NONMONOTONE;
}

bool MinMaxAggrLit::fact() const
{
	return
		(
			fact_ &&
			!head() &&
			(
				(lower() && !upper() && (!sign() ^ (type_ == MINIMUM))) ||
				(upper() && !lower() && ( sign() ^ (type_ == MINIMUM)))
			)
		) ||
		AggrLit::fact();
}

//////////////////////////////////////// MinMaxBoundAggrState ////////////////////////////////////////

MinMaxBoundAggrState::MinMaxBoundAggrState(Grounder *g, MinMaxAggrLit &lit)
	: min_(lit.type() == MinMaxAggrLit::MAXIMUM ? Val::inf() : Val::sup())
	, max_(lit.type() == MinMaxAggrLit::MAXIMUM ? Val::inf() : Val::sup())
{
	accumulate(g, lit, lit.type() == MinMaxAggrLit::MAXIMUM ? Val::inf() : Val::sup(), true, true);
}

void MinMaxBoundAggrState::accumulate(Grounder *g, AggrLit &lit, const Val &val, bool isNew, bool newFact)
{
	if(static_cast<MinMaxAggrLit&>(lit).type() == MinMaxAggrLit::MAXIMUM)
	{
		if(newFact && min_.compare(val, g) < 0) { min_ = val; }
		if(isNew   && max_.compare(val, g) < 0) { max_ = val; }
	}
	else
	{
		if(newFact && max_.compare(val, g) > 0) { max_ = val; }
		if(isNew   && min_.compare(val, g) > 0) { min_ = val; }
	}
	Val lower(lit.lower() ? lit.lower()->val(g) : Val::inf());
	Val upper(lit.upper() ? lit.upper()->val(g) : Val::sup());
	checkBounds(g, lit, Val64::create(min_), Val64::create(max_), Val64::create(lower), Val64::create(upper));
}

void MinMaxBoundAggrState::doAccumulate(Grounder *g, AggrLit &lit, const ValVec &set, bool isNew, bool newFact)
{
	accumulate(g, lit, set[0], isNew, newFact);
}

//////////////////////////////////////// MinMaxAssignAggrState ////////////////////////////////////////

MinMaxAssignAggrState::MinMaxAssignAggrState(MinMaxAggrLit::Type type)
	: type_(type)
	, fixed_(type == MinMaxAggrLit::MAXIMUM ? Val::inf() : Val::sup())
{
	fact_  = true;
	assign_.push_back(fixed_);

}

void MinMaxAssignAggrState::doAccumulate(Grounder *g, AggrLit &, const ValVec &set, bool isNew, bool newFact)
{
	if(type_ == MinMaxAggrLit::MAXIMUM ? set[0].compare(fixed_, g) > 0 : set[0].compare(fixed_, g) < 0)
	{
		bool found;
		if(newFact)
		{
			found                 = false;
			fixed_                = set[0];
			AssignVec::iterator j = assign_.begin();
			for(AssignVec::iterator i = j; i != assign_.end(); i++)
			{
				int cmp = i->val.compare(fixed_, g);
				if(type_ == MinMaxAggrLit::MAXIMUM ? cmp >= 0 : cmp <= 0)
				{
					if(cmp == 0) { found = true; }
					*j++ = *i;
				}
			}
			assign_.erase(j, assign_.end());
		}
		else if(isNew)
		{
			found = std::find(assign_.begin(), assign_.end(), Assign(set[0])) != assign_.end();
		}
		else { found = true; }
		if(!found) { assign_.push_back(set[0]); }
		fact_ = assign_.size() == 1;
	}
}
