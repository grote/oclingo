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

#include <clingcon/constraintvarterm.h>
#include <gringo/litdep.h>
#include <gringo/grounder.h>
#include <gringo/storage.h>

namespace Clingcon
{
	ConstraintVarTerm::ConstraintVarTerm(const Loc &loc)
		: ConstraintTerm(loc)
		, var_(loc)
	{
	}

	ConstraintVarTerm::ConstraintVarTerm(const Loc &loc, uint32_t nameId)
		: ConstraintTerm(loc)
                , var_(loc,nameId)
	{
	}

	bool ConstraintVarTerm::anonymous() const
	{
		return var_.anonymous();
	}

	Val ConstraintVarTerm::val(Grounder *grounder) const
	{
		return var_.val(grounder);
	}

	bool ConstraintVarTerm::unify(Grounder *grounder, const Val &v, int binder) const
	{
		return var_.unify(grounder,v,binder);
	}

        ConstraintAbsTerm::Ref* ConstraintVarTerm::abstract(ConstraintSubstitution& ) const
	{
		return 0;
		//return var_.abstract(subst);
	}

	void ConstraintVarTerm::visit(PrgVisitor *v, bool bind)
	{
		var_.visit(v,bind);
	}

	void ConstraintVarTerm::vars(VarSet &vars) const
	{
		var_.vars(vars);
	}

	void ConstraintVarTerm::print(Storage *sto, std::ostream &out) const
	{
                var_.print(sto,out);
	}

        GroundConstraint* ConstraintVarTerm::toGroundConstraint(Grounder* g)
	{
                return new GroundConstraint(g,val(g));
	}

	ConstraintTerm *ConstraintVarTerm::clone() const
	{
		return new ConstraintVarTerm(*this);
	}
}
