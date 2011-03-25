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

#include "gringo/junctionlit.h"
#include "gringo/instantiator.h"
#include "gringo/litdep.h"
#include "gringo/index.h"

// - analysis
//   - literals after : have to be stratified
//   - no restriction to head literal
// - matching
//   - if not global vars in map
//     - ground all conditionals
//     - add to (multi) map global vars -> vars(head)
//   - if conjunction
//     - loop over stored vars
//       - mark as finished if head matches
//     - if all vars marked
//       - the conjunction matches
//     - else
//       - do not match
//   - if disjunction
//     - always match
// - printing
//   - start printing
//   - loop over stored vars
//     - set substitution to vars
//     - print the head
//   - end printing

namespace
{
	class CondHeadExpander : public Expander
	{
	public:
		CondHeadExpander(JunctionLit &aggr, JunctionCond *cond, Expander &ruleExpander);
		void expand(Lit *lit, Type type);

	private:
		JunctionLit  &aggr_;
		JunctionCond *cond_;
		Expander     &ruleExp_;
	};

	class CondBodyExpander : public Expander
	{
	public:
		CondBodyExpander(JunctionCond &cond);
		void expand(Lit *lit, Type type);
	private:
		JunctionCond &cond_;
	};
}

///////////////////////////// JunctionCond /////////////////////////////

JunctionCond::JunctionCond(const Loc &loc, Lit *head, LitPtrVec &body)
	: Groundable(loc)
	, head_(head)
	, body_(body)
{
}

void JunctionCond::normalize(Grounder *g, Expander *headExp, Expander *bodyExp)
{
	head_->normalize(g, headExp);
	for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
	{
		body_[i].normalize(g, bodyExp);
	}
}

bool JunctionCond::grounded(Grounder *g)
{

}

void JunctionCond::ground(Grounder *g)
{

}

void JunctionCond::visit(PrgVisitor *visitor)
{
	visitor->visit(head_.get(), false);
	foreach(Lit &lit, body_) { visitor->visit(&lit, true); }
}

void JunctionCond::print(Storage *sto, std::ostream &out) const
{
	head_->print(sto, out);
	foreach(const Lit &lit, body_)
	{
		out << ":";
		lit.print(sto, out);
	}
}

JunctionCond::~JunctionCond()
{
}

///////////////////////////// JunctionLit /////////////////////////////

JunctionLit::JunctionLit(const Loc &loc, JunctionCondVec &conds)
	: Lit(loc)
	, fact_(false)
	, conds_(conds)
{
}

void JunctionLit::normalize(Grounder *g, Expander *expander)
{
	for(JunctionCondVec::size_type i = 0; i < conds_.size(); i++)
	{
		CondHeadExpander headExp(*this, &conds_[i], *expander);
		CondBodyExpander bodyExp(conds_[i]);
		conds_[i].normalize(g, &headExp, &bodyExp);
	}
}

bool JunctionLit::match(Grounder *grounder)
{
	fact_ = false;
	// ...
}

bool JunctionLit::fact() const
{
	return fact_;
}

void JunctionLit::print(Storage *sto, std::ostream &out) const
{
	bool comma = false;
	foreach(const JunctionCond &cond, conds_)
	{
		if(comma) { out << "|"; }
		else      { comma = true; }
		cond.print(sto, out);
	}
}

void JunctionLit::index(Grounder *g, Groundable *gr, VarSet &bound)
{
	/*
	parent_ = gr;
	VarSet global;
	GlobalsCollector::collect(*this, global, parent_->level());
	if(head())
	{
		foreach(AggrCond &lit, conds_) { lit.initHead(); }
	}
	// NOTE: this possibly reinitializes the domain
	//       but the set of global variables always stays the same
	domain_.init(global);
	gr->instantiator()->append(new AggrIndex(*this, bound));
	if(assign()) { assign()->vars(bound); }
	*/
}

Lit::Score JunctionLit::score(Grounder *, VarSet &)
{
	return Score(LOWEST, 0);
}

void JunctionLit::visit(PrgVisitor *visitor)
{
	foreach(JunctionCond &cond, conds_) { visitor->visit(&cond, head()); }
}

void JunctionLit::accept(::Printer *v)
{
}

Lit *JunctionLit::clone() const
{
	return new JunctionLit(*this);
}

JunctionLit::~JunctionLit()
{
}

///////////////////////////// CondHeadExpander /////////////////////////////

CondHeadExpander::CondHeadExpander(JunctionLit &aggr, JunctionCond *cond, Expander &ruleExpander)
	: aggr_(aggr)
	, cond_(cond)
	, ruleExp_(ruleExpander)
{
}

void CondHeadExpander::expand(Lit *lit, Expander::Type type)
{
	switch(type)
	{
		case RANGE:
			ruleExp_.expand(lit, type);
			break;
		case POOL:
		{
			LitPtrVec body;
			foreach(const Lit &l, cond_->body()) { body.push_back(l.clone()); }
			std::auto_ptr<JunctionCond> cond(new JunctionCond(cond_->loc(), lit, body));
			if(aggr_.head())
			{
				JunctionCondVec lits;
				foreach(const JunctionCond &l, aggr_.conds())
				{
					if(&l == cond_) lits.push_back(cond);
					else lits.push_back(new JunctionCond(l));
				}
				JunctionLit *lit(new JunctionLit(aggr_.loc(), lits));
				ruleExp_.expand(lit, type);
			}
			else { aggr_.conds().push_back(cond.release()); }
			break;
		}
		case RELATION:
			cond_->body().push_back(lit);
			break;
	}
}

///////////////////////////// CondBodyExpander /////////////////////////////

CondBodyExpander::CondBodyExpander(JunctionCond &cond)
	: cond_(cond)
{
}

void CondBodyExpander::expand(Lit *lit, Expander::Type)
{
	cond_.body().push_back(lit);
}
