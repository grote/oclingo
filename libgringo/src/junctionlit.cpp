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
#include "gringo/output.h"

/*
  both conjunction and disjunction are grounded the same way
   - heads must not be used for matching

   - non-heads grounded in bottom up fashion
	 - instantiator is finished iff (index of) conjunction is finished

   - for conjunctions always maintain exactly one conditional
	 - should be the case because preprocessing is for heads is special

   - calculate global vaiables wrt. conditionals only
	 - :- p(X), q(Y), r(Z) : s(X,Z).
	 - Y might be problematic here because the conjunction might be regrounded
	 - create an index for the conjunction literal
	   - store the global variables there (only Z in this case)
	   - after grounding of an conditional finished mark as finished
	   - if the containing literal is finished remove the finished mark
		 (and finish the indices of the conditional)
	   - global variables or an identifier for them have to be passed as identifier to output

	- fact heads
	 - disjunction:  ignore the whole rule
	 - conjunctions: ignore the literal

   - rule:         head :-    ..., conjunction,      ... .
	 instantiator: idx(head), ..., idx(conjunction), ...
	 conjunction:  a : b : ...
	 instantiator: idx(cond), idx(a), idx(b), ...

*/

struct JunctionIndex : public StaticIndex
{
	struct Status
	{
		// Note: conjunction are facts as long as there are no non-facts
		//       disjunctions are facts if there is some fact
		Status(bool head)
			: generation(1)
			, uid(0)
			, fact(!head)
		{
		}

		uint32_t generation;
		uint32_t uid;
		bool     fact;
	};
	typedef boost::unordered_map<ValVec, Status> Done;

	JunctionIndex(JunctionLit &lit, VarSet &global)
		: lit_(lit)
		, global_(global.begin(), global.end())
		, generation_(2)
		, uid_(0)
		, dirty_(false)
	{
	}

	bool first(Grounder *grounder, int)
	{
		ValVec global;
		foreach (uint32_t var, global_) { global.push_back(grounder->val(var)); }
		Done::iterator it = done_.find(global);
		status_ = &(it != done_.end() ? it : done_.insert(Done::value_type(global, Status(lit_.head()))).first)->second;
		if (status_->generation == 0) { return false; }
		if (status_->generation == 1) { status_->uid = uid_++; }
		if (status_->generation < generation_)
		{
			if (!lit_.ground(grounder))
			{
				status_->generation = 1;
				return false;
			}
			status_->generation = generation_;
		}
		lit_.fact(status_->fact);
		status_ = 0;
		return true;
	}

	void reset()
	{
		done_.clear();
		generation_ = 2;
		uid_        = 0;
		dirty_      = false;
	}

	void finish()
	{
		lit_.finish();
		generation_++;
		dirty_ = false;
	}

	bool fact() const
	{
		return status_->fact;
	}

	void fact(bool fact)
	{
		status_->fact = fact;
	}

	void setDirty()
	{
		dirty_ = true;
	}

	bool hasNew() const
	{
		return dirty_;
	}

private:
	Status      *status_;
	JunctionLit &lit_;
	Done         done_;
	VarVec       global_;
	uint32_t     generation_;
	uint32_t     uid_;
	bool         dirty_;
public:
	IndexPtrVec  headIndices;
};

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

void JunctionCond::normalize(Grounder *g, const Lit::Expander &headExp, const Lit::Expander &bodyExp, JunctionLit *parent, uint32_t index)
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

void JunctionCond::addIndex(Grounder *g, VarSet &bound, Lit *lit)
{
	inst_->append(lit->index(g, this, bound));
}

bool JunctionCond::complete() const
{
	if (!head_->complete()) { return false; }
	foreach (Lit const &lit, body_)
	{
		if (!lit.complete()) { return false; }
	}
	return true;
}

bool JunctionCond::grounded(Grounder *g, Index &head, JunctionIndex &index, uint32_t offset)
{
	Index::Match match = head.firstMatch(g, 0);
	head_->grounded(g);
	bool fact = true;
	foreach (Lit &lit, body_)
	{
		lit.grounded(g);
		if (!lit.fact()) { fact = false; }
	}
	if (!parent_->head())
	{
		if (head_->complete() && fact && !match.first) { return false; }
		if (!fact || !head_->fact()) { index.fact(false); }
	}
	else
	{
		if (fact && head_->fact()) { index.fact(true); }
	}
	std::cerr << "pass cond to printer..." << std::endl;
	return true;
}

void JunctionCond::init(Grounder *g, JunctionIndex &parent)
{
	if(!inst_.get())
	{
		assert(level() > 0);
		inst_.reset(new Instantiator(vars(), 0));
		VarSet bound;
		GlobalsCollector::collect(*this, bound, level() - 1);
		if(litDep_.get())
		{
			litDep_->order(g, boost::bind(&JunctionCond::addIndex, this, g, boost::ref(bound), _1), bound);
		}
		else
		{
			foreach(Lit &lit, body_) { inst_->append(lit.index(g, this, bound)); }
		}
		parent.headIndices.push_back(head_->index(g, this, bound));
		inst_->callback(boost::bind(&JunctionCond::grounded, this, _1, boost::ref(parent.headIndices.back()), boost::ref(parent), index_));
	}
	if(inst_->init(g)) { enqueue(g); }
}

bool JunctionCond::ground(Grounder *g)
{
	return inst_->ground(g);
}

void JunctionCond::visit(PrgVisitor *visitor)
{
	visitor->visit(head_.get(), false);
	foreach(Lit &lit, body_) { visitor->visit(&lit, false); }
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
	, conds_(conds)
	, parent_(0)
	, uid_(0)
	, fact_(false)
{
}

bool JunctionLit::ground(Grounder *g)
{
	foreach (JunctionCond &cond, conds_)
	{
		if (!cond.ground(g)) { return false; }
	}
	return true;
}

void JunctionLit::expandHead(const Lit::Expander &ruleExp, JunctionCond &cond, Lit *lit, Lit::ExpansionType type)
{
	switch(type)
	{
		case RANGE:
		{
			ruleExp(lit, type);
			break;
		}
		case POOL:
		{
			LitPtrVec body;
			foreach(const Lit &l, cond.body_) { body.push_back(l.clone()); }
			std::auto_ptr<JunctionCond> newCond(new JunctionCond(cond.loc(), lit, body));
			if(head())
			{
				JunctionCondVec lits;
				foreach(const JunctionCond &l, conds_)
				{
					if(&l == &cond) { lits.push_back(newCond); }
					else            { lits.push_back(new JunctionCond(l)); }
				}
				JunctionLit *lit(new JunctionLit(loc(), lits));
				ruleExp(lit, type);
			}
			else { conds_.push_back(newCond); }
			break;
		}
		case RELATION:
		{
			cond.body_.push_back(lit);
			break;
		}
	}
}

void JunctionLit::normalize(Grounder *g, const Expander &e)
{
	for(JunctionCondVec::size_type i = 0; i < conds_.size(); i++)
	{
		Expander headExp = boost::bind(&JunctionLit::expandHead, this, boost::ref(e), boost::ref(conds_[i]), _1, _2);
		Expander bodyExp = boost::bind(static_cast<void (LitPtrVec::*)(Lit *)>(&LitPtrVec::push_back), &conds_[i].body_, _1);
		conds_[i].normalize(g, headExp, bodyExp, this, i);
	}
	uid_ = g->aggrUid();
}

bool JunctionLit::complete() const
{
	foreach(const JunctionCond &cond, conds_)
	{
		if (!cond.complete()) { return false; }
	}
	return true;
}

void JunctionLit::fact(bool fact)
{
	fact_ = fact;
}

bool JunctionLit::fact() const
{
	if (head() || complete()) { return fact_; }
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

void JunctionLit::finish()
{
	foreach (JunctionCond &cond, conds_) { cond.finish(); }
}

Index *JunctionLit::index(Grounder *g, Formula *f, VarSet &)
{
	parent_ = f;
	VarSet global;
	GlobalsCollector::collect(*this, global, f->level());
	std::auto_ptr<JunctionIndex> idx(new JunctionIndex(*this, global));
	foreach (JunctionCond &cond, conds_) { cond.init(g, *idx); }
	return idx.release();
}

Lit::Score JunctionLit::score(Grounder *, VarSet &)
{
	return Score(LOWEST, 0);
}

void JunctionLit::enqueue(Grounder *g)
{
	parent_->enqueue(g);
}

void JunctionLit::visit(PrgVisitor *visitor)
{
	foreach(JunctionCond &cond, conds_) { visitor->visit(&cond, head()); }
}

void JunctionLit::accept(::Printer *v)
{
	Printer *printer = v->output()->printer<Printer>();
	std::cerr << "pass lit to printer..." << std::endl;
	// I need the respective index here ...
	// printer->begin(head());
	// dom_.print(printer);
	// TODO ...
	// printer->end();
}

Lit *JunctionLit::clone() const
{
	return new JunctionLit(*this);
}

JunctionLit::~JunctionLit()
{
}
