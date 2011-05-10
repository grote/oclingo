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

#include <clingcon/constraintluaterm.h>
#include <gringo/grounder.h>
#include <gringo/func.h>
#include <gringo/lualit.h>
#include <gringo/varterm.h>
#include <clingcon/constraintvarterm.h>

namespace Clingcon
{
	ConstraintLuaTerm::ConstraintLuaTerm(const Loc &loc, uint32_t name, ConstraintTermPtrVec &args)
		: ConstraintTerm(loc)
		, name_(name)
		, args_(args)
	{
	}

	void ConstraintLuaTerm::normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify)
	{
		(void)unify;
		for(ConstraintTermPtrVec::iterator it = args_.begin(); it != args_.end(); it++)
		{
			it->normalize(parent, ConstraintTerm::VecRef(args_, it), g, expander, false);
		}
		uint32_t var = g->createVar();
		TermPtrVec vec;
		for(ConstraintTermPtrVec::iterator i = args_.begin(); i != args_.end(); ++i)
			vec.push_back(i->toTerm());

		expander->expand(new LuaLit(loc(), new VarTerm(loc(), var), vec, name_, g->luaIndex(loc(), name_)), Expander::RANGE);
		ref.reset(new ConstraintVarTerm(loc(), var));
	}

	ConstraintAbsTerm::Ref* ConstraintLuaTerm::abstract(ConstraintSubstitution& subst) const
	{
		return 0;
		//return subst.anyVar();
	}

	void ConstraintLuaTerm::print(Storage *sto, std::ostream &out) const
	{
		out << "@" << sto->string(name_);
		out << "(";
		bool comma = false;
		foreach(const ConstraintTerm &t, args_)
		{
			if(comma) out << ",";
			else comma = true;
			t.print(sto, out);
		}
		out << ")";
	}

	ConstraintTerm *ConstraintLuaTerm::clone() const
	{
		return new ConstraintLuaTerm(*this);
	}
	
        GroundConstraint* ConstraintLuaTerm::toGroundConstraint(Grounder* )
	{
		assert(false);
                return new GroundConstraint();
	}

	ConstraintLuaTerm::~ConstraintLuaTerm()
	{
	}
}
