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

#include <gringo/display.h>
#include <gringo/lit.h>
#include <gringo/term.h>
#include <gringo/output.h>
#include <gringo/predlit.h>
#include <gringo/grounder.h>
#include <gringo/prgvisitor.h>
#include <gringo/litdep.h>
#include <gringo/exceptions.h>
#include <gringo/instantiator.h>
#include <gringo/functerm.h>
#include <gringo/mathterm.h>
#include <gringo/constterm.h>

namespace
{
	class ShowHeadExpander : public Expander
	{
	public:
		ShowHeadExpander(Grounder *g, LitPtrVec &body);
		void expand(Lit *lit, Type type);
	private:
		Grounder  *g_;
		LitPtrVec &body_;
	};

	class ShowBodyExpander : public Expander
	{
	public:
		ShowBodyExpander(Show &d);
		void expand(Lit *lit, Type);
	private:
		Show &d_;
	};

}


//////////////////////////////// Show ////////////////////////////////

Show::Show(const Loc &loc, Term *term, LitPtrVec &body, bool funcToPred)
	: SimpleStatement(loc)
	, head_(new ShowHeadLit(loc, term))
	, body_(body.release())
	, funcToPred_(funcToPred)
{
}

Show::Show(const Loc &loc, ShowHeadLit *lit, LitPtrVec &body)
	: SimpleStatement(loc)
	, head_(lit)
	, body_(body.release())
{
}


bool Show::grounded(Grounder *g)
{
	Printer *printer = g->output()->printer<Printer>();
	printer->begin(static_cast<ShowHeadLit&>(*head_).val(g), funcToPred_);
	foreach(Lit &lit, body_)
	{
		lit.grounded(g);
		if(!lit.fact() || lit.forcePrint()) { lit.accept(printer); }
	}
	printer->end();
	return true;
}


void Show::normalize(Grounder *g)
{
	ShowHeadExpander headExp(g, body_);
	ShowBodyExpander bodyExp(*this);
	head_->normalize(g, &headExp);
	if(funcToPred_)
	{
		std::auto_ptr<PredLit> pred = static_cast<ShowHeadLit&>(*head_).toPred(g);
		if(pred.get()) { body_.insert(body_.begin(), pred); }
		else
		{
			std::ostringstream oss;
			head_->print(g, oss);
			throw TypeException("cannot convert term to predicate", StrLoc(g, head_->loc()), oss.str());
		}
	}
	for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
	{
		body_[i].normalize(g, &bodyExp);
	}
}

void Show::visit(PrgVisitor *visitor)
{
	visitor->visit(head_.get(), false);
	foreach(Lit &lit, body_) { visitor->visit(&lit, false); }
}

void Show::print(Storage *sto, std::ostream &out) const
{
	out << "#show ";
	head_->print(sto, out);
	std::vector<const Lit*> body;
	foreach(const Lit &lit, body_) { body.push_back(&lit); }
	std::sort(body.begin(), body.end(), Lit::cmpPos);
	foreach(const Lit *lit, body)
	{
		out << ":";
		lit->print(sto, out);
	}
	out << ".";
}

void Show::append(Lit *lit)
{
	body_.push_back(lit);
}

Show::~Show()
{
}

//////////////////////////////// ShowHeadLit ////////////////////////////////

ShowHeadLit::ShowHeadLit(const Loc &loc, Term *term)
	: Lit(loc)
	, term_(term)
{
}

ShowHeadLit *ShowHeadLit::clone() const
{
	return new ShowHeadLit(*this);
}

void ShowHeadLit::normalize(Grounder *g, Expander *expander)
{
	term_->normalize(this, Term::PtrRef(term_), g, expander, false);
}

bool ShowHeadLit::fact() const
{
	return false;
}

void ShowHeadLit::print(Storage *sto, std::ostream &out) const
{
	term_->print(sto, out);
}

Index *ShowHeadLit::index(Grounder *g, Formula *gr, VarSet &bound)
{
	return new MatchIndex(this);
}

void ShowHeadLit::visit(PrgVisitor *visitor)
{
	term_->visit(visitor, false);
}

void ShowHeadLit::accept(Printer *)
{
	assert(false);
}

bool ShowHeadLit::match(Grounder *)
{
	return true;
}

Val ShowHeadLit::val(Grounder *g)
{
	return term_->val(g);
}

std::auto_ptr<PredLit> ShowHeadLit::toPred(Grounder *g)
{
	std::auto_ptr<PredLit> pred;
	FuncTerm *func = dynamic_cast<FuncTerm*>(term_.get());
	if(func)
	{
		TermPtrVec args = func->args();
		Domain    *dom  = g->domain(func->name(), args.size());
		pred.reset(new PredLit(term_->loc(), dom, args));
	}
	else
	{
		ConstTerm *id = dynamic_cast<ConstTerm*>(term_.get());
		if(id && id->val(g).type == Val::ID)
		{
			TermPtrVec args;
			Domain    *dom  = g->domain(id->val(g).index, args.size());
			pred.reset(new PredLit(term_->loc(), dom, args));
		}
	}
	return pred;
}

//////////////////////////////// ShowHeadExpander ////////////////////////////////

ShowHeadExpander::ShowHeadExpander(Grounder *g, LitPtrVec &body)
	: g_(g)
	, body_(body)
{
}

void ShowHeadExpander::expand(Lit *lit, Expander::Type type)
{
	switch(type)
	{
		case RANGE:
		case RELATION:
		{
			body_.push_back(lit);
			break;
		}
		case POOL:
		{
			LitPtrVec body;
			foreach(Lit &l, body_) { body.push_back(l.clone()); }
			g_->addInternal(new Show(lit->loc(), static_cast<ShowHeadLit*>(lit), body));
			break;
		}
	}
}

//////////////////////////////// ShowBodyExpander ////////////////////////////////

ShowBodyExpander::ShowBodyExpander(Show &d)
	: d_(d)
{
}

void ShowBodyExpander::expand(Lit *lit, Type)
{
	d_.append(lit);
}
