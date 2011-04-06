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

#include <gringo/prgvisitor.h>
#include <gringo/varterm.h>
#include <gringo/aggrlit.h>

//////////////////////////////////////// GlobalsCollector ////////////////////////////////////////

GlobalsCollector::GlobalsCollector(VarSet &globals, uint32_t level)
	: globals_(globals)
	, level_(level)
{
}

void GlobalsCollector::visit(VarTerm *var, bool)
{
	if(var->level() <= level_) { globals_.insert(var->index()); }
}

void GlobalsCollector::visit(Term* term, bool bind)
{
	term->visit(this, bind);
}

void GlobalsCollector::visit(Lit *lit, bool)
{
	lit->visit(this);
}

void GlobalsCollector::visit(Formula *grd, bool)
{
	grd->visit(this);
}

void GlobalsCollector::collect(AggrLit &lit, VarSet &globals, uint32_t level)
{
	GlobalsCollector gc(globals, level);
	foreach(AggrCond &cond, lit.conds()) { cond.visit(&gc); }
	if(!lit.assign())
	{
		if(lit.lower()) { lit.lower()->visit(&gc, false); }
		if(lit.upper()) { lit.upper()->visit(&gc, false); }
	}
}

void GlobalsCollector::collect(Lit &lit, VarSet &globals, uint32_t level)
{
	GlobalsCollector gc(globals, level);
	gc.visit(&lit, false);
}

void GlobalsCollector::collect(Lit &lit, VarVec &globals, uint32_t level)
{
	VarSet set;
	collect(lit, set, level);
	globals.assign(set.begin(), set.end());
}

void GlobalsCollector::collect(Formula &f, VarSet &globals, uint32_t level)
{
	GlobalsCollector gc(globals, level);
	gc.visit(&f, false);
}
