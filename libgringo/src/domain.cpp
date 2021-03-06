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

#include <gringo/domain.h>
#include <gringo/val.h>
#include <gringo/predindex.h>
#include <gringo/predlit.h>
#include <gringo/term.h>
#include <gringo/grounder.h>
#include <gringo/formula.h>
#include <gringo/instantiator.h>
#include <gringo/grounder.h>
#include <gringo/exceptions.h>

Domain::Domain(uint32_t nameId, uint32_t arity, uint32_t domId)
	: nameId_(nameId)
	, arity_(arity)
	, domId_(domId)
	, vals_(arity)
	, new_(0)
	, external_(false)
	, show(false)
	, hide(false)
{
}

const ValVecSet::Index &Domain::find(const ValVec::const_iterator &v) const
{
	return vals_.find(v);
}

bool Domain::insert(Grounder *, const ValVec::const_iterator &v, bool fact)
{
	ValVecSet::InsertRes res = vals_.insert(v, fact);
	return res.get<1>();
}

bool Domain::extend(Grounder *g, PredIndex *idx, uint32_t offset)
{
	bool modified = false;
	ValVec::const_iterator k = vals_.begin() + arity_ * offset;
	for(uint32_t i = offset; i < size(); i++, k+= arity_)
	{
		modified = idx->extend(g, k) || modified;
	}
	return modified;
}

void Domain::allVals(Grounder *g, const TermPtrVec &terms, VarDomains &varDoms)
{
	VarSet vars;
	foreach(const Term &term, terms) { term.vars(vars); }
	ValVec::const_iterator k = vals_.begin();
	for(uint32_t i = varDoms.offset; i < size(); i++)
	{
		bool unified = true;
		foreach(const Term &term, terms)
		{
			if(!term.unify(g, *k++, 0))
			{
				unified = false;
				break;
			}
		}
		if(unified)
		{
			foreach(uint32_t var, vars) { varDoms.map[var].insert(g->val(var)); }
		}
		foreach(uint32_t var, vars) { g->unbind(var); }
	}
	varDoms.offset = size();
}
