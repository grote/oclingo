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

#include "gringo/junctionlit.h"

///////////////////////////// JunctionCond /////////////////////////////

JunctionCond::JunctionCond(const Loc &loc, Lit *head, LitPtrVec &body)
	: head_(head)
	, body_(body)
{
}

JunctionCond::~JunctionCond()
{
}

///////////////////////////// JunctionLit /////////////////////////////

JunctionLit::JunctionLit(const Loc &loc, JunctionCondVec &conds)
	: Lit(loc)
	, conds_(conds)
{
}

void JunctionLit::normalize(Grounder *g, Expander *expander)
{
}

bool JunctionLit::match(Grounder *grounder)
{
}

bool JunctionLit::fact() const
{
}

void JunctionLit::print(Storage *sto, std::ostream &out) const
{
}

void JunctionLit::index(Grounder *g, Groundable *gr, VarSet &bound)
{
}

Lit::Score JunctionLit::score(Grounder *g, VarSet &bound)
{
}

void JunctionLit::visit(PrgVisitor *visitor)
{
}

void JunctionLit::accept(::Printer *v)
{
}

Lit *JunctionLit::clone() const
{
}

JunctionLit::~JunctionLit()
{
}
