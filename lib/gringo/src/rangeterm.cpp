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

#include "rangeterm.h"
#include "value.h"
#include "grounder.h"
#include "expandable.h"
#include "variable.h"
#include "rangeliteral.h"

using namespace NS_GRINGO;

RangeTerm::RangeTerm(Term *lower, Term *upper) : Term(), lower_(lower), upper_(upper)
{
}

void RangeTerm::getVars(VarSet &vars) const
{
	// rangeterms are eliminated while preprocessing
	assert(false);
}

bool RangeTerm::isComplex()
{
	// rangeterms are eliminated while preprocessing
	assert(false);
}

void RangeTerm::preprocess(Literal *l, Term *&p, Grounder *g, Expandable *e)
{
	int var = g->createUniqueVar();
	e->appendLiteral(new RangeLiteral(new Variable(g, var), lower_, upper_), Expandable::RANGETERM);
	p = new Variable(g, var);
	lower_ = 0;
	upper_ = 0;
	delete this;
}

void RangeTerm::addIncParam(Grounder *g, Term *&p, const Value &v)
{
	lower_->addIncParam(g, lower_, v);
	upper_->addIncParam(g, upper_, v);
}

void RangeTerm::print(const GlobalStorage *g, std::ostream &out) const
{
	out << pp(g, lower_) << ".." << pp(g, upper_);
}

Term *RangeTerm::getLower()
{
	return lower_;
}

Term *RangeTerm::getUpper()
{
	return upper_;
}

Value RangeTerm::getConstValue(Grounder *g)
{
	assert(false);
}

Value RangeTerm::getValue(Grounder *g)
{
	assert(false);
}

RangeTerm::RangeTerm(const RangeTerm &r) : lower_(r.lower_->clone()), upper_(r.upper_->clone())
{
}

Term* RangeTerm::clone() const
{
	return new RangeTerm(*this);
}

RangeTerm::~RangeTerm()
{
	if(lower_)
		delete lower_;
	if(upper_)
		delete upper_;
}

