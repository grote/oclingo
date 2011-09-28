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
#include <gringo/gringo.h>
#include <gringo/locateable.h>
#include <gringo/term.h>
#include <clingcon/groundconstraint.h>
#include <clingcon/cspprinter.h>

namespace Clingcon
{
	class ConstraintTerm : public Locateable
	{
	public:
		typedef std::pair<ConstraintTerm*, ConstraintTermPtrVec*> Split;
		class Ref
		{
		public:
			virtual void reset(ConstraintTerm *a) const = 0;
			virtual void replace(ConstraintTerm *a) const = 0;
			virtual ~Ref() { }
		};
		class VecRef : public Ref
		{
		public:
			VecRef(ConstraintTermPtrVec &vec, ConstraintTermPtrVec::iterator pos) : vec_(vec), pos_(pos) { }
			void reset(ConstraintTerm *a) const { vec_.replace(pos_, a); }
			void replace(ConstraintTerm *a) const { vec_.replace(pos_, a).release(); }
		private:
			ConstraintTermPtrVec          &vec_;
			ConstraintTermPtrVec::iterator pos_;
		};
		class PtrRef : public Ref
		{
		public:
			PtrRef(clone_ptr<ConstraintTerm> &ptr) : ptr_(ptr) { }
			void reset(ConstraintTerm *a) const { ptr_.reset(a); }
			void replace(ConstraintTerm *a) const { ptr_.release(); ptr_.reset(a); }
		private:
			clone_ptr<ConstraintTerm> &ptr_;
		};

	public:
		ConstraintTerm(const Loc &loc) : Locateable(loc) { }
                virtual void initInst(Grounder *g) {}
		virtual Val val(Grounder *grounder) const = 0;
		virtual Split split() { return Split(0, 0); }
                virtual void normalize(Lit *parent, const Ref &ref, Grounder *g, const Lit::Expander& expander, bool unify) = 0;
		virtual bool unify(Grounder *grounder, const Val &v, int binder) const = 0;
                virtual bool match(Grounder* g) = 0;
		virtual void vars(VarSet &v) const = 0;
		virtual void visit(PrgVisitor *visitor, bool bind) = 0;
                virtual bool isNumber(Grounder* ) const = 0;
		virtual void print(Storage *sto, std::ostream &out) const = 0;
		virtual void visitVarTerm(PrgVisitor* v) = 0;
                virtual GroundConstraint* toGroundConstraint(Grounder* ) = 0;
		
		virtual ConstraintTerm *clone() const = 0;
                virtual ~ConstraintTerm();
	};

	inline ConstraintTerm* new_clone(const ConstraintTerm& a)
	{
		return a.clone();
	}	

}//namespace Clingcon
