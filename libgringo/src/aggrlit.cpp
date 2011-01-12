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


/*
 * aggrlit with set of condlits
 * condlit as usual + set of terms
 * term tuple determines uniqueness
 * in body conditionals + head correspond to conjunction
 * in head conditionals will be treated like body literals
 * if an aggrgegate is grounded
 *   set a counter to the number of conditionals in the aggregate
 *   mark all conditionals
 * if all literals in a conditional are complete grounding similar to current algorithm
 *   immediately ground the conditional
 *   decrement aggregate counter
 * otherwise the aggregate enqueues the recursive conditional
 *   enqueue the conditional
 * if conditional is grounded and marked
 *   decrement counter in aggregate
 *   if counter hits zero and aggregate possibly matches
 *     enqueue the rule with the aggregate again
 *     or if all conditionals where complete immediately match
 * optimizations for monotone/antimonotone aggregates possible
 *   a monotone aggregate stays true
 *   an antimonotone aggregate stays false
 * 
 * 
 * grounding happens w.r.t. some global substitution
 *   globals substitutions have to be stored in the condlit
 */

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


namespace
{
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
		static void convert(Grounder *g, CondLit &cond);
	private:
		Grounder *grounder_;
		CondLit  &cond_;
		VarSet    vars_;
		int       number_;
		bool      addVars_;
	};
}

//////////////////////////////////////// AggrLit ////////////////////////////////////////

AggrLit::AggrLit(const Loc &loc, CondLitVec &conds)
	: Lit(loc)
	, sign_(false)
	, assign_(false)
	, fact_(false)
	, conds_(conds.release())
{
	foreach(CondLit &lit, conds_) { lit.aggr_ = this; }
}

AggrLit::AggrLit(const AggrLit &aggr)
	: Lit(aggr)
	, sign_(aggr.sign_)
	, assign_(aggr.assign_)
	, fact_(aggr.fact_)
	, lower_(aggr.lower_)
	, upper_(aggr.upper_)
	, conds_(aggr.conds_)
{
	foreach(CondLit &lit, conds_) { lit.aggr_ = this; }
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
	cond->aggr_ = this;
	conds_.push_back(cond);
}

void AggrLit::addDomain(Grounder *g, bool)
{ 
	assert(head());
	foreach(CondLit &lit, conds_) { lit.addDomain(g, false); }
}

void AggrLit::finish(Grounder *g) 
{
	#pragma message "probably unused here?"
	foreach(CondLit &lit, conds_) { lit.finish(g); }
}

void AggrLit::visit(PrgVisitor *visitor)
{
	if(lower_.get()) { visitor->visit(lower_.get(), assign_); }
	if(upper_.get()) { visitor->visit(upper_.get(), false); }
	foreach(CondLit &lit, conds_) { visitor->visit(&lit, head()); }
}

void AggrLit::normalize(Grounder *g, Expander *expander) 
{
	assert(!(assign_ && sign_));
	assert(!(assign_ && head()));
	if(lower_.get()) { lower_->normalize(this, Term::PtrRef(lower_), g, expander, assign_); }
	if(upper_.get()) { upper_->normalize(this, Term::PtrRef(upper_), g, expander, false); }
	for(CondLitVec::size_type i = 0; i < conds_.size(); i++)
	{
		conds_[i].normalize(g);
	}
}

AggrLit::~AggrLit()
{
}

//////////////////////////////////////// SetLit ////////////////////////////////////////


//////////////////////////////////////// CondLit ////////////////////////////////////////

CondLit::CondLit(const Loc &loc, TermPtrVec &terms, LitPtrVec &lits, Style style)
	: Locateable(loc)
	, style_(style)
	, set_(loc, terms)
	, lits_(lits)
	, aggr_(0)
{
}

void CondLit::normalize(Grounder *g)
{
	assert(!lits_.empty());
	LitPtrVec::iterator i = lits_.begin();
	if(style_ != STYLE_DLV)
	{
		CondHeadExpander headExp(*this);
		i->normalize(g, &headExp);
	}
	CondSetExpander setExp(*this);
	set_.normalize(g, &setExp);
	CondBodyExpander bodyExp(*this);
	for(; i != lits_.end(); i++) { i->normalize(g, &bodyExp); }
	if(style_ != STYLE_DLV) { LparseCondLitConverter::convert(g, *this); }
}

/*
void CondLit::ground(Grounder *g)
{
	weights_.clear();
	head_->clear();
	if(inst_.get()) 
	{
		inst_->reset();
		inst_->ground(g);
	}
	else
	{
		if(head_->match(g)) grounded(g);
	}
}

void CondLit::visit(PrgVisitor *visitor)
{
	#pragma message "this will be no longer needed in the new aggregate implementation"
	if(head_->complete() && aggr_->optimizeComplete()) head_->head(false);
	visitor->visit(head_.get(), false);
	visitor->visit(weight_.get(), false);
	foreach(Lit &lit, body_) visitor->visit(&lit, true);
}

void CondLit::print(Storage *sto, std::ostream &out) const
{
	head_->print(sto, out);
	if(!aggr_->set())
	{
		out << "=";
		weight_->print(sto, out);
	}
	std::vector<const Lit*> body;
	foreach(const Lit &lit, body_) { body.push_back(&lit); }
	std::sort(body.begin(), body.end(), Lit::cmpPos);
	foreach(const Lit *lit, body)
	{
		out << ":";
		lit->print(sto, out);
	}
}

void CondLit::addDomain(Grounder *g, bool fact) 
{ 
	for(size_t p = 0; p < weights_.size(); p++)
	{
		head_->move(p);
		head_->addDomain(g, fact);
	}
}

void CondLit::finish(Grounder *g)
{
}

void CondLit::init(Grounder *g, const VarSet &b)
{
	if(body_.size() > 0 || vars_.size() > 0)
	{
		inst_.reset(new Instantiator(this));
		if(vars_.size() > 0) litDep_->order(g, b);
		else
		{
			VarSet bound(b);
			foreach(Lit &lit, body_) 
			{
				lit.init(g, bound);
				lit.index(g, this, bound);
			}
			head_->init(g, bound);
			head_->index(g, this, bound);
		}
	}
	else head_->init(g, b);
	litDep_.reset(0);
}

bool CondLit::grounded(Grounder *g)
{
	try
	{
		Val v = weight_->weight()->val(g);
		head_->grounded(g);
		tribool res = aggr_->accumulate(g, v, *head_);
		if(unknown(res))
		{
			head_->push();
			weights_.push_back(v);
		}
		else if(!res) return false;
		return true;
	}
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
}

void CondLit::accept(AggrLit::Printer *v)
{
	size_t p = 0;
	foreach(Val &val, weights_)
	{
		head_->move(p++);
		head_->accept(v);
		v->weight(val);
	}
}

CondLit::~CondLit()
{
}

CondLit* new_clone(const CondLit& a)
{
	return new CondLit(a);
}
*/

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
			LitPtrVec lits(cond_.lits()->begin() + 1, cond_.lits()->end());
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
		uint32_t name  = grounder_->index("#" + grounder_->string(pred->dom()->nameId()));
		int32_t  arity = pred->dom()->arity();
		cond_.terms()->push_back(new ConstTerm(cond_.loc(), Val::create(Val::STRING, name)));
		cond_.terms()->push_back(new ConstTerm(cond_.loc(), Val::create(Val::NUM, arity)));
	}
}

void LparseCondLitConverter::visit(Lit *lit, bool)
{
	addVars_ = true;
	lit->visit(this);
}

void LparseCondLitConverter::convert(Grounder *g, CondLit &cond)
{
	LparseCondLitConverter conv(g, cond);
	if(cond.style() == CondLit::STYLE_LPARSE) { conv.visit(&*cond.lits()->begin(), false); }
	else if(cond.style() == CondLit::STYLE_LPARSE_SET)
	{
		foreach(Lit &lit, *cond.lits()) { conv.visit(&lit, false); }
	}
	if(conv.addVars_)
	{
		foreach(uint32_t var, conv.vars_) { cond.terms()->push_back(new VarTerm(cond.loc(), var)); }
	}
}

// if not predlit:
//   X=Y+1=W -> X=I=W:I=Y+1
//   terms=W,##<typeid>,<all variables>
// else:
//   if not set:
//     p(f(X);Y)=W
//     terms: W,#<predicate name>,<arity>,<all variables!>
//   else:
//     p(f(X);Y)=W
//     terms: W,#<predicate name>,<arity>,<terms in predicate>
