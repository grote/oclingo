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

#include <clingcon/clingcon.h>
#include <clingcon/constraintterm.h>
#include <gringo/constterm.h>
#include <gringo/grounder.h>

namespace Clingcon
{
	class ConstraintConstTerm : public ConstraintTerm
	{
	public:
                ConstraintConstTerm(const Loc &loc, Term* t);
                ~ConstraintConstTerm();
		Val val(Grounder *grounder) const;
                void normalize(Lit *parent, const Ref &, Grounder *grounder, const Lit::Expander& expander, bool unify)
                {
                    t_->normalize(parent,Term::PtrRef(t_),grounder,expander,unify);
                }
		bool unify(Grounder *grounder, const Val &v, int binder) const;
		void vars(VarSet &vars) const;
		void visit(PrgVisitor *visitor, bool bind);
                virtual bool match(Grounder* g){ return true; }
		void print(Storage *sto, std::ostream &out) const;
                bool isNumber(Grounder* g) const { return t_->val(g).type==Val::NUM; }
		ConstraintConstTerm *clone() const;
                //ConstTerm* toTerm() const {return new ConstTerm(loc(),val_);};
                virtual GroundConstraint* toGroundConstraint(Grounder* );
                virtual void visitVarTerm(PrgVisitor* v) { t_->visit(v,false); }
	private:
                clone_ptr<Term> t_;
	};

        inline ConstraintConstTerm* new_clone(const ConstraintConstTerm& a);
}
