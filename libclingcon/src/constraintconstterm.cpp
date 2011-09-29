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

#include <clingcon/constraintconstterm.h>
#include <gringo/grounder.h>
#include <gringo/litdep.h>

namespace Clingcon
{
        ConstraintConstTerm::ConstraintConstTerm(const Loc &loc, Term* t) :
                ConstraintTerm(loc), t_(t)
	{
	}

        ConstraintConstTerm::~ConstraintConstTerm(){}

	Val ConstraintConstTerm::val(Grounder *grounder) const
	{
            return t_->val(grounder);
	}

	bool ConstraintConstTerm::unify(Grounder *grounder, const Val &v, int binder) const
	{
            return t_->unify(grounder,v,binder);
            //return t_->val(grounder) == v;
	}

	void ConstraintConstTerm::vars(VarSet &vars) const
	{
            t_->vars(vars);
	}

	void ConstraintConstTerm::visit(PrgVisitor *v, bool bind)
	{
            t_->visit(v,bind);
	}

	void ConstraintConstTerm::print(Storage *sto, std::ostream &out) const
	{
                t_->print(sto, out);
	}

	ConstraintConstTerm *ConstraintConstTerm::clone() const
	{
            //return new ConstraintConstTerm(*this);
            return new ConstraintConstTerm(loc(),t_->clone());
	}

        GroundConstraint* ConstraintConstTerm::toGroundConstraint(Grounder* g)
	{
            return new GroundConstraint(g,t_->val(g));
	}
}
