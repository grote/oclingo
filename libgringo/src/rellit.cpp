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

#include <gringo/rellit.h>
#include <gringo/term.h>
#include <gringo/index.h>
#include <gringo/grounder.h>
#include <gringo/formula.h>
#include <gringo/instantiator.h>
#include <gringo/litdep.h>

RelLit::RelLit(const Loc &loc, Type t, Term *a, Term *b)
	: Lit(loc)
	, t_(t)
	, a_(a)
	, b_(b)
{
}

bool RelLit::match(Grounder *g)
{
	if(head()) { return true; }
	else
	{
		switch(t_)
		{
			case RelLit::GREATER: return a_->val(g).compare(b_->val(g), g) > 0;
			case RelLit::LOWER:   return a_->val(g).compare(b_->val(g), g) < 0;
			case RelLit::EQUAL:   return a_->val(g) == b_->val(g);
			case RelLit::GTHAN:   return a_->val(g).compare(b_->val(g), g) >= 0;
			case RelLit::LTHAN:   return a_->val(g).compare(b_->val(g), g) <= 0;
			case RelLit::INEQUAL: return a_->val(g) != b_->val(g);
			case RelLit::ASSIGN:  return a_->val(g).compare(b_->val(g), g) == 0;
		}
		assert(false);
		return false;
	}
}

namespace
{
	class AssignIndex : public StaticIndex
	{
	public:
		AssignIndex(Term *a, Term *b, const VarVec &bind);
		bool first(Grounder *grounder, int binder);
	private:
		Term  *a_;
		Term  *b_;
		VarVec bind_;
	};

	AssignIndex::AssignIndex(Term *a, Term *b, const VarVec &bind)
		: a_(a)
		, b_(b)
		, bind_(bind)
	{
	}

	bool AssignIndex::first(Grounder *grounder, int binder)
	{
		foreach(uint32_t var, bind_) grounder->unbind(var);
		return a_->unify(grounder, b_->val(grounder), binder);
	}
}

Index *RelLit::index(Grounder *g, Formula *, VarSet &bound)
{
	(void)g;
	if(t_ == ASSIGN)
	{
		VarSet vars;
		VarVec bind;
		a_->vars(vars);
		std::set_difference(vars.begin(), vars.end(), bound.begin(), bound.end(), std::back_insert_iterator<VarVec>(bind));
		if(bind.size() > 0)
		{
			AssignIndex *p = new AssignIndex(a_.get(), b_.get(), bind);
			bound.insert(bind.begin(), bind.end());
			return p;
		}
	}
	return new MatchIndex(this);
}

void RelLit::accept(Printer *v)
{
	(void)v;
}

void RelLit::visit(PrgVisitor *v)
{
	if(a_.get()) v->visit(a_.get(), t_ == ASSIGN);
	if(b_.get()) v->visit(b_.get(), false);
}

void RelLit::print(Storage *sto, std::ostream &out) const
{
	a_->print(sto, out);
	switch(t_)
	{
		case RelLit::GREATER: out << ">"; break;
		case RelLit::LOWER:   out << "<"; break;
		case RelLit::EQUAL:   out << "=="; break;
		case RelLit::GTHAN:   out << ">="; break;
		case RelLit::LTHAN:   out << "=<"; break;
		case RelLit::INEQUAL: out << "!="; break;
		case RelLit::ASSIGN:  out << ":="; break;
	}
	b_->print(sto, out);
}

void RelLit::normalize(Grounder *g, const Expander &e)
{
	if(a_.get()) { a_->normalize(this, Term::PtrRef(a_), g, e, t_ == ASSIGN); }
	if(b_.get()) { b_->normalize(this, Term::PtrRef(b_), g, e, false); }
}

Lit *RelLit::clone() const
{
	return new RelLit(*this);
}

RelLit::~RelLit()
{
}

