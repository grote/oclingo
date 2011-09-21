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
                ConstraintFuncTerm(const Loc &loc, uint32_t name, TermPtrVec &args);
		Val val(Grounder *grounder) const;
                void normalize(Lit *parent, const Ref &ref, Grounder *g, const Lit::Expander& expander, bool unify);
		ConstraintAbsTerm::Ref* abstract(ConstraintSubstitution& subst) const;
		bool unify(Grounder *grounder, const Val &v, int binder) const;
		void vars(VarSet &v) const;
                virtual bool match(Grounder* ){ return true; }
		void visit(PrgVisitor *visitor, bool bind);
		bool constant() const;
		void print(Storage *sto, std::ostream &out) const;
                ConstraintFuncTerm *clone() const;
		FuncTerm* toTerm() const
		{
                        TermPtrVec vec = args_;
                        //for(TermPtrVec::const_iterator i = args_.begin(); i != args_.end(); ++i)
                        //	vec.push_back(i->toTerm());
                        return new FuncTerm(loc(), name_, vec);
		}
		virtual void visitVarTerm(PrgVisitor* v)
		{
                        for(TermPtrVec::iterator i = args_.begin(); i != args_.end(); ++i)
                                //i->visitVarTerm(v);
                            v->visit(&(*i),false);
		}
                virtual GroundConstraint* toGroundConstraint(Grounder* );
		~ConstraintFuncTerm();
	private:
		uint32_t                name_;
                TermPtrVec              args_;
                mutable clone_ptr<ConstraintFuncTerm> clone_;
	};

        inline ConstraintFuncTerm* new_clone(const ConstraintFuncTerm& a)
        {
                return a.clone();
        }
}
