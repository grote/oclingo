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
  in general:
	immediately ground complete conditionals
  in body:
	if conditional matches
	  accumulate #cond(<aggr-num>,<cond-num>)
	  inform output
	if aggregate matches:
	  enqueue associated groundable
	  (not neccesary if aggregate has already matched)
	in the end:
	  #aggr(<aggr-num>) :- <lower> <aggregate> [ #cond(<aggr-num>, <cond-num>), ... ] <upper>.
  in head:
	aggregate always matches
	if conditional matches
	  accumulate #cond(<aggr-num>,<cond-num>)
	  inform output
	  if aggregate matches
		add domains (might include previous ones)
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
#include <gringo/index.h>

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
		AggrIndex(AggrLit *lit);
		Match firstMatch(Grounder *grounder, int binder);
		Match nextMatch(Grounder *grounder, int binder);
		void reset();
		void finish();
		bool hasNew() const;
		~AggrIndex();
	private:
		AggrLit *lit_;
		bool     finished_;
	};

	class GlobalsBinder : public Index
	{
	public:
		GlobalsBinder(CondLit *lit);
		Match firstMatch(Grounder *grounder, int binder);
		Match nextMatch(Grounder *grounder, int binder);
		void reset();
		void finish();
		bool hasNew() const;
		~GlobalsBinder();
	private:
		CondLit *lit_;
		uint32_t offset_;
	};
}

//////////////////////////////////////// AggrLit::AggrState ////////////////////////////////////////

AggrLit::AggrState::AggrState()
	: new_(false)
	, lock_(false)
	, match_(false)
	, fact_(false)
{ }

void AggrLit::AggrState::accumulate(Grounder *g, AggrLit &lit, const ValVec &set, bool fact)
{
	std::cerr << "AggrState" << std::endl;
	std::cerr << "\taccumulating: ";
	foreach(const Val &val, set)
	{
		val.print(g, std::cerr);
		std::cerr << " ";
	}
	std::cerr << std::endl;
	std::cerr << "\tmatches (pre): " << match_ << std::endl;

	Sets::iterator it = sets_.find(set.size());
	if(it == sets_.end()) {	it = sets_.insert(Sets::value_type(set.size(), ValVecSet(set.size()))).first; }
	std::pair<const ValVecSet::Index &, bool> res = it->second.insert(set.begin());
	bool newFact = !res.first.fact && fact;
	res.first.fact = res.first.fact || fact;
	doAccumulate(g, lit, set, res.second, newFact);
	std::cerr << "\tmatches (post): " << match_ << std::endl;
}

AggrLit::AggrState::~AggrState()
{
}

//////////////////////////////////////// AggrLit ////////////////////////////////////////

AggrLit::AggrLit(const Loc &loc, CondLitVec &conds)
	: Lit(loc)
	, sign_(false)
	, assign_(false)
	, complete_(tribool::indeterminate_value)
	, parent_(0)
	, conds_(conds.release())
	, last_(0)
	, enqueued_(0)
	, new_(false)
	, index_(1)
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
	cond->aggr_ = this;
	conds_.push_back(cond);
}

void AggrLit::addDomain(Grounder *g, bool)
{
	assert(head());
	foreach(CondLit &lit, conds_) { lit.addDomain(g, false); }
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

bool AggrLit::isNew() const
{
	return last_->isNew();
}

bool AggrLit::fact() const
{
	return (monotonicity() == MONOTONE || complete()) && last_->matches() && last_->fact();
}

bool AggrLit::matchLast() const
{
	return enqueued_ > conds_.size();
}

bool AggrLit::match(Grounder *g)
{
	std::cerr << "AggrLit::match()" << std::endl;
	ValVec bound;
	foreach(uint32_t i, bound_) { bound.push_back(g->val(i)); }
	std::pair<const ValVecSet::Index&, bool> res(index_.insert(bound.begin()));
	if(!res.second)
	{
		std::cerr << "\tresusing state: \n\t";
		debug(g, last_);
		uint32_t offset = !bound.empty() ? res.first / bound.size() : 0;
		std::cerr << "use existing index: \n\t" << res.first << " / " << index_.size() * bound_.size() << " @ " << offset << std::endl;
		last_ = &aggrStates_[offset];
		std::cerr << "\t";
		debug(g, last_);
	}
	else
	{
		aggrStates_.push_back(newAggrState(g));
		last_ = &aggrStates_.back();
		std::cerr << "\tadding state: \n\t";
		debug(g, last_);
		enqueued_+= conds_.size() + 1;
		foreach(CondLit &lit, conds_) { lit.ground(g); }
		enqueued_-= conds_.size() + 1;
	}
	// NOTE: its uncritical to set the new_ flag here
	if(last_->lock()) { new_ = true; }
	// TODO: handle signed aggregates
	//       handle aggregates that switch from true -> to false and stay false
	//       (aggregates with an antimonotone part)
	return last_->matches();
}

void AggrLit::enqueueParent(Grounder *g, AggrState &state)
{
	if(enqueued_ == 0 && state.matches())
	{
		if(state.lock()) { new_ = true; }
		g->enqueue(parent_);
	}
}

void AggrLit::finish(Grounder *)
{
	new_ = false;
	foreach(AggrState &state, aggrStates_) { state.finish(); }
}

void AggrLit::index(Grounder *, Groundable *gr, VarSet &bound)
{
	parent_ = gr;
	bound_.assign(bound.begin(), bound.end());
	index_ = ValVecSet(bound_.size());
	// NOTE: think about binders (in theory the binder in the conditionals have to be shifted)
	#pragma message "handle assignments!!"
	gr->instantiator()->append(new AggrIndex(this));
}

void AggrLit::bind(Grounder *g, uint32_t offset, int binder)
{
	ValVecSet::iterator i = index_.begin() + offset * bound_.size();
	VarVec::iterator    j = bound_.begin();
	// NOTE: think about binders
	for(; j != bound_.end(); i++, j++) { g->val(*j, *i, binder); }
}

void AggrLit::unbind(Grounder *g)
{
	foreach(uint32_t i, bound_) { g->unbind(i); }
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
	, current_(0)
	, complete_(tribool::indeterminate_value)
{
}

void CondLit::head(bool head)
{
	lits_[0].head(head);
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
}

void CondLit::doEnqueue(bool enqueue)
{
	aggr_->enqueue(enqueue);
}

bool CondLit::bind(Grounder *g, uint32_t offset, int binder)
{
	if(offset <  aggr_->states().size())
	{
		current_ = &aggr_->states()[offset];
		if(!aggr_->matchLast()) { aggr_->bind(g, offset, binder); }
		return true;
	}
	else { return false; }
}

void CondLit::ground(Grounder *g)
{
	std::cerr << "CondLit::ground" << std::endl;
	inst_->ground(g);
	if(!aggr_->matchLast()) { aggr_->unbind(g); }
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
	#pragma message "implement me!!!"
	/*
	for(size_t p = 0; p < weights_.size(); p++)
	{
		head_->move(p);
		head_->addDomain(g, fact);
	}
	*/
}

void AggrLit::debug(Grounder *g, AggrState *s)
{
	std::cerr << "\tstate(" << s << "):";
	foreach(uint32_t var, bound_)
	{
		std::cerr << " ";
		g->val(var).print(g, std::cerr);
	}
	std::cerr << std::endl;
}

bool CondLit::grounded(Grounder *g)
{
	#pragma message "output the conditional"
	assert(current_);
	bool fact = true;
	foreach(Lit &lit, lits_)
	{
		lit.grounded(g);
		if(!lit.fact()) { fact = false; }
	}
	ValVec set;
	foreach(Term &term, *set_.terms()) { set.push_back(term.val(g)); }
	try
	{
		current_->accumulate(g, *aggr_, set, fact);
		aggr()->debug(g, current_);
		aggr_->enqueueParent(g, *current_);
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

void CondLit::instantiator(Instantiator *inst)
{
	#pragma message "solve this differently (later)"
	inst->append(new GlobalsBinder(this));
	Groundable::instantiator(inst);
}

CondLit::~CondLit()
{
}

CondLit* new_clone(const CondLit& a)
{
	return new CondLit(a);
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

AggrIndex::AggrIndex(AggrLit *lit)
	: lit_(lit)
	, finished_(false)
{
}

Index::Match AggrIndex::firstMatch(Grounder *g, int)
{
	bool match = lit_->match(g);
	return Match(match, !finished_ ||  lit_->isNew());
}

Index::Match AggrIndex::nextMatch(Grounder *, int)
{
	return Match(false, false);
}

void AggrIndex::reset()
{
	finished_ = false;
}

void AggrIndex::finish()
{
	finished_ = true;
}

bool AggrIndex::hasNew() const
{
	return !finished_ || lit_->hasNew();
}

AggrIndex::~AggrIndex()
{
}

//////////////////////////////////////// GlobalsBinder ////////////////////////////////////////

GlobalsBinder::GlobalsBinder(CondLit *lit)
	: lit_(lit)
	, offset_(0)
{
}

Index::Match GlobalsBinder::firstMatch(Grounder *g, int binder)
{
	offset_ = lit_->aggr()->matchLast() ? lit_->aggr()->states().size() - 1 : 0;
	return nextMatch(g, binder);
}

Index::Match GlobalsBinder::nextMatch(Grounder *g, int binder)
{
	if(offset_ < lit_->aggr()->states().size())
	{
		lit_->bind(g, offset_++, binder);
		return Match(true, lit_->aggr()->matchLast());
	}
	else { return Match(false, false); }
}

void GlobalsBinder::reset()
{
}

void GlobalsBinder::finish()
{
}

bool GlobalsBinder::hasNew() const
{
	return lit_->aggr()->matchLast();
}

GlobalsBinder::~GlobalsBinder()
{
}
