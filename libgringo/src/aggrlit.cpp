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

#include <gringo/aggrlit.h>
#include <gringo/term.h>
#include <gringo/grounder.h>
#include <gringo/prgvisitor.h>
#include <gringo/instantiator.h>
#include <gringo/litdep.h>
#include <gringo/exceptions.h>
#include <gringo/predlit.h>
#include <gringo/varterm.h>
#include <gringo/constterm.h>
#include <gringo/domain.h>
#include <gringo/index.h>
#include <gringo/output.h>

namespace
{
	class AggrCondAnonymousRemover : public PrgVisitor
	{
	public:
		AggrCondAnonymousRemover(Grounder *g, AggrCond &cond);
		void visit(VarTerm *var, bool bind);
		void visit(Term* term, bool bind);
		void visit(Lit *lit, bool domain);
	public:
		static void remove(Grounder *g, AggrCond &cond);
	private:
		Grounder *grounder_;
		AggrCond  &cond_;
		uint32_t  vars_;
	};

	class LparseAggrCondConverter : public PrgVisitor
	{
	public:
		LparseAggrCondConverter(Grounder *g, AggrCond &cond);
		void visit(VarTerm *var, bool bind);
		void visit(Term* term, bool bind);
		void visit(PredLit *pred);
		void visit(Lit *lit, bool domain);
	public:
		static void convert(Grounder *g, AggrCond &cond, uint32_t number);
	private:
		Grounder *grounder_;
		AggrCond  &cond_;
		VarSet    vars_;
		bool      addVars_;
		bool      ignorePreds_;
	};

	class AggrIndex : public Index
	{
	public:
		AggrIndex(AggrLit &lit, VarSet &bound);
		Match firstMatch(Grounder *grounder, int binder);
		Match nextMatch(Grounder *grounder, int binder);
		void reset();
		void finish();
		bool hasNew() const;
		AggrState *current() const;
		~AggrIndex();
	private:
		AggrLit   &lit_;
		VarVec     assignVars_;
		bool       finished_;
	};

	class AggrStateDirtyIndex : public Index
	{
	public:
		AggrStateDirtyIndex(AggrLit &lit);
		Match firstMatch(Grounder *grounder, int binder);
		Match nextMatch(Grounder *grounder, int binder);
		bool hasNew() const;
		void reset();
		void finish();

	private:
		AggrLit &lit_;
	};

}

//////////////////////////////////////// AggrState ////////////////////////////////////////

AggrState::AggrState()
	: groundSwitch(true)
{ }

void AggrState::accumulate(Grounder *g, Lit *head, const VarVec &headVars, AggrLit &lit, const ValVec &set, bool fact)
{
	std::pair<ValVecSet::iterator, bool> res = sets_.insert(ValVecSet::value_type(set, false));
	bool newFact      = !res.first->second && fact;
	res.first->second = res.first->second || fact;
	if(res.second && head->head())
	{
		Substitution &vals = subst_[head];
		vals.first++;
		foreach(uint32_t var, headVars) { vals.second.push_back(g->val(var)); }
	}
	doAccumulate(g, lit, set, res.second, newFact);
}

AggrState::~AggrState()
{
}

//////////////////////////////////////// BoundAggrState::Val64 ////////////////////////////////////////

BoundAggrState::Val64::Val64(const int64_t &num)
	: isNum_(true)
	, num_(num)
{ }

BoundAggrState::Val64::Val64(const Val &val)
	: isNum_(true)
	, val_(val)
{ }

BoundAggrState::Val64 BoundAggrState::Val64::create(const int64_t &num)
{
	return Val64(num);
}

BoundAggrState::Val64 BoundAggrState::Val64::create(const Val &val)
{
	if(val.type == Val::NUM) { return Val64(val.num); }
	else                     { return Val64(val);     }
}


int BoundAggrState::Val64::compare(const Val64 &v, Storage *s) const
{
	if(isNum_ && v.isNum_)
	{
		if(num_ < v.num_)      { return -1; }
		else if(num_ > v.num_) { return 1; }
		else                   { return 0; }
	}
	else { return val_.compare(v.val_, s); }
}

//////////////////////////////////////// BoundAggrState ////////////////////////////////////////

void BoundAggrState::checkBounds(Grounder *g, AggrLit &lit, const Val64 &min, const Val64 &max, const Val64 &lower, const Val64 &upper)
{
	match_ = fact_ = upper.compare(lower, g) <= -(lit.lowerEq_ && lit .upperEq_);

	match_ = match_ || min.compare(lower, g) <= -lit.lowerEq_;
	match_ = match_ || upper.compare(max, g) <= -lit.upperEq_;

	fact_ = fact_ || max.compare(lower, g) <= -lit.lowerEq_;
	fact_ = fact_ || upper.compare(min, g) <= -lit.upperEq_;

	//match_ = upper < lower || min < lower || upper < max;
	//fact_  = upper < lower || max < lower || upper < min;
	if(!lit.sign())
	{
		bool match = !fact_;
		fact_      = !match_;
		match_     = match;
	}
}

//////////////////////////////////////// AggrDomain ////////////////////////////////////////

AggrDomain::AggrDomain()
	: domain_(0)
	, last_(0)
	, new_(false)
	, groundSwitch_(true)
{
}

void AggrDomain::init(const VarSet &global)
{
	if(global_.size() != global.size())
	{
		assert(global_.size() == 0);
		global_.assign(global.begin(), global.end());
		domain_ = ValVecSet(global_.size());
	}
}

BoolPair AggrDomain::state(Grounder *g, AggrLit *lit)
{
	ValVec vals;
	foreach(uint32_t var, global_) { vals.push_back(g->val(var)); }
	ValVecSet::InsertRes res = domain_.insert(vals.begin(), false);
	// NOTE: inserting a new state does not immediately provide new bindings
	if(res.get<1>()) { states_.push_back(lit->newAggrState(g)); }
	lastId_ = vals.size() ? res.get<0>() / vals.size() : 0;
	last_   = &states_[lastId_];
	if(res.get<1>())
	{
		last_->groundSwitch = !groundSwitch_;
		return BoolPair(true, true);
	}
	else
	{
		bool oldSwitch = last_->groundSwitch;
		last_->groundSwitch = !groundSwitch_;
		return BoolPair(false, oldSwitch == groundSwitch_);
	}
}

void AggrDomain::finish()
{
	groundSwitch_ = !groundSwitch_;
	foreach(AggrState &state, states_)
	{
		state.finish();
		state.groundSwitch = groundSwitch_;
	}
	new_ = false;
}

//////////////////////////////////////// AggrLit ////////////////////////////////////////

AggrLit::AggrLit(const Loc &loc, AggrCondVec &conds, bool set)
	: Lit(loc)
	, sign_(false)
	, assign_(false)
	, fact_(false)
	, set_(set)
	, lowerEq_(true)
	, upperEq_(true)
	, complete_(boost::logic::indeterminate)
	, parent_(0)
	, conds_(conds.release())
	, aggrUid_(0)
{
	foreach(AggrCond &lit, conds_) { lit.aggr_ = this; }
}

void AggrLit::lower(Term *l, bool eq)
{ 
	lower_.reset(l); 
	lowerEq_ = eq;
}

void AggrLit::upper(Term *u, bool eq)
{ 
	upper_.reset(u);
	upperEq_ = eq;
}

void AggrLit::assign(Term *a)
{
	lower_.reset(a);
	assign_ = true;
	lowerEq_ = upperEq_ = true;
}

void AggrLit::add(AggrCond *cond)
{
	conds_.push_back(cond);
}

void AggrLit::addDomain(Grounder *g, bool)
{
	assert(head());
	if(domain_.last()->match())
	{
		foreach(AggrCond &lit, conds_) { lit.addDomain(g, false); }
	}
}

void AggrLit::enqueue(Grounder *g)
{
	domain().markNew();
	parent_->enqueue(g);
}

bool AggrLit::complete() const
{
	if(boost::logic::indeterminate(complete_))
	{
		complete_ = true;
		foreach(const AggrCond &lit, conds_)
		{
			if(!lit.complete())
			{
				complete_ = false;
				break;
			}
		}
	}
	return complete_;
}

bool AggrLit::fact() const
{
	return !head() && complete() && fact_;
}

void AggrLit::endGround(Grounder *g)
{
	foreach(AggrCond &cond, conds_) { cond.endGround(g); }
}

Index *AggrLit::index(Grounder *g, Formula *gr, VarSet &bound)
{
	parent_ = gr;
	VarSet global;
	GlobalsCollector::collect(*this, global, parent_->level());
	if(head())
	{
		foreach(AggrCond &cond, conds_) { cond.initHead(); }
	}
	// NOTE: this possibly reinitializes the domain
	//       but the set of global variables always stays the same
	domain_.init(global);
	AggrIndex *idx = new AggrIndex(*this, bound);
	if(assign()) { assign()->vars(bound); }
	foreach (AggrCond &cond, conds_) { cond.initInst(g); }
	return idx;
}

void AggrLit::visit(PrgVisitor *visitor)
{
	foreach(AggrCond &lit, conds_) { visitor->visit(&lit, head()); }
	if(lower_.get()) { visitor->visit(lower_.get(), assign_); }
	if(upper_.get()) { visitor->visit(upper_.get(), false); }
}

void AggrLit::normalize(Grounder *g, const Expander &e)
{
	assert(!(assign_ && sign_));
	assert(!(assign_ && head()));
	if(lower_.get()) { lower_->normalize(this, Term::PtrRef(lower_), g, e, assign_); }
	if(upper_.get()) { upper_->normalize(this, Term::PtrRef(upper_), g, e, false); }
	for(AggrCondVec::size_type i = 0; i < conds_.size(); i++)
	{
		conds_[i].head(head());
		conds_[i].aggr_ = this;
		conds_[i].normalize(g, i);
	}
	aggrUid_ = g->aggrUid();
}

void AggrLit::ground(Grounder *g, bool isNew)
{
	if(isNew)
	{
		foreach(AggrCond &lit, conds_) { lit.ground(g); }
	}
	fact_ = domain_.last()->fact();
}

void AggrLit::grounded(Grounder *g)
{
	if(lower()) { valLower_ = lower()->val(g); }
	if(upper()) { valUpper_ = upper()->val(g); }
	if(!head()) { domain_.last()->lock(); }
}

AggrLit::~AggrLit()
{
}

//////////////////////////////////////// AggrSetLit ////////////////////////////////////////

AggrSetLit::AggrSetLit(const Loc &loc, TermPtrVec &terms)
	: Lit(loc)
	, terms_(terms.release())
{
}

Index *AggrSetLit::index(Grounder *, Formula *gr, VarSet &)
{
	AggrCond *cond = static_cast<AggrCond*>(gr);
	return new AggrStateDirtyIndex(*cond->aggr());
}

void AggrSetLit::normalize(Grounder *g, const Expander &e)
{
	for(TermPtrVec::iterator i = terms_.begin(); i != terms_.end(); i++)
	{
		i->normalize(this, Term::VecRef(terms_, i), g, e, false);
	}
}

void AggrSetLit::visit(PrgVisitor *visitor)
{
	foreach(Term &term, terms_) { term.visit(visitor, false); }
}

void AggrSetLit::print(Storage *sto, std::ostream &out) const
{
	bool comma = false;
	foreach(const Term &term, terms_)
	{
		if(comma) { out << ","; }
		else      { comma = true; }
		term.print(sto, out);
	}
}

Lit *AggrSetLit::clone() const
{
	return new AggrSetLit(*this);
}

AggrSetLit::~AggrSetLit()
{
}

//////////////////////////////////////// AggrCond ////////////////////////////////////////

AggrCond::AggrCond(const Loc &loc, TermPtrVec &terms, LitPtrVec &lits, Style style)
	: Formula(loc)
	, style_(style)
	, set_(loc, terms)
	, lits_(lits.release())
	, aggr_(0)
	, complete_(boost::logic::indeterminate)
	, head_(false)
{
}

void AggrCond::head(bool head)
{
	head_ = head;
	// TODO: ugly - a special Lit::method is probably the only way arround? (Lit::positive?)
	PredLit *lit = dynamic_cast<PredLit*>(&lits_[0]);
	if(lit && !lit->sign()) { lit->head(head); }
}

void AggrCond::enqueue(Grounder *g)
{
	aggr_->enqueue(g);
}

bool AggrCond::complete() const
{
	if(boost::logic::indeterminate(complete_))
	{
		bool head = head_;
		complete_ = true;
		foreach(const Lit &lit, lits_)
		{
			if(!lit.complete() && !head)
			{
				complete_ = false;
				break;
			}
			head = false;
		}
	}
	return complete_;
}

void AggrCond::expandHead(Lit *lit, Lit::ExpansionType type)
{
	switch(type)
	{
		case Lit::RANGE:
		case Lit::RELATION:
		{
			lits_.push_back(lit);
			break;
		}
		case Lit::POOL:
		{
			LitPtrVec lits;
			lits.push_back(lit);
			lits.insert(lits.end(), lits_.begin() + 1, lits_.end());
			TermPtrVec terms(*set_.terms());
			aggr()->add(new AggrCond(loc(), terms, lits, style_));
			break;
		}
	}
}

void AggrCond::expandSet(Lit *lit, Lit::ExpansionType type)
{
	switch(type)
	{
		case Lit::RANGE:
		case Lit::RELATION:
		{
			lits_.push_back(lit);
			break;
		}
		case Lit::POOL:
		{
			std::auto_ptr<AggrSetLit> set(static_cast<AggrSetLit*>(lit));
			LitPtrVec lits(lits_);
			aggr()->add(new AggrCond(loc(), *set->terms(), lits, style_));
			break;
		}
	}
}

void AggrCond::normalize(Grounder *g, uint32_t number)
{
	assert(!lits_.empty());
	bool head = lits_[0].head() || style_ != STYLE_DLV;
	if(head) { lits_[0].normalize(g, boost::bind(&AggrCond::expandHead, this, _1, _2)); }
	set_.normalize(g, boost::bind(&AggrCond::expandSet, this, _1, _2));
	Lit::Expander bodyExp = boost::bind(static_cast<void (LitPtrVec::*)(Lit *)>(&LitPtrVec::push_back), &lits_, _1);
	for(LitPtrVec::size_type i = head; i < lits_.size(); i++)
	{
		lits_[i].normalize(g, bodyExp);
	}
	if(style_ != STYLE_DLV)
	{
		AggrCondAnonymousRemover::remove(g, *this);
		LparseAggrCondConverter::convert(g, *this, number);
	}
}

void AggrCond::initInst(Grounder *g)
{
	if(!inst_.get())
	{
		inst_.reset(new Instantiator(vars(), boost::bind(&AggrCond::grounded, this, _1)));
		simpleInitInst(g, *inst_);
	}
	if(inst_->init(g)) { enqueue(g); }
}

void AggrCond::finish()
{
	inst_->finish();
}

void AggrCond::ground(Grounder *g)
{
	inst_->ground(g);
}

void AggrCond::visit(PrgVisitor *visitor)
{
	visitor->visit(&set_, false);
	foreach(Lit &lit, lits_) { visitor->visit(&lit, false); }
}

void AggrCond::print(Storage *sto, std::ostream &out) const
{
	out << "<";
	set_.print(sto, out);
	out << ">";
	std::vector<const Lit*> lits;
	foreach(const Lit &lit, lits_) { lits.push_back(&lit); }
	std::sort(lits.begin() + head_, lits.end(), Lit::cmpPos);
	foreach(const Lit *lit, lits)
	{
		out << ":";
		lit->print(sto, out);
	}

}

void AggrCond::addDomain(Grounder *g, bool fact)
{
	Lit &head = lits_[0];
	if(!head.sign())
	{
		// TODO: what about storing only local vars in head vars?
		g->pushContext();
		AggrState::Substitution &vals = aggr_->domain().last()->substitution(&head);
		ValVec::iterator it = vals.second.begin();
		for(uint32_t i = 0; i < vals.first; i++)
		{
			foreach(uint32_t &var, headVars_) { g->val(var, *it++, 0); }
			head.grounded(g);
			head.addDomain(g, fact);
		}
		foreach(uint32_t &var, headVars_) { g->unbind(var); }
		vals.second.clear();
		vals.first = 0;
		g->popContext();
	}
}

bool AggrCond::grounded(Grounder *g)
{
	bool fact = true;
	foreach(Lit &lit, lits_)
	{
		lit.grounded(g);
		if(!lit.fact()) { fact = false; }
	}
	ValVec set;
	foreach(Term &term, *set_.terms()) { set.push_back(term.val(g)); }
	try { aggr_->domain().last()->accumulate(g, &lits_[0], headVars_, *aggr_, set, fact); }
	catch(const Val *val)
	{
		std::ostringstream oss;
		oss << "cannot convert ";
		val->print(g, oss);
		oss << " to integer";
		std::string str(oss.str());
		oss.str("");
		print(g, oss);
		throw TypeException(str, StrLoc(g, loc()), oss.str());
	}
	Printer *printer = g->output()->printer<Printer>();
	printer->begin(style_, Printer::State(aggr_->aggrUid(), aggr_->domain().lastId()), set);
	bool head = head_;
	foreach(Lit &lit, lits_)
	{
		if(!lit.fact()) { lit.accept(printer); }
		else if(head)   { printer->trueLit(); }
		if(head) {
			printer->endHead();
			head = false;
		}
	}
	printer->end();
	return true;
}

void AggrCond::endGround(Grounder *g)
{
	if(!lits_[0].sign()) { lits_[0].endGround(g); }
}

void AggrCond::initHead()
{
	if(!lits_[0].sign() && lits_[0].head()) { GlobalsCollector::collect(lits_[0], headVars_, level()); }
}

AggrCond::~AggrCond()
{
}

AggrCond* new_clone(const AggrCond& a)
{
	return new AggrCond(a);
}

//////////////////////////////////////// AggrCondAnonymousRemover ////////////////////////////////////////

AggrCondAnonymousRemover::AggrCondAnonymousRemover(Grounder *g, AggrCond &cond)
	: grounder_(g)
	, cond_(cond)
	, vars_(0)
{
}

void AggrCondAnonymousRemover::visit(VarTerm *var, bool)
{
	if(var->anonymous())
	{
		std::ostringstream oss;
		oss << "#anonymous(" << vars_++ << ")";
		var->nameId(grounder_->index(oss.str()));
	}
}

void AggrCondAnonymousRemover::visit(Term* term, bool bind)
{
	term->visit(this, bind);
}

void AggrCondAnonymousRemover::visit(Lit *lit, bool)
{
	lit->visit(this);
}

void AggrCondAnonymousRemover::remove(Grounder *g, AggrCond &cond)
{
	AggrCondAnonymousRemover ar(g, cond);
	cond.visit(&ar);
}

//////////////////////////////////////// LparseAggrCondConverter ////////////////////////////////////////

LparseAggrCondConverter::LparseAggrCondConverter(Grounder *g, AggrCond &cond)
	: grounder_(g)
	, cond_(cond)
	, addVars_(false)
	, ignorePreds_(false)
{ }

void LparseAggrCondConverter::visit(VarTerm *var, bool)
{
	vars_.insert(var->nameId());
}

void LparseAggrCondConverter::visit(Term* term, bool bind)
{
	if(addVars_) { term->visit(this, bind); }
	else         { cond_.terms()->push_back(term->clone()); }
}

void LparseAggrCondConverter::visit(PredLit *pred)
{
	if(!ignorePreds_ && cond_.style() == AggrCond::STYLE_LPARSE && cond_.aggr()->set())
	{
		addVars_ = false;
		std::stringstream ss;
		ss << "#pred(" << grounder_->string(pred->dom()->nameId()) << "," << pred->dom()->arity() << ")";
		uint32_t name  = grounder_->index(ss.str());
		cond_.terms()->push_back(new ConstTerm(cond_.loc(), Val::id(name)));
	}
}

void LparseAggrCondConverter::visit(Lit *lit, bool)
{
	addVars_ = true;
	lit->visit(this);
}

void LparseAggrCondConverter::convert(Grounder *g, AggrCond &cond, uint32_t number)
{
	LparseAggrCondConverter conv(g, cond);
	if(cond.style() == AggrCond::STYLE_LPARSE && cond.aggr()->set())
	{
		LitPtrVec::iterator it = cond.lits()->begin();
		conv.visit(&*it, false);
		if(conv.addVars_)
		{
			conv.ignorePreds_ = true;
			for(it++; it != cond.lits()->end(); it++) { conv.visit(&*it, false); }
		}
	}
	else if(cond.style() == AggrCond::STYLE_LPARSE)
	{
		foreach(Lit &lit, *cond.lits()) { conv.visit(&lit, false); }
	}
	if(conv.addVars_)
	{
		std::stringstream ss;
		ss << "#cond(" << number << ")";
		uint32_t name  = g->index(ss.str());
		cond.terms()->push_back(new ConstTerm(cond.loc(), Val::id(name)));
		foreach(uint32_t var, conv.vars_) { cond.terms()->push_back(new VarTerm(cond.loc(), var)); }
	}
}

//////////////////////////////////////// AggrIndex ////////////////////////////////////////

AggrIndex::AggrIndex(AggrLit &lit, VarSet &bound)
	: lit_(lit)
	, finished_(false)
{
	if(lit.assign())
	{
		VarSet vars;
		lit.assign()->vars(vars);
		std::set_difference(vars.begin(), vars.end(), bound.begin(), bound.end(), std::back_inserter(assignVars_));
	}
}

AggrState *AggrIndex::current() const { return lit_.domain().last(); }

Index::Match AggrIndex::firstMatch(Grounder *g, int binder)
{
	BoolPair isNew = lit_.domain().state(g, &lit_);
	lit_.isNewAggrState(isNew.first);
	lit_.ground(g, isNew.second);
	if(lit_.assign())
	{
		assert(!lit_.head());
		Val v;
		for(bool match = current()->bindFirst(&v); match; match = current()->bindNext(&v))
		{
			foreach(uint32_t var, assignVars_) { g->unbind(var); }
			if(lit_.assign()->unify(g, v, binder)) { return Match(true, !finished_ || current()->isNew()); }
		}
		return Match(false, false);
	}
	else if(current()->bindFirst(0) || lit_.head()) { return Match(true, !finished_ || current()->isNew()); }
	else { return Match(false, false); }
}

Index::Match AggrIndex::nextMatch(Grounder *g, int binder)
{
	if(lit_.assign())
	{
		Val v;
		while(current()->bindNext(&v))
		{
			foreach(uint32_t var, assignVars_) { g->unbind(var); }
			if(lit_.assign()->unify(g, v, binder)) { return Match(true, !finished_ || current()->isNew()); }
		}
		return Match(false, false);
	}
	else { return Match(false, false); }
}

void AggrIndex::reset()
{
	finished_ = false;
}

void AggrIndex::finish()
{
	// TODO: continue here!
	foreach(AggrCond &cond, lit_.conds()) { cond.finish(); }
	lit_.domain().finish();
	finished_ = true;
}

bool AggrIndex::hasNew() const
{
	return !finished_ || lit_.domain().hasNew();
}

AggrIndex::~AggrIndex()
{
}

//////////////////////////////////////// AggrStateDirtyIndex ////////////////////////////////////////

AggrStateDirtyIndex::AggrStateDirtyIndex(AggrLit &lit)
	: lit_(lit)
{
}

Index::Match AggrStateDirtyIndex::firstMatch(Grounder *, int)
{
	return Match(true, lit_.isNewAggrState());
}

bool AggrStateDirtyIndex::hasNew() const
{
	return lit_.isNewAggrState();
}

AggrStateDirtyIndex::Match AggrStateDirtyIndex::nextMatch(Grounder *, int)
{
	return Match(false, false);
}

void AggrStateDirtyIndex::reset()
{
}

void AggrStateDirtyIndex::finish()
{
}

