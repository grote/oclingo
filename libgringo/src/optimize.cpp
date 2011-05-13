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

#include <gringo/optimize.h>
#include <gringo/lit.h>
#include <gringo/term.h>
#include <gringo/output.h>
#include <gringo/predlit.h>
#include <gringo/grounder.h>
#include <gringo/prgvisitor.h>
#include <gringo/litdep.h>
#include <gringo/exceptions.h>
#include <gringo/instantiator.h>

namespace
{
	class OptimizeSetExpander : public Expander
	{
	public:
		OptimizeSetExpander(Grounder *g, Optimize &opt);
		void expand(Lit *lit, Type type);
	private:
		Grounder *g_;
		Optimize &opt_;
	};

	class OptimizeHeadExpander : public Expander
	{
	public:
		OptimizeHeadExpander(Grounder *g, Optimize &opt);
		void expand(Lit *lit, Type type);
	private:
		Grounder *g_;
		Optimize &opt_;
	};

	class OptimizeBodyExpander : public Expander
	{
	public:
		OptimizeBodyExpander(Optimize &min);
		void expand(Lit *lit, Type type);
	private:
		Optimize &opt_;
	};

}

//////////////////////////////////////// OptimizeSetLit ////////////////////////////////////////

OptimizeSetLit::OptimizeSetLit(const Loc &loc, TermPtrVec &terms)
	: Lit(loc)
	, terms_(terms.release())
{
}

bool OptimizeSetLit::fact() const
{
	return true;
}

bool OptimizeSetLit::match(Grounder *g)
{
	return true;
}

bool OptimizeSetLit::edbFact() const
{
	return false;
}

void OptimizeSetLit::addDomain(Grounder *, bool)
{
	assert(false);
}

void OptimizeSetLit::accept(Printer *)
{
}

Index *OptimizeSetLit::index(Grounder *, Formula *gr, VarSet &)
{
	return new MatchIndex(this);
}

void OptimizeSetLit::normalize(Grounder *g, Expander *e)
{
	for(TermPtrVec::iterator i = terms_.begin(); i != terms_.end(); i++)
	{
		i->normalize(this, Term::VecRef(terms_, i), g, e, false);
	}
}

void OptimizeSetLit::visit(PrgVisitor *visitor)
{
	foreach(Term &term, terms_) { term.visit(visitor, false); }
}

void OptimizeSetLit::print(Storage *sto, std::ostream &out) const
{
	out << "<";
	bool comma = false;
	foreach(const Term &term, terms_)
	{
		if(comma) { out << ","; }
		else      { comma = true; }
		term.print(sto, out);
	}
	out << ">";
}

Lit *OptimizeSetLit::clone() const
{
	return new OptimizeSetLit(*this);
}

OptimizeSetLit::~OptimizeSetLit()
{
}

//////////////////////////////////////// Optimize ////////////////////////////////////////

Optimize::Optimize(const Loc &loc, TermPtrVec &terms, LitPtrVec &body, bool maximize, bool set, bool headLike)
	: SimpleStatement(loc)
	, setLit_(loc, terms)
	, body_(body.release())
	, maximize_(maximize)
	, set_(set)
	, headLike_(headLike)
{
}

Optimize::Optimize(const Optimize &opt, Lit *head)
	: SimpleStatement(opt.loc())
	, setLit_(opt.setLit_)
	, maximize_(opt.maximize_)
	, set_(opt.set_)
	, headLike_(opt.headLike_)
{
	assert(!body_.empty());
	body_.push_back(head);
	for(LitPtrVec::const_iterator it = opt.body_.begin() + 1; it != opt.body_.end(); it++) { body_.push_back(it->clone()); }
}

Optimize::Optimize(const Optimize &opt, const OptimizeSetLit &setLit)
	: SimpleStatement(opt.loc())
	, setLit_(setLit)
	, body_(opt.body_)
	, maximize_(opt.maximize_)
	, set_(opt.set_)
	, headLike_(opt.headLike_)
{
}


void Optimize::normalize(Grounder *g)
{
	if(headLike_ && !body_.empty())
	{
		OptimizeHeadExpander headExp(g, *this);
		body_[0].normalize(g, &headExp);
	}
	// do set expansion here ...
	OptimizeBodyExpander bodyExp(*this);
	for(LitPtrVec::size_type i = headLike_; i < body_.size(); i++) { body_[i].normalize(g, &bodyExp); }
}

void Optimize::append(Lit *lit)
{
	body_.push_back(lit);
}

bool Optimize::grounded(Grounder *g)
{
#pragma message "implement me!!!"
}

void Optimize::visit(PrgVisitor *visitor)
{
#pragma message "implement me!!!"
}

void Optimize::print(Storage *sto, std::ostream &out) const
{
#pragma message "implement me!!!"
}

Optimize::~Optimize()
{
}

//////////////////////////////////////// OptimizeSetExpander ////////////////////////////////////////

OptimizeSetExpander::OptimizeSetExpander(Grounder *g, Optimize &opt)
	: g_(g)
	, opt_(opt)
{ }

void OptimizeSetExpander::expand(Lit *lit, Expander::Type type)
{
	switch(type)
	{
		case RANGE:
		case RELATION:
			opt_.append(lit);
			break;
		case POOL:
			std::auto_ptr<OptimizeSetLit> setLit(static_cast<OptimizeSetLit*>(lit));
			g_->addInternal(new Optimize(opt_, *setLit));
			break;
	}
}


//////////////////////////////////////// OptimizeHeadExpander ////////////////////////////////////////

OptimizeHeadExpander::OptimizeHeadExpander(Grounder *g, Optimize &opt)
	: g_(g)
	, opt_(opt)
{
}

void OptimizeHeadExpander::expand(Lit *lit, Expander::Type type)
{
	switch(type)
	{
		case RANGE:
		case RELATION:
			opt_.append(lit);
			break;
		case POOL:
			g_->addInternal(new Optimize(opt_, lit));
			break;
	}
}

//////////////////////////////////////// OptimizeBodyExpander ////////////////////////////////////////

OptimizeBodyExpander::OptimizeBodyExpander(Optimize &opt)
	: opt_(opt)
{
}

void OptimizeBodyExpander::expand(Lit *lit, Expander::Type type)
{
	(void)type;
	opt_.append(lit);
}

/*

#pragma message "implement this in the same manner as condlits"
class Optimize::PrioLit : public Lit
{
public:
	PrioLit(const Loc &loc, Term *weight, Term *prio) : Lit(loc), weight_(weight), prio_(prio) { }
	bool fact() const { return true; }
	bool match(Grounder *grounder) { (void)grounder; return true; }
	void addDomain(Grounder *, bool fact) { (void)fact; assert(false); }
	Index *index(Grounder *g, Formula *gr, VarSet &bound) { (void)g; (void)gr; (void)bound; return 0; }
	bool edbFact() const { return false; }
	void normalize(Grounder *g, Expander *expander) { (void)g; (void)expander; assert(false); }
	void visit(PrgVisitor *visitor)
	{
		visitor->visit(weight_.get(), false);
		visitor->visit(prio_.get(), false);
	}
	void print(Storage *sto, std::ostream &out) const
	{
		weight_->print(sto, out);
		out << "@";
		prio_->print(sto, out);
	}
	void accept(::Printer *v) { (void)v; }
	clone_ptr<Term> &weight() { return weight_; }
	clone_ptr<Term> &prio()   { return prio_; }
	Lit *clone() const { return new PrioLit(*this); }
	~PrioLit() { }
private:
	clone_ptr<Term> weight_;
	clone_ptr<Term> prio_;
};

Optimize::PrioLit* new_clone(const Optimize::PrioLit& a)
{
	return new Optimize::PrioLit(a);
}

Optimize::Optimize(const Loc &loc, PredLit *head, Term *weight, Term *prio, LitPtrVec &body, bool maximize)
	: SimpleStatement(loc)
	, head_(head)
	, prio_(new PrioLit(loc, weight, prio))
	, body_(body.release())
	, maximize_(maximize)
	, set_(false)
{
}

Term *Optimize::weight()
{
	return prio_->weight().get();
}

Term *Optimize::prio()
{
	return prio_->prio().get();
}

void Optimize::ground(Grounder *g)
{
	enqueued_ = false;
	#pragma message "optimize statements can be printed right away without storing them!"
	prios_.clear();
	head_->clear();
	if(inst_.get()) 
	{
		inst_->reset();
		inst_->ground(g);
	}
	else if(head_->match(g)) { grounded(g); }
	Printer *printer = g->output()->printer<Printer>();
	printer->begin(maximize_, set_);
	size_t p = 0;
	foreach(PrioVec::value_type prio, prios_)
	{
		head_->move(p++);
		printer->print(head_.get(), prio.first, prio.second);
	}
	printer->end();
}

bool Optimize::grounded(Grounder *g)
{
	try
	{
		int32_t w = weight()->val(g).number();
		int32_t p = prio()->val(g).number();
		head_->grounded(g);
		if(!set_ || head_->testUnique(*uniques_.get(), prio()->val(g)))
		{
			head_->push();
			prios_.push_back(std::make_pair(w, p));
		}
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

namespace
{
	class OptimizeHeadExpander : public Expander
	{
	public:
		OptimizeHeadExpander(Grounder *g, Optimize &min);
		void expand(Lit *lit, Type type);
	private:
		Grounder *g_;
		Optimize &min_;
	};

	class OptimizeBodyExpander : public Expander
	{
	public:
		OptimizeBodyExpander(Optimize &min);
		void expand(Lit *lit, Type type);
	private:
		Optimize &min_;
	};

	OptimizeHeadExpander::OptimizeHeadExpander(Grounder *g, Optimize &min)
		: g_(g)
		, min_(min)
	{
	}

	void OptimizeHeadExpander::expand(Lit *lit, Expander::Type type)
	{
		switch(type)
		{
			case RANGE:
			case RELATION:
				min_.body().push_back(lit);
				break;
			case POOL:
				LitPtrVec body;
				foreach(Lit &l, min_.body()) body.push_back(l.clone());
				Optimize *o = new Optimize(min_.loc(), static_cast<PredLit*>(lit), min_.weight()->clone(), min_.prio()->clone(), body, min_.maximize());
				if(min_.set()) o->uniques(min_.uniques());
				g_->addInternal(o);
				break;
		}
	}

	OptimizeBodyExpander::OptimizeBodyExpander(Optimize &min)
		: min_(min)
	{
	}

	void OptimizeBodyExpander::expand(Lit *lit, Expander::Type type)
	{
		(void)type;
		min_.body().push_back(lit);
	}
}

void Optimize::normalize(Grounder *g)
{
	OptimizeHeadExpander headExp(g, *this);
	OptimizeBodyExpander bodyExp(*this);
	head_->normalize(g, &headExp);
	weight()->normalize(head_.get(), Term::PtrRef(prio_->weight()), g, &headExp, false);
	prio()->normalize(head_.get(), Term::PtrRef(prio_->prio()), g, &headExp, false);
	for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
		body_[i].normalize(g, &bodyExp);
}

void Optimize::visit(PrgVisitor *visitor)
{
	visitor->visit(static_cast<Lit*>(head_.get()), false);
	visitor->visit(prio_.get(), false);
	foreach(Lit &lit, body_) { visitor->visit(&lit, true); }
}

void Optimize::print(Storage *sto, std::ostream &out) const
{
	out << (maximize_ ? "#maximize" : "#minimize");
	out << (set_ ? "{" : "[");
	head_->print(sto, out);
	out << "=";
	prio_->print(sto, out);
	std::vector<const Lit*> body;
	foreach(const Lit &lit, body_) { body.push_back(&lit); }
	std::sort(body.begin(), body.end(), Lit::cmpPos);
	foreach(const Lit *lit, body)
	{
		out << ":";
		lit->print(sto, out);
	}
	out << (set_ ? "}" : "]");
}

void Optimize::append(Lit *lit)
{
	body_.push_back(lit);
}

Optimize::~Optimize()
{
}

*/
