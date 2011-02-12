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

namespace
{
	class GlobalsCollector : public PrgVisitor
	{
	private:
		GlobalsCollector(VarSet &globals, uint32_t level);

		void visit(VarTerm *var, bool bind);
		void visit(Term* term, bool bind);
		void visit(Lit *lit, bool domain);
		void visit(Groundable *grd, bool choice);

	public:
		static void collect(AggrLit &lit, VarSet &globals, uint32_t level);
		static void collect(Lit &lit, VarVec &globals, uint32_t level);

	private:
		VarSet  &globals_;
		uint32_t level_;
	};

	class CondHeadExpander : public Expander
	{
	public:
		CondHeadExpander(CondLit &cond);
		void expand(Lit *lit, Type type);
	private:
		CondLit &cond_;
	};

	class CondSetExpander : public Expander
	{
	public:
		CondSetExpander(CondLit &cond);
		void expand(Lit *lit, Type type);
	private:
		CondLit &cond_;
	};

	class CondBodyExpander : public Expander
	{
	public:
		CondBodyExpander(CondLit &cond);
		void expand(Lit *lit, Type type);
	private:
		CondLit &cond_;
	};

	class LparseCondLitConverter : public PrgVisitor
	{
	public:
		LparseCondLitConverter(Grounder *g, CondLit &cond);
		void visit(VarTerm *var, bool bind);
		void visit(Term* term, bool bind);
		void visit(PredLit *pred);
		void visit(Lit *lit, bool domain);
	public:
		static void convert(Grounder *g, CondLit &cond, uint32_t number);
	private:
		Grounder *grounder_;
		CondLit  &cond_;
		VarSet    vars_;
		bool      addVars_;
	};

	class AggrIndex : public Index
	{
	public:
		AggrIndex(AggrLit &lit);
		Match firstMatch(Grounder *grounder, int binder);
		Match nextMatch(Grounder *grounder, int binder);
		void reset();
		void finish();
		bool hasNew() const;
		AggrState *current() const;
		~AggrIndex();
	private:
		AggrLit   &lit_;
		bool       finished_;
	};

}

//////////////////////////////////////// AggrState ////////////////////////////////////////

AggrState::AggrState()
{ }

void AggrState::accumulate(Grounder *g, Lit *head, const VarVec &headVars, AggrLit &lit, const ValVec &set, bool fact)
{
	std::pair<ValVecSet::iterator, bool> res = sets_.insert(ValVecSet::value_type(set, false));
	bool newFact      = !res.first->second && fact;
	res.first->second = res.first->second || fact;
	if(res.second)
	{
		foreach(uint32_t var, headVars) { subst_[head].push_back(g->val(var)); }
	}
	doAccumulate(g, lit, set, res.second, newFact);
}

AggrState::~AggrState()
{
}

//////////////////////////////////////// AggrDomain ////////////////////////////////////////

AggrDomain::AggrDomain()
	: domain_(0)
	, last_(0)
	, new_(false)
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

AggrState *AggrDomain::state(Grounder *g, AggrLit *lit)
{
	ValVec vals;
	foreach(uint32_t var, global_) { vals.push_back(g->val(var)); }
	std::pair<const ValVecSet::Index&, bool> res = domain_.insert(vals.begin(), false);
	// NOTE: inserting a new state does not immediately provide new bindings
	if(res.second) { states_.push_back(lit->newAggrState(g)); }
	last_ = &states_[vals.size() ? res.first / vals.size() : 0];
	return last_;
}

void AggrDomain::finish()
{
	// TODO: could be done more efficiently (vector + markings)
	foreach(AggrState &state, states_) { state.finish(); }
	new_ = false;
}

//////////////////////////////////////// AggrLit ////////////////////////////////////////

AggrLit::AggrLit(const Loc &loc, CondLitVec &conds)
	: Lit(loc)
	, sign_(false)
	, assign_(false)
	, fact_(false)
	, complete_(tribool::indeterminate_value)
	, parent_(0)
	, conds_(conds.release())
{
	foreach(CondLit &lit, conds_) { lit.aggr_ = this; }
}

void AggrLit::doHead(bool head)
{
	foreach(CondLit &lit, conds_) { lit.head(head); }
}

void AggrLit::lower(Term *l) 
{ 
	lower_.reset(l); 
}

void AggrLit::upper(Term *u)
{ 
	upper_.reset(u);
}

void AggrLit::assign(Term *a)
{
	lower_.reset(a);
	assign_ = true;
}

void AggrLit::add(CondLit *cond)
{
	conds_.push_back(cond);
}

void AggrLit::addDomain(Grounder *g, bool)
{
	assert(head());
	if(domain_.last()->match())
	{
		foreach(CondLit &lit, conds_) { lit.addDomain(g, false); }
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
		foreach(const CondLit &lit, conds_)
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
	return !head() && ((monotonicity() == MONOTONE && !assign()) || complete()) && fact_;
}

void AggrLit::finish(Grounder *g)
{
	foreach(CondLit &lit, conds_) { lit.finish(g); }
}

void AggrLit::index(Grounder *, Groundable *gr, VarSet &bound)
{
	parent_ = gr;
	VarSet global;
	GlobalsCollector::collect(*this, global, parent_->level());
	// NOTE: this possibly reinitializes the domain
	//       but the set of global variables always stays the same
	domain_.init(global);
	if(assign()) { assign()->vars(bound); }
	gr->instantiator()->append(new AggrIndex(*this));
}

void AggrLit::visit(PrgVisitor *visitor)
{
	foreach(CondLit &lit, conds_) { visitor->visit(&lit, head()); }
	if(lower_.get()) { visitor->visit(lower_.get(), assign_); }
	if(upper_.get()) { visitor->visit(upper_.get(), false); }
}

void AggrLit::normalize(Grounder *g, Expander *expander) 
{
	assert(!(assign_ && sign_));
	assert(!(assign_ && head()));
	if(lower_.get()) { lower_->normalize(this, Term::PtrRef(lower_), g, expander, assign_); }
	if(upper_.get()) { upper_->normalize(this, Term::PtrRef(upper_), g, expander, false); }
	for(CondLitVec::size_type i = 0; i < conds_.size(); i++)
	{
		conds_[i].normalize(g, i);
	}
	foreach(CondLit &lit, conds_) { lit.aggr_ = this; }
}

void AggrLit::ground(Grounder *g)
{
	foreach(CondLit &lit, conds_) { lit.ground(g); }
	fact_ = domain_.last()->fact();
}

void AggrLit::grounded(Grounder *)
{
	if(!head()) { domain_.last()->lock(); }
}

AggrLit::~AggrLit()
{
}

//////////////////////////////////////// SetLit ////////////////////////////////////////

SetLit::SetLit(const Loc &loc, TermPtrVec &terms)
	: Lit(loc)
	, terms_(terms.release())
{
}

void SetLit::normalize(Grounder *g, Expander *e)
{
	for(TermPtrVec::iterator i = terms_.begin(); i != terms_.end(); i++)
	{
		i->normalize(this, Term::VecRef(terms_, i), g, e, false);
	}
}

void SetLit::visit(PrgVisitor *visitor)
{
	foreach(Term &term, terms_) { term.visit(visitor, false); }
}

void SetLit::print(Storage *sto, std::ostream &out) const
{
	bool comma = false;
	foreach(const Term &term, terms_)
	{
		if(comma) { out << ","; }
		else      { comma = true; }
		term.print(sto, out);
	}
}

Lit *SetLit::clone() const
{
	return new SetLit(*this);
}

SetLit::~SetLit()
{
}

//////////////////////////////////////// CondLit ////////////////////////////////////////

CondLit::CondLit(const Loc &loc, TermPtrVec &terms, LitPtrVec &lits, Style style)
	: Locateable(loc)
	, style_(style)
	, set_(loc, terms)
	, lits_(lits)
	, aggr_(0)
	, complete_(tribool::indeterminate_value)
{
}

void CondLit::head(bool head)
{
	if(!lits_[0].sign()) { lits_[0].head(head); }
}

void CondLit::enqueue(Grounder *g)
{
	aggr_->enqueue(g);
}

bool CondLit::complete() const
{
	if(boost::logic::indeterminate(complete_))
	{
		complete_ = true;
		foreach(const Lit &lit, lits_)
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

void CondLit::normalize(Grounder *g, uint32_t number)
{
	assert(!lits_.empty());
	bool head = lits_[0].head() || style_ != STYLE_DLV;
	if(head)
	{
		CondHeadExpander headExp(*this);
		lits_[0].normalize(g, &headExp);
	}
	CondSetExpander setExp(*this);
	set_.normalize(g, &setExp);
	CondBodyExpander bodyExp(*this);
	for(LitPtrVec::size_type i = head; i < lits_.size(); i++)
	{
		lits_[i].normalize(g, &bodyExp);
	}
	if(style_ != STYLE_DLV) { LparseCondLitConverter::convert(g, *this, number); }
	if(!lits_[0].sign() && lits_[0].head()) { GlobalsCollector::collect(lits_[0], headVars_, level()); }
}

void CondLit::ground(Grounder *g)
{
	inst_->ground(g);
}

void CondLit::visit(PrgVisitor *visitor)
{
	visitor->visit(&set_, false);
	foreach(Lit &lit, lits_) { visitor->visit(&lit, false); }
}

void CondLit::print(Storage *sto, std::ostream &out) const
{
	out << "<";
	set_.print(sto, out);
	std::vector<const Lit*> lits;
	foreach(const Lit &lit, lits_) { lits.push_back(&lit); }
	std::sort(lits.begin(), lits.end(), Lit::cmpPos);
	bool comma = false;
	foreach(const Lit *lit, lits)
	{
		out << (comma ? "," : ":");
		lit->print(sto, out);
		comma = true;
	}
	out << ">";
}

void CondLit::addDomain(Grounder *g, bool fact)
{
	Lit &head = lits_[0];
	if(!head.sign())
	{
		ValVec &vals = aggr_->domain().last()->substitution(&head);
		if(vals.empty())
		{
			head.grounded(g);
			head.addDomain(g, fact);
		}
		for(ValVec::iterator it = vals.begin(); it != vals.end();)
		{
			foreach(uint32_t &var, headVars_) { g->val(var, *it++, 0); }
			head.grounded(g);
			head.addDomain(g, fact);
		}
		foreach(uint32_t &var, headVars_) { g->unbind(var); }
		vals.clear();
	}
}

bool CondLit::grounded(Grounder *g)
{
	#pragma message "output the conditional"
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
	return true;
}

void CondLit::accept(AggrLit::Printer *v)
{
	#pragma message "implement me!!!"
	/*
	size_t p = 0;
	foreach(Val &val, weights_)
	{
		head_->move(p++);
		head_->accept(v);
		v->weight(val);
	}
	*/
}

void CondLit::finish()
{
	// Note: indices of CondLit are finished in AggrIndex
}

void CondLit::finish(Grounder *g)
{
	if(!lits_[0].sign()) { lits_[0].finish(g); }
}

CondLit::~CondLit()
{
}

CondLit* new_clone(const CondLit& a)
{
	return new CondLit(a);
}

//////////////////////////////////////// GlobalsCollector ////////////////////////////////////////

GlobalsCollector::GlobalsCollector(VarSet &globals, uint32_t level)
	: globals_(globals)
	, level_(level)
{
}

void GlobalsCollector::visit(VarTerm *var, bool)
{
	if(var->level() <= level_) { globals_.insert(var->index()); }
}

void GlobalsCollector::visit(Term* term, bool bind)
{
	term->visit(this, bind);
}

void GlobalsCollector::visit(Lit *lit, bool)
{
	lit->visit(this);
}

void GlobalsCollector::visit(Groundable *grd, bool)
{
	grd->visit(this);
}

void GlobalsCollector::collect(AggrLit &lit, VarSet &globals, uint32_t level)
{
	GlobalsCollector gc(globals, level);
	foreach(CondLit &cond, lit.conds()) { cond.visit(&gc); }
	if(!lit.assign())
	{
		if(lit.lower()) { lit.lower()->visit(&gc, false); }
		if(lit.upper()) { lit.upper()->visit(&gc, false); }
	}
}

void GlobalsCollector::collect(Lit &lit, VarVec &globals, uint32_t level)
{
	VarSet set;
	GlobalsCollector gc(set, level);
	gc.visit(&lit, false);
	globals.assign(set.begin(), set.end());
}

//////////////////////////////////////// CondHeadExpander ////////////////////////////////////////

CondHeadExpander::CondHeadExpander(CondLit &cond)
	: cond_(cond)
{ }

void CondHeadExpander::expand(Lit *lit, Expander::Type type)
{
	switch(type)
	{
		case RANGE:
		case RELATION:
			cond_.add(lit);
			break;
		case POOL:
			LitPtrVec lits;
			lits.push_back(lit);
			lits.insert(lits.end(), cond_.lits()->begin() + 1, cond_.lits()->end());
			TermPtrVec terms(*cond_.terms());
			cond_.aggr()->add(new CondLit(cond_.loc(), terms, lits, cond_.style()));
			break;
	}
}

//////////////////////////////////////// CondSetExpander ////////////////////////////////////////

CondSetExpander::CondSetExpander(CondLit &cond)
	: cond_(cond)
{ }

void CondSetExpander::expand(Lit *lit, Expander::Type type)
{
	switch(type)
	{
		case RANGE:
		case RELATION:
			cond_.add(lit);
			break;
		case POOL:
			std::auto_ptr<SetLit> set(static_cast<SetLit*>(lit));
			LitPtrVec lits(*cond_.lits());
			cond_.aggr()->add(new CondLit(cond_.loc(), *set->terms(), lits, cond_.style()));
			break;
	}
}

//////////////////////////////////////// CondBodyExpander ////////////////////////////////////////

CondBodyExpander::CondBodyExpander(CondLit &cond)
	: cond_(cond)
{ }

void CondBodyExpander::expand(Lit *lit, Expander::Type) { cond_.add(lit); }

//////////////////////////////////////// LparseCondLitConverter ////////////////////////////////////////

LparseCondLitConverter::LparseCondLitConverter(Grounder *g, CondLit &cond)
	: grounder_(g)
	, cond_(cond)
	, addVars_(false)
{ }

void LparseCondLitConverter::visit(VarTerm *var, bool)
{
	vars_.insert(var->nameId());
}

void LparseCondLitConverter::visit(Term* term, bool bind)
{
	if(addVars_) { term->visit(this, bind); }
	else         { cond_.terms()->push_back(term->clone()); }
}

void LparseCondLitConverter::visit(PredLit *pred)
{
	if(cond_.style() == CondLit::STYLE_LPARSE_SET)
	{
		addVars_ = false;
		std::stringstream ss;
		ss << "#pred(" << grounder_->string(pred->dom()->nameId()) << "," << pred->dom()->arity() << ")";
		uint32_t name  = grounder_->index(ss.str());
		cond_.terms()->push_back(new ConstTerm(cond_.loc(), Val::create(Val::ID, name)));
	}
}

void LparseCondLitConverter::visit(Lit *lit, bool)
{
	addVars_ = true;
	lit->visit(this);
}

void LparseCondLitConverter::convert(Grounder *g, CondLit &cond, uint32_t number)
{
	LparseCondLitConverter conv(g, cond);
	if(cond.style() == CondLit::STYLE_LPARSE_SET) { conv.visit(&*cond.lits()->begin(), false); }
	else if(cond.style() == CondLit::STYLE_LPARSE)
	{
		foreach(Lit &lit, *cond.lits()) { conv.visit(&lit, false); }
	}
	if(conv.addVars_)
	{
		std::stringstream ss;
		ss << "#cond(" << number << ")";
		uint32_t name  = g->index(ss.str());
		cond.terms()->push_back(new ConstTerm(cond.loc(), Val::create(Val::ID, name)));
		foreach(uint32_t var, conv.vars_) { cond.terms()->push_back(new VarTerm(cond.loc(), var)); }
	}
}

//////////////////////////////////////// AggrIndex ////////////////////////////////////////

AggrIndex::AggrIndex(AggrLit &lit)
	: lit_(lit)
	, finished_(false)
{
}

AggrState *AggrIndex::current() const { return lit_.domain().last(); }

Index::Match AggrIndex::firstMatch(Grounder *g, int binder)
{
	lit_.domain().state(g, &lit_);
	lit_.ground(g);
	if(lit_.assign())
	{
		Val v;
		for(bool match = current()->bindFirst(&v); match; match = current()->bindNext(&v))
		{
			if(lit_.assign()->unify(g, v, binder)) { return Match(true, !finished_ || current()->isNew()); }
		}
		return Match(false, false);
	}
	else if(current()->bindFirst(0)) { return Match(true, !finished_ || current()->isNew()); }
	else { return Match(false, false); }
}

Index::Match AggrIndex::nextMatch(Grounder *g, int binder)
{
	if(lit_.assign())
	{
		Val v;
		while(current()->bindNext(&v))
		{
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
	foreach(CondLit &lit, lit_.conds())
	{
		lit.instantiator()->finish();
	}
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
