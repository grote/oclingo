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
#include "gringo/grounder.h"

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

	class JunctionIndex : public Index
	{
	public:
		JunctionIndex(JunctionLit &lit);
		Match firstMatch(Grounder *grounder, int binder);
		Match nextMatch(Grounder *grounder, int binder);
		void reset();
		void finish();
		bool hasNew() const;
	private:
		JunctionLit &lit_;
		bool         finished_;
	};

	class JunctionStateNewIndex : public NewOnceIndex
	{
	public:
		JunctionStateNewIndex(JunctionLitDomain &dom);
		Match firstMatch(Grounder *g, int binder);
		bool hasNew() const;
	private:
		JunctionLitDomain &dom_;
	};

	void addIndex(Instantiator *inst, Grounder *g, Formula *f, VarSet &bound, Lit *head, Lit *lit)
	{
		if(lit != head) { inst->append(lit->index(g, f, bound)); }
	}

}

///////////////////////////// JunctionState /////////////////////////////
JunctionState::JunctionState()
	: match(false)
	, fact(false)
	, isNew(true)
{
}

///////////////////////////// JunctionLitDomain /////////////////////////////

JunctionLitDomain::JunctionLitDomain()
	: f_(0)
	, new_(false)
	, newState_(false)
{
}

bool JunctionLitDomain::fact() const
{
	return current_->fact;
}

void JunctionLitDomain::finish()
{
	foreach(StateMap::reference ref, state_)
	{
		if(ref.second.match) { ref.second.isNew = false; }
	}
	new_ = false;
}

void JunctionLitDomain::initGlobal(Formula *f, const VarVec &global)
{
	f_      = f;
	global_ = global;
}

void JunctionLitDomain::initLocal(Grounder *g, Formula *f, uint32_t index, Lit &head)
{
	if(local_.size() <= index)
	{

		heads_.push_back(&head);
		local_.push_back(VarVec());
		VarSet bound;
		GlobalsCollector::collect(head, bound, f->level());
		std::set_difference(bound.begin(), bound.end(), global_.begin(), global_.end(), std::back_inserter(local_.back()));
		indices_.push_back(head.index(g, f, bound));
	}

}

void JunctionLitDomain::enqueue(Grounder *g)
{
	new_ = true;
	f_->enqueue(g);
}

void JunctionLitDomain::state(Grounder *g)
{
	ValVec global;
	foreach(uint32_t var, global_) { global.push_back(g->val(var)); }
	current_ = &state_[global];
}

void JunctionLitDomain::accumulate(Grounder *g, uint32_t index)
{
	current_->vals.push_back(Val::create(Val::ID, index));
	foreach(uint32_t var, local_[index]) { current_->vals.push_back(g->val(var)); }
}

BoolPair JunctionLitDomain::match(Grounder *g)
{
	foreach(Index &idx, indices_) { idx.init(g); }
	if(!current_->match)
	{
		current_->match = true;
		current_->fact  = true;
		for(ValVec::iterator it = current_->vals.begin(); it != current_->vals.end(); it++)
		{
			uint32_t index = it->index;
			foreach(uint32_t var, local_[index]) { g->val(var, *++it, 0); }
			if(!indices_[index].firstMatch(g, 0).first)
			{
				current_->match = false;
				current_->fact  = false;
				break;
			}
			if(current_->fact)
			{
				heads_[index]->grounded(g);
				current_->fact = heads_[index]->fact();
			}
		}
		foreach(VarVec &vars, local_)
		{
			foreach(uint32_t var, vars) { g->unbind(var); }
		}
	}
	return BoolPair(current_->match, current_->match && current_->isNew);
}

///////////////////////////// JunctionCond /////////////////////////////

JunctionCond::JunctionCond(const Loc &loc, Lit *head, LitPtrVec &body)
	: Formula(loc)
	, head_(head)
	, body_(body)
	, index_(0)
	, parent_(0)
{
	head_->head(true);
}

void JunctionCond::finish()
{
	inst_->finish();
}

void JunctionCond::init(Grounder *g, JunctionLitDomain &dom)
{
	dom.initLocal(g, this, index_, *head_);
}

void JunctionCond::normalize(Grounder *g, Expander *headExp, Expander *bodyExp, JunctionLit *parent, uint32_t index)
{
	parent_ = parent;
	index_  = index;
	head_->normalize(g, headExp);
	for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
	{
		body_[i].normalize(g, bodyExp);
	}
}

void JunctionCond::enqueue(Grounder *g)
{
	parent_->enqueue(g);
}

void JunctionCond::initInst(Grounder *g)
{
	if(!inst_.get())
	{
		assert(level() > 0);
		inst_.reset(new Instantiator(vars(), boost::bind(&JunctionLit::groundedCond, parent_, _1, index_)));
		inst_->append(new JunctionStateNewIndex(parent_->domain()));
		VarSet bound;
		GlobalsCollector::collect(*this, bound, level() - 1);
		litDep_->order(g, boost::bind(addIndex, inst_.get(), g, this, boost::ref(bound), parent_->head() ? (Lit*)0 : head_.get(), _1), bound);
	}
	if(inst_->init(g)) { enqueue(g); }
}

void JunctionCond::ground(Grounder *g)
{
	inst_->ground(g);
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
	, match_(false)
	, fact_(false)
	, conds_(conds)
{
}

BoolPair JunctionLit::ground(Grounder *g)
{
	dom_.state(g);
	foreach(JunctionCond &cond, conds_) { cond.ground(g); }
	return dom_.match(g);
}

void JunctionLit::normalize(Grounder *g, Expander *expander)
{
	for(JunctionCondVec::size_type i = 0; i < conds_.size(); i++)
	{
		CondHeadExpander headExp(*this, &conds_[i], *expander);
		CondBodyExpander bodyExp(conds_[i]);
		conds_[i].normalize(g, &headExp, &bodyExp, this, i);
	}
}

bool JunctionLit::fact() const
{
	return dom_.fact();
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

Index *JunctionLit::index(Grounder *g, Formula *f, VarSet &)
{
	// Note: no variables are bound
	VarVec global;
	GlobalsCollector::collect(*this, global, f->level());
	dom_.initGlobal(f, global);
	foreach(JunctionCond &cond, conds_) { cond.init(g, dom_); }
	return new JunctionIndex(*this);
}

Lit::Score JunctionLit::score(Grounder *, VarSet &)
{
	return Score(LOWEST, 0);
}

void JunctionLit::enqueue(Grounder *g)
{
	dom_.enqueue(g);
}

void JunctionLit::visit(PrgVisitor *visitor)
{
	foreach(JunctionCond &cond, conds_)
	{
		visitor->visit(&cond, head());
	}
}

void JunctionLit::accept(::Printer *v)
{
	#pragma message "implement me!"
}

Lit *JunctionLit::clone() const
{
	return new JunctionLit(*this);
}

bool JunctionLit::groundedCond(Grounder *grounder, uint32_t index)
{
	dom_.accumulate(grounder, index);
	return true;
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

///////////////////////////// CondBodyExpander /////////////////////////////

JunctionIndex::JunctionIndex(JunctionLit &lit)
	: lit_(lit)
	, finished_(false)
{
}

Index::Match JunctionIndex::firstMatch(Grounder *grounder, int binder)
{
	return lit_.ground(grounder);
}

Index::Match JunctionIndex::nextMatch(Grounder *grounder, int binder)
{
	return Match(false, false);
}

void JunctionIndex::reset()
{
	finished_ = false;
}

void JunctionIndex::finish()
{
	foreach(JunctionCond &cond, lit_.conds()) { cond.finish(); }
	lit_.domain().finish();
	finished_ = true;
}

bool JunctionIndex::hasNew() const
{
	return !finished_ || lit_.domain().hasNew();
}

///////////////////////////// JunctionStateNewIndex /////////////////////////////

JunctionStateNewIndex::JunctionStateNewIndex(JunctionLitDomain &dom)
	: dom_(dom)
{
}

Index::Match JunctionStateNewIndex::firstMatch(Grounder *, int)
{
	return Match(true, dom_.newState());
}

bool JunctionStateNewIndex::hasNew() const
{
	return NewOnceIndex::hasNew() || dom_.newState();
}
