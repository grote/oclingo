// Copyright (c) 2008, Roland Kaminski
//
// This file is part of GrinGo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

#include "multipleargsterm.h"
#include "literal.h"
#include "expandable.h"
#include "value.h"
#include "grounder.h"

using namespace NS_GRINGO;

MultipleArgsTerm::MultipleArgsTerm(Term *a, Term *b) : a_(a), b_(b)
{
}

MultipleArgsTerm::MultipleArgsTerm(const MultipleArgsTerm &r) : a_(r.a_->clone()), b_(r.b_->clone())
{
}

Term* MultipleArgsTerm::clone() const
{
	return new MultipleArgsTerm(*this);
}

void MultipleArgsTerm::print(const GlobalStorage *g, std::ostream &out) const
{
	out << "(" << pp(g, a_) << "; " << pp(g, b_) << ")";
}

void MultipleArgsTerm::getVars(VarSet &vars) const
{
	assert(false);
}

bool MultipleArgsTerm::isComplex()
{
	assert(false);
}

Value MultipleArgsTerm::getConstValue(Grounder *g)
{
	assert(false);
}

Value MultipleArgsTerm::getValue(Grounder *g)
{
	assert(false);
}

namespace
{
	class CloneSentinel : public Term
	{
	public:
		CloneSentinel(Term *a) : a_(a) {}
		Term* clone() const
		{
			Term *a = a_;
			delete this;
			return a;
		}
		~CloneSentinel() { }
		// the rest is unused
		void getVars(VarSet &vars) const { assert(false); }
		bool isComplex() { assert(false); }
		Value getValue(Grounder *g) { assert(false); }
		Value getConstValue(Grounder *g) { assert(false); }
		void preprocess(Literal *l, Term *&p, Grounder *g, Expandable *e) { assert(false); }
		void print(const GlobalStorage *g, std::ostream &out) const { assert(false); }
		bool unify(const GlobalStorage *g, const Value& t, const VarVector& vars, ValueVector& subst) const { assert(false); }
		void addIncParam(Grounder *g, Term *&p, const Value &v) { assert(false); }
	protected:
		Term *a_;
	};
}

void MultipleArgsTerm::addIncParam(Grounder *g, Term *&p, const Value &v) 
{
	a_->addIncParam(g, a_, v);
	b_->addIncParam(g, b_, v);
}

void MultipleArgsTerm::preprocess(Literal *l, Term *&p, Grounder *g, Expandable *e)
{
	p = new CloneSentinel(b_);
	e->appendLiteral(l->clone(), Expandable::MATERM);
	p = a_;
	a_ = 0;
	b_ = 0;
	p->preprocess(l, p, g, e);
	delete this;
}

MultipleArgsTerm::~MultipleArgsTerm()
{
	if(a_)
		delete a_;
	if(b_)
		delete b_;
}

