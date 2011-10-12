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
#include <clingcon/constraintterm.h>
#include <clingcon/constraintvarcond.h>

namespace Clingcon
{
        class ConstraintAggrTerm : public ConstraintTerm, public Container
	{
        public:
            enum Type
            {
                SUM,
                MIN,
                MAX
            };
	public:
                ConstraintAggrTerm(const Loc &loc, Type t, Clingcon::ConstraintVarCondPtrVec* cond);
		Val val(Grounder *grounder) const;
                void normalize(Lit *parent, const Ref &ref, Grounder *g, const Lit::Expander& expander, bool unify);
		bool unify(Grounder *grounder, const Val &v, int binder) const;
		void vars(VarSet &v) const;
		void visit(PrgVisitor *visitor, bool bind);
                bool isNumber(Grounder* ) const;
		void print(Storage *sto, std::ostream &out) const;
                ConstraintAggrTerm *clone() const;
                virtual bool match(Grounder* );
                virtual void initInst(Grounder *g)
                {
                    for (ConstraintVarCondPtrVec::iterator i = cond_->begin(); i != cond_->end(); ++i)
                    {
                        i->initInst(g);
                    }
                }

                virtual void add(ConstraintVarCond* add)
                {
                    cond_->push_back(add);
                }


		virtual void visitVarTerm(PrgVisitor* v)
		{
                    for (ConstraintVarCondPtrVec::iterator i = cond_->begin(); i != cond_->end(); ++i)
                    {
                        v->visit(&(*i), false);
                       // i->visit(v);
                    }
                        //if(a_.get())a_->visitVarTerm(v);
                        //if(b_.get())b_->visitVarTerm(v);
		}
                virtual GroundConstraint* toGroundConstraint(Grounder* );

                ~ConstraintAggrTerm();
	private:
                GroundConstraint* toGroundConstraint(Grounder* g, GroundedConstraintVarLitVec& vec, int i);
                clone_ptr<Clingcon::ConstraintVarCondPtrVec> cond_;
                Type t_;

	};

        inline ConstraintAggrTerm* new_clone(const ConstraintAggrTerm& a)
        {
                return a.clone();
        }
}
