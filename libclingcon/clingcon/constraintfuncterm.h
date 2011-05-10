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

#pragma once

#include <gringo/gringo.h>
#include <clingcon/clingcon.h>
#include <clingcon/constraintterm.h>
#include <gringo/functerm.h>

namespace Clingcon
{
	class ConstraintFuncTerm : public ConstraintTerm
	{
	public:
		ConstraintFuncTerm(const Loc &loc, uint32_t name, ConstraintTermPtrVec &args);
		Val val(Grounder *grounder) const;
		void normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify);
		ConstraintAbsTerm::Ref* abstract(ConstraintSubstitution& subst) const;
		bool unify(Grounder *grounder, const Val &v, int binder) const;
		void vars(VarSet &v) const;
		void visit(PrgVisitor *visitor, bool bind);
		bool constant() const;
		void print(Storage *sto, std::ostream &out) const;
		ConstraintTerm *clone() const;
		FuncTerm* toTerm() const
		{
			TermPtrVec vec;
			for(ConstraintTermPtrVec::const_iterator i = args_.begin(); i != args_.end(); ++i)
				vec.push_back(i->toTerm());
			return new FuncTerm(loc(), name_, vec);
		}
		virtual void visitVarTerm(PrgVisitor* v)
		{
			for(ConstraintTermPtrVec::iterator i = args_.begin(); i != args_.end(); ++i)
				i->visitVarTerm(v);
		}
                virtual GroundConstraint* toGroundConstraint(Grounder* );
		~ConstraintFuncTerm();
	private:
		uint32_t                name_;
		ConstraintTermPtrVec              args_;
		mutable clone_ptr<ConstraintTerm> clone_;
	};
}
