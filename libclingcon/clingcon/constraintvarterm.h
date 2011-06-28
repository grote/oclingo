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
#include <gringo/varterm.h>
#include <gringo/prgvisitor.h>

namespace Clingcon
{
	class ConstraintVarTerm : public ConstraintTerm
	{
	public:
		ConstraintVarTerm(const Loc &loc);
		ConstraintVarTerm(const Loc &loc, uint32_t nameId);
		bool anonymous() const;
		void nameId(uint32_t nameId) { var_.nameId(nameId); }
		uint32_t nameId() const { return var_.nameId(); }
		uint32_t index() const { return var_.index(); }
		uint32_t level() const { return var_.level(); }
                void normalize(Lit *, const Ref &, Grounder *, const Lit::Expander& , bool ) { /*var_.normalize(parent,ref,grounder,expander);*/}
		ConstraintAbsTerm::Ref* abstract(ConstraintSubstitution& subst) const;
                void index(uint32_t index, uint32_t level) { var_.index(index,level); }
		Val val(Grounder *grounder) const;
		bool constant() const { return false; }
		bool unify(Grounder *grounder, const Val &v, int binder) const;
		void vars(VarSet &vars) const;
                virtual bool match(Grounder* ){ return true; }
		void visit(PrgVisitor *visitor, bool bind);
		void print(Storage *sto, std::ostream &out) const;
		VarTerm* toTerm() const
		{
			return new VarTerm(var_);
		}
		virtual void visitVarTerm(PrgVisitor* v)
		{
                        v->visit(&var_, false);
		}
                virtual GroundConstraint* toGroundConstraint(Grounder* );
		ConstraintTerm *clone() const;
	private:
		VarTerm var_;
	};

	inline ConstraintVarTerm* new_clone(const ConstraintVarTerm& a)
	{
		return static_cast<ConstraintVarTerm*>(a.clone());
	}
}
