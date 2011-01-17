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

#include <gringo/statement.h>
#include <gringo/varcollector.h>
#include <gringo/lit.h>
#include <gringo/litdep.h>
#include <gringo/varterm.h>
#include <gringo/grounder.h>
#include <gringo/exceptions.h>
#include <gringo/instantiator.h>

namespace
{
	class IndexAdder : private PrgVisitor
	{
	private:
		IndexAdder(Grounder *g);
		virtual void visit(Lit *lit, bool domain);
		virtual void visit(Groundable *grd, bool choice);
	public:
		static void add(Grounder *g, Statement *s);
	private:
		Grounder   *g_;
		Groundable *grd_;
		VarSet      bound_;
		bool        top_;
	};
}

//////////////////////////////// Statement ////////////////////////////////

void Statement::init(Grounder *g)
{
	IndexAdder::add(g, this);
}

Statement::Statement(const Loc &loc)
	: Locateable(loc)
{
}

void Statement::check(Grounder *g)
{
	VarCollector collector(g);
	collector.visit(this, choice());
	uint32_t numVars = collector.collect();
	if(numVars > 0)
	{
		LitDep::Builder builder(numVars);
		builder.visit(this, choice());
		VarTermVec terms;
		if(!builder.check(terms))
		{
			std::ostringstream oss;
			print(g, oss);
			UnsafeVarsException ex(StrLoc(g, loc()), oss.str());
			foreach(VarTerm *term, terms)
			{
				oss.str("");
				term->print(g, oss);
				ex.add(StrLoc(g, term->loc()), oss.str());
			}
			throw ex;
		}
	}
}

//////////////////////////////// IndexAdder ////////////////////////////////

IndexAdder::IndexAdder(Grounder *g)
	: g_(g)
	, grd_(0)
	, top_(true)
{ }

void IndexAdder::visit(Lit *lit, bool)
{
	if(!grd_->litDep()) { lit->index(g_, grd_, bound_); }
	lit->visit(this);
}

void IndexAdder::visit(Groundable *grd, bool)
{
	bool top = top_;
	top_     = false;
	if(!grd->instantiator())
	{
		Groundable *oldGrd(grd_);
		VarSet      oldBound(bound_);
		grd_ = grd;
		grd_->instantiator(new Instantiator(grd_));
		grd_->visit(this);
		if(grd_->litDep()) { grd_->litDep()->order(g_, bound_); }
		grd_->instantiator()->fix();
		grd_ = oldGrd;
		std::swap(bound_, oldBound);
	}
	grd->instantiator()->init(g_, top);
}

void IndexAdder::add(Grounder *g, Statement *s)
{
	IndexAdder adder(g);
	adder.visit(s, false);
}
