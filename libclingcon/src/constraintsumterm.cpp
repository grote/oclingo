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

#include <clingcon/constraintsumterm.h>
#include <clingcon/constraintconstterm.h>
#include <clingcon/constraintvarterm.h>
#include <clingcon/globalconstraint.h>
#include <clingcon/exception.h>
#include <gringo/grounder.h>
#include <gringo/varterm.h>
#include <gringo/constterm.h>
#include <gringo/rellit.h>
#include <gringo/prgvisitor.h>
#include <gringo/exceptions.h>
#include <gringo/litdep.h>


namespace Clingcon
{
        ConstraintSumTerm::ConstraintSumTerm(const Loc &loc,Clingcon::ConstraintVarCondPtrVec* cond) :
            ConstraintTerm(loc), cond_(cond)
	{
            for(ConstraintVarCondPtrVec::iterator i = cond_->begin(); i != cond_->end(); ++i)
                i->parent_ = this;
	}

        Val ConstraintSumTerm::val(Grounder *) const
	{
		assert(false);
                return Val::fail();
	}

        bool ConstraintSumTerm::unify(Grounder *grounder, const Val &v, int binder) const
	{
                #pragma message("ask ROLAND")
		return false;
	}

        void ConstraintSumTerm::vars(VarSet &v) const
	{
            for (ConstraintVarCondPtrVec::const_iterator i = cond_->begin(); i != cond_->end(); ++i)
            {
                (*(i->head()))->vars(v);
                //Hier überall auch den body betrachten?
               // (*(i->lit())).vars(v);
            }
	}

        void ConstraintSumTerm::visit(PrgVisitor *visitor, bool bind)
	{
            for (ConstraintVarCondPtrVec::iterator i = cond_->begin(); i != cond_->end(); ++i)
            {
                i->visit(visitor);
            }

                //a_->visit(visitor,bind);
                //b_->visit(visitor,bind);
		//(void)bind;
		//visitor->visit(a_.get(), false);
		//if(b_.get()) visitor->visit(b_.get(), false);
	}

        bool ConstraintSumTerm::constant() const
	{
            return false;
                //return a_->constant() && (!b_.get() || b_->constant());
	}

        void ConstraintSumTerm::print(Storage *sto, std::ostream &out) const
	{
            out << "$sum{";
            for (size_t i = 0; i < cond_->size(); ++i)
            {
                (*cond_)[i].print(sto,out);
                if (i+1<cond_->size())
                    out << ",";
            }
             out << "}";
	}

        bool ConstraintSumTerm::match(Grounder* g)
        {
            for(ConstraintVarCondPtrVec::iterator i = cond_->begin(); i != cond_->end(); ++i)
                i->ground(g);
            return true;
        }

        void ConstraintSumTerm::normalize(Lit *, const Ref &, Grounder* g, const Lit::Expander& , bool )
	{
            for (size_t i = 0; i < cond_->size(); ++i)
            {
                (*cond_)[i].normalize(g);
//		if(a_.get()) a_->normalize(parent, PtrRef(a_), g, expander, false);
//		if(b_.get()) b_->normalize(parent, PtrRef(b_), g, expander, false);
//		if((!a_.get() || a_->constant()) && (!b_.get() || b_->constant()))
//		{
//			ref.reset(new ConstraintConstTerm(loc(), val(g)));
//		}
//		else if(unify)
//		{
//			uint32_t var = g->createVar();
//			expander->expand(new RelLit(a_->loc(), RelLit::ASSIGN, new VarTerm(a_->loc(), var), this->toTerm()), Expander::RELATION);
//			ref.reset(new ConstraintVarTerm(a_->loc(), var));
                }
	}

        ConstraintAbsTerm::Ref* ConstraintSumTerm::abstract(ConstraintSubstitution& subst) const
	{
		return subst.anyVar();
	}

        ConstraintSumTerm *ConstraintSumTerm::clone() const
	{
                return new ConstraintSumTerm(*this);
	}

        GroundConstraint* ConstraintSumTerm::toGroundConstraint(Grounder* g, GroundedConstraintVarLitVec& vec, int i)
        {
            if (i+1 < vec.size())
            {
                return new GroundConstraint(g,GroundConstraint::PLUS, vec[i].vt_.release(), toGroundConstraint(g,vec,i+1));

//                return new GroundConstraint(g,GroundConstraint::PLUS, (*(*cond_)[i].head())->toGroundConstraint(g), toGroundConstraint(g,i+1));
            }
            else
                return vec[i].vt_.release();
//                return (*(*cond_)[i].head())->toGroundConstraint(g);


        }


        GroundConstraint* ConstraintSumTerm::toGroundConstraint(Grounder* g)
	{
            GroundedConstraintVarLitVec vec;

            for (size_t i = 0; i < cond_->size(); ++i)
                (*cond_)[i].getVariables(vec);
            if (vec.size())
                return toGroundConstraint(g,vec,0);
            else
                return new GroundConstraint(g,Val::number(0));
	}

        ConstraintSumTerm::~ConstraintSumTerm()
	{
	}
}
