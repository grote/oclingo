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

#include <gringo/show.h>
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
	class DisplayHeadExpander : public Expander
	{
	public:
		DisplayHeadExpander(Grounder *g, LitPtrVec &body, Display::Type type);
		void expand(Lit *lit, Type type);
	private:
		Grounder  *g_;
		LitPtrVec &body_;
		Display::Type type_;
	};

	class DisplayBodyExpander : public Expander
	{
	public:
		DisplayBodyExpander(Display &d);
		void expand(Lit *lit, Type);
	private:
		Display &d_;
	};

}

//////////////////////////////// Display ////////////////////////////////

Display::Display(const Loc &loc, Term *term, LitPtrVec &body, Type type)
	: SimpleStatement(loc)
	, head_(new DisplayHeadLit(loc, term))
	, body_(body.release())
	, type_(type)
{
}

Display::Display(const Loc &loc, DisplayHeadLit *lit, LitPtrVec &body, Type type)
	: SimpleStatement(loc)
	, head_(lit)
	, body_(body.release())
	, type_(type)
{
}


bool Display::grounded(Grounder *g)
{
	Printer *printer = g->output()->printer<Printer>();
	printer->begin(static_cast<DisplayHeadLit&>(*head_).val(g), type_);
	foreach(Lit &lit, body_)
	{
		lit.grounded(g);
		if(!lit.fact() || lit.forcePrint()) { lit.accept(printer); }
	}
	printer->end();
	return true;
}


void Display::normalize(Grounder *g)
{
	DisplayHeadExpander headExp(g, body_, type_);
	DisplayBodyExpander bodyExp(*this);
	head_->normalize(g, &headExp);
	if(type_ != SHOWTERM)
	{
		std::auto_ptr<PredLit> pred = static_cast<DisplayHeadLit&>(*head_).toPred(g);
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

void Display::visit(PrgVisitor *visitor)
{
	visitor->visit(head_.get(), false);
	foreach(Lit &lit, body_) { visitor->visit(&lit, false); }
}

void Display::print(Storage *sto, std::ostream &out) const
{
	out << (type_ == HIDEPRED ? "#hide " : "#show ");
	if(type_ == SHOWTERM)
	{
		head_->print(sto, out);
		out << ":";
	}
	std::vector<const Lit*> body;
	foreach(const Lit &lit, body_) { body.push_back(&lit); }
	std::sort(body.begin(), body.end(), Lit::cmpPos);
	bool comma = false;
	foreach(const Lit *lit, body)
	{
		if(comma) { out << ":"; }
		else      { comma = true; }
		lit->print(sto, out);
	}
	out << (type_ == SHOWTERM ? ":" : "") << ".";
}

void Display::append(Lit *lit)
{
	body_.push_back(lit);
}

Display::~Display()
{
}

//////////////////////////////// DisplayHeadLit ////////////////////////////////

DisplayHeadLit::DisplayHeadLit(const Loc &loc, Term *term)
	: Lit(loc)
	, term_(term)
{
}

DisplayHeadLit *DisplayHeadLit::clone() const
{
	return new DisplayHeadLit(*this);
}

void DisplayHeadLit::normalize(Grounder *g, Expander *expander)
{
	term_->normalize(this, Term::PtrRef(term_), g, expander, false);
}

bool DisplayHeadLit::fact() const
{
	return false;
}

void DisplayHeadLit::print(Storage *sto, std::ostream &out) const
{
	term_->print(sto, out);
}

Index *DisplayHeadLit::index(Grounder *g, Formula *gr, VarSet &bound)
{
	return new MatchIndex(this);
}

void DisplayHeadLit::visit(PrgVisitor *visitor)
{
	term_->visit(visitor, false);
}

void DisplayHeadLit::accept(Printer *)
{
	assert(false);
}

bool DisplayHeadLit::match(Grounder *)
{
	return true;
}

Val DisplayHeadLit::val(Grounder *g)
{
	return term_->val(g);
}

std::auto_ptr<PredLit> DisplayHeadLit::toPred(Grounder *g)
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

//////////////////////////////// DisplayHeadExpander ////////////////////////////////

DisplayHeadExpander::DisplayHeadExpander(Grounder *g, LitPtrVec &body, Display::Type type)
	: g_(g)
	, body_(body)
	, type_(type)
{
}

void DisplayHeadExpander::expand(Lit *lit, Expander::Type type)
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
			g_->addInternal(new Display(lit->loc(), static_cast<DisplayHeadLit*>(lit), body, type_));
			break;
		}
	}
}

//////////////////////////////// DisplayBodyExpander ////////////////////////////////

DisplayBodyExpander::DisplayBodyExpander(Display &d)
	: d_(d)
{
}

void DisplayBodyExpander::expand(Lit *lit, Type)
{
	d_.append(lit);
}
