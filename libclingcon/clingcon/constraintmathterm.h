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
#include <gringo/mathterm.h>
#include <clingcon/constraintterm.h>
#include <gringo/mathterm.h>

namespace Clingcon
{
	class ConstraintMathTerm : public ConstraintTerm
	{
	public:
	public:
		ConstraintMathTerm(const Loc &loc, const MathTerm::Func &f, ConstraintTerm *a, ConstraintTerm *b = 0);
		Val val(Grounder *grounder) const;
                void normalize(Lit *parent, const Ref &ref, Grounder *g, const Lit::Expander& expander, bool unify);
		bool unify(Grounder *grounder, const Val &v, int binder) const;
		void vars(VarSet &v) const;
		void visit(PrgVisitor *visitor, bool bind);
                bool isNumber(Grounder* ) const;
                virtual bool match(Grounder* );
		void print(Storage *sto, std::ostream &out) const;
                ConstraintMathTerm *clone() const;
                virtual void initInst(Grounder *g)
                {
                    if(a_.get())a_->initInst(g);
                    if(b_.get())b_->initInst(g);
                }

		virtual void visitVarTerm(PrgVisitor* v)
		{
			if(a_.get())a_->visitVarTerm(v);
			if(b_.get())b_->visitVarTerm(v);
		}
                virtual GroundConstraint* toGroundConstraint(Grounder* );
		~ConstraintMathTerm();
	private:
		MathTerm::Func            f_;
		clone_ptr<ConstraintTerm> a_;
		clone_ptr<ConstraintTerm> b_;
	};

        inline ConstraintMathTerm* new_clone(const ConstraintMathTerm& a)
        {
                return a.clone();
        }
}
