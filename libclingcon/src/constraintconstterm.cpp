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
namespace Clingcon
{
	ConstraintConstTerm::ConstraintConstTerm(const Loc &loc, const Val &v) :
		ConstraintTerm(loc), val_(v)
	{
	}

	Val ConstraintConstTerm::val(Grounder *grounder) const
	{
		(void)grounder;
		return val_;
	}

	bool ConstraintConstTerm::unify(Grounder *grounder, const Val &v, int binder) const
	{
		(void)grounder;
		(void)binder;
		return val_ == v;
	}

	void ConstraintConstTerm::vars(VarSet &vars) const
	{
		(void)vars;
	}

	void ConstraintConstTerm::visit(PrgVisitor *v, bool bind)
	{
		(void)v;
		(void)bind;
	}

	void ConstraintConstTerm::print(Storage *sto, std::ostream &out) const
	{
		val_.print(sto, out);
	}

	ConstraintConstTerm *ConstraintConstTerm::clone() const
	{
		return new ConstraintConstTerm(*this);
	}

	ConstraintAbsTerm::Ref* ConstraintConstTerm::abstract(ConstraintSubstitution& subst) const
	{
		return subst.addTerm(new ConstraintAbsTerm(val_));
	}

        GroundConstraint* ConstraintConstTerm::toGroundConstraint(Grounder* g)
	{
                return new GroundConstraint(g,val_);
	}
}
