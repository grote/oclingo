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
#include <gringo/luaterm.h>

namespace Clingcon
{
	class ConstraintLuaTerm : public ConstraintTerm
	{
	public:
		ConstraintLuaTerm(const Loc &loc, uint32_t name, ConstraintTermPtrVec &args);
                void normalize(Lit *parent, const Ref &ref, Grounder *g, const Lit::Expander& expander, bool unify);
		ConstraintAbsTerm::Ref* abstract(ConstraintSubstitution& subst) const;
		void print(Storage *sto, std::ostream &out) const;
                ConstraintLuaTerm *clone() const;
		uint32_t name() const;
		Val val(Grounder *grounder) const;
		bool constant() const;
                virtual bool match(Grounder* ){ return true; }
		bool unify(Grounder *grounder, const Val &v, int binder) const;
		void vars(VarSet &v) const;
		void visit(PrgVisitor *visitor, bool bind);
		LuaTerm* toTerm() const
		{
			TermPtrVec vec;
			for(ConstraintTermPtrVec::const_iterator i = args_.begin(); i != args_.end(); ++i)
				vec.push_back(i->toTerm());
			return new LuaTerm(loc(), name_, vec);
		}
		virtual void visitVarTerm(PrgVisitor* v)
		{
			for (ConstraintTermPtrVec::iterator i = args_.begin(); i != args_.end(); ++i)
				i->visitVarTerm(v);
		}
                virtual GroundConstraint* toGroundConstraint(Grounder* );
		~ConstraintLuaTerm();
	private:
		uint32_t                name_;
		ConstraintTermPtrVec    args_;
	};

	inline uint32_t ConstraintLuaTerm::name() const { return name_; }

	inline Val ConstraintLuaTerm::val(Grounder *) const                      { assert(false); return Val::fail(); }
	inline bool ConstraintLuaTerm::constant() const                          { assert(false); return false; }
	inline bool ConstraintLuaTerm::unify(Grounder *, const Val &, int) const { assert(false); return false; }
	inline void ConstraintLuaTerm::vars(VarSet &) const                      { assert(false); }
	inline void ConstraintLuaTerm::visit(PrgVisitor *, bool)                 { assert(false); }

        inline ConstraintLuaTerm* new_clone(const ConstraintLuaTerm& a)
        {
                return a.clone();
        }
}
