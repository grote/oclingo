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
#include <gringo/varterm.h>
#include <gringo/constterm.h>
#include <gringo/domain.h>

namespace
{
/*
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
*/

	class OptimizeAnonymousRemover : public PrgVisitor
	{
	public:
		OptimizeAnonymousRemover(Grounder *g);
		void visit(VarTerm *var, bool bind);
		void visit(Term* term, bool bind);
		void visit(Lit *lit, bool domain);
	public:
		static void remove(Grounder *g, Optimize &opt);
	private:
		Grounder *grounder_;
		uint32_t  vars_;
	};


	class OptimizeLparseConverter : public PrgVisitor
	{
	public:
		OptimizeLparseConverter(Grounder *g, TermPtrVec &terms);
		void visit(VarTerm *var, bool bind);
		void visit(Term* term, bool bind);
		void visit(PredLit *pred);
		void visit(Lit *lit, bool domain);
	public:
		static void convert(const Loc &loc, Grounder *g, TermPtrVec &terms, LitPtrVec &body, bool setRewrite, Optimize::SharedNumber number);
	private:
		Grounder   *grounder_;
		TermPtrVec &terms_;
		VarSet      vars_;
		bool        addVars_;
		bool        setRewrite_;
	};
}

//////////////////////////////////////// OptimizeSetLit ////////////////////////////////////////

OptimizeSetLit::OptimizeSetLit(const Loc &loc, TermPtrVec &terms)
	: Lit(loc)
	, terms_(terms.release())
{
}

TermPtrVec &OptimizeSetLit::terms()
{
	return terms_;
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

void OptimizeSetLit::grounded(Grounder *g)
{
	vals_.clear();
	foreach(const Term &term, terms_) { vals_.push_back(term.val(g)); }
}

const ValVec &OptimizeSetLit::vals() const
{
	return vals_;
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

void OptimizeSetLit::normalize(Grounder *g, const Expander &e)
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

Optimize::Optimize(const Loc &loc, TermPtrVec &terms, LitPtrVec &body, bool maximize, Type type, SharedNumber num)
	: SimpleStatement(loc)
	, number_(num)
	, setLit_(loc, terms)
	, body_(body.release())
	, maximize_(maximize)
	, type_(type)
{
}

Optimize::Optimize(const Optimize &opt, Lit *head)
	: SimpleStatement(opt.loc())
	, number_(opt.number_)
	, setLit_(opt.setLit_)
	, maximize_(opt.maximize_)
	, type_(opt.type_)
{
	assert(!body_.empty());
	body_.push_back(head);
	for(LitPtrVec::const_iterator it = opt.body_.begin() + 1; it != opt.body_.end(); it++) { body_.push_back(it->clone()); }
}

Optimize::Optimize(const Optimize &opt, const OptimizeSetLit &setLit)
	: SimpleStatement(opt.loc())
	, number_(opt.number_)
	, setLit_(setLit)
	, body_(opt.body_)
	, maximize_(opt.maximize_)
	, type_(opt.type_)
{
}

void Optimize::expandSet(Grounder *g, Lit *lit, Lit::ExpansionType type)
{
	switch(type)
	{
		case Lit::RANGE:
		case Lit::RELATION:
		{
			append(lit);
			break;
		}
		case Lit::POOL:
		{
			std::auto_ptr<OptimizeSetLit> setLit(static_cast<OptimizeSetLit*>(lit));
			g->addInternal(new Optimize(*this, *setLit));
			break;
		}
	}
}

void Optimize::expandHead(Grounder *g, Lit *lit, Lit::ExpansionType type)
{
	switch(type)
	{
		case Lit::RANGE:
		case Lit::RELATION:
		{
			append(lit);
			break;
		}
		case Lit::POOL:
		{
			g->addInternal(new Optimize(*this, lit));
			break;
		}
	}
}

void Optimize::normalize(Grounder *g)
{
	bool headLike = type_ != CONSTRAINT;
	if(headLike && !body_.empty()) { body_[0].normalize(g, boost::bind(&Optimize::expandHead, this, g, _1, _2)); }
	setLit_.normalize(g, boost::bind(&Optimize::expandSet, this, g, _1, _2));
	Lit::Expander bodyExp = boost::bind(&Optimize::append, this, _1);
	for(LitPtrVec::size_type i = headLike; i < body_.size(); i++) { body_[i].normalize(g, bodyExp); }
	if(type_ == SET || type_ ==  MULTISET || type_ == CONSTRAINT)
	{
		OptimizeAnonymousRemover::remove(g, *this);
		OptimizeLparseConverter::convert(loc(), g, setLit_.terms(), body_, type_ == SET, number_);
	}
}

void Optimize::append(Lit *lit)
{
	body_.push_back(lit);
}

bool Optimize::grounded(Grounder *g)
{
	Printer *printer = g->output()->printer<Printer>();
	printer->begin(type_, maximize_);

	setLit_.grounded(g);

	if(setLit_.vals()[0].type != Val::NUM)
	{
		std::stringstream os;
		print(g, os);
		throw TypeException("only integers weights permitted", StrLoc(g, setLit_.loc()), os.str());
	}

	printer->print(setLit_.vals());

	foreach(Lit &lit, body_)
	{
		lit.grounded(g);
		lit.accept(printer);
	}

	printer->end();
	return true;
}

void Optimize::visit(PrgVisitor *visitor)
{
	visitor->visit(&setLit_, false);
	foreach(Lit &lit, body_) { visitor->visit(&lit, false); }
}

void Optimize::print(Storage *sto, std::ostream &out) const
{
	out << (maximize_ ? "#maximize" : "#minimize") << " { ";
	setLit_.print(sto, out);
	foreach(const Lit &lit, body_)
	{
		out << " : ";
		lit.print(sto, out);
	}
	out << " }.";
}

Optimize::~Optimize()
{
}

//////////////////////////////////////// LparseOptimizeConverter ////////////////////////////////////////

OptimizeAnonymousRemover::OptimizeAnonymousRemover(Grounder *g)
	: grounder_(g)
	, vars_(0)
{
}

void OptimizeAnonymousRemover::visit(VarTerm *var, bool)
{
	if(var->anonymous())
	{
		std::ostringstream oss;
		oss << "#opt_anonymous(" << vars_++ << ")";
		var->nameId(grounder_->index(oss.str()));
	}
}

void OptimizeAnonymousRemover::visit(Term* term, bool bind)
{
	term->visit(this, bind);
}

void OptimizeAnonymousRemover::visit(Lit *lit, bool)
{
	lit->visit(this);
}

void OptimizeAnonymousRemover::remove(Grounder *g, Optimize &opt)
{
	OptimizeAnonymousRemover ar(g);
	opt.visit(&ar);
}

//////////////////////////////////////// OptimizeLparseConverter ////////////////////////////////////////

OptimizeLparseConverter::OptimizeLparseConverter(Grounder *g, TermPtrVec &terms)
	: grounder_(g)
	, terms_(terms)
	, addVars_(false)
	, setRewrite_(false)
{ }

void OptimizeLparseConverter::visit(VarTerm *var, bool)
{
	vars_.insert(var->nameId());
}

void OptimizeLparseConverter::visit(Term* term, bool bind)
{
	if(addVars_) { term->visit(this, bind); }
	else         { terms_.push_back(term->clone()); }
}

void OptimizeLparseConverter::visit(PredLit *pred)
{
	if(setRewrite_)
	{
		addVars_ = false;
		std::stringstream ss;
		ss << "#pred(" << grounder_->string(pred->dom()->nameId()) << "," << pred->dom()->arity() << ")";
		uint32_t name  = grounder_->index(ss.str());
		terms_.push_back(new ConstTerm(pred->loc(), Val::create(Val::ID, name)));
	}
}

void OptimizeLparseConverter::visit(Lit *lit, bool)
{
	addVars_ = true;
	lit->visit(this);
}

void OptimizeLparseConverter::convert(const Loc &loc, Grounder *g, TermPtrVec &terms, LitPtrVec &body, bool setRewrite, Optimize::SharedNumber number)
{
	OptimizeLparseConverter conv(g, terms);
	if(setRewrite)
	{
		LitPtrVec::iterator it = body.begin();
		conv.setRewrite_ = true;
		conv.visit(&*it, false);
		if(conv.addVars_)
		{
			conv.setRewrite_ = false;
			for(it++; it != body.end(); it++) { conv.visit(&*it, false); }
		}
	}
	else
	{
		conv.setRewrite_ = false;
		foreach(Lit &lit, body) { conv.visit(&lit, false); }
	}
	if(conv.addVars_)
	{
		std::stringstream ss;
		ss << "#multiset(" << (*number)++ << ")";
		uint32_t name  = g->index(ss.str());
		terms.push_back(new ConstTerm(loc, Val::create(Val::ID, name)));
		foreach(uint32_t var, conv.vars_) { terms.push_back(new VarTerm(loc, var)); }
	}
}
