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
	class ConstraintAbsTerm
	{
		enum Type { ANY = Val::SUP + 1 };
	public:
		class Ref
		{
		public:
			Ref(ConstraintAbsTerm *term) 
				: term_(term)
			{
			}
			void replace(Ref &ref)
			{
				term_ = ref.term_;
			}
			ConstraintAbsTerm *operator->() const
			{
				return term_; 
			}
			bool operator==(const Ref &b) const
			{
				return term_ == b.term_;
			}
		private:
			ConstraintAbsTerm *term_;
		};
		typedef std::vector<ConstraintAbsTerm::Ref*> Children;
	public:
		ConstraintAbsTerm()
		{
			val_.type  = ANY;
		}
		ConstraintAbsTerm(const Val &v, const Children &children = Children())
			: val_(v)
			, children_(children)
		{
		}
		bool any() const
		{
			return val_.type == ANY;
		}
		static bool unify(Ref &a, Ref &b)
		{
                    if(a == b) { return true;  }
                    else if(a->any())
                    {
                            a.replace(b);
                            return true;
                    }
                    else if(b->any())
                    {
                            b.replace(a);
                            return true;
                    }
                    else
                    {
                            if(a->val_ != b->val_)                         { return false; }
                            if(a->children_.size() != b->children_.size()) { return false; }
                            for(Children::iterator i = a->children_.begin(), j = b->children_.begin(); i != a->children_.end(); i++, j++)
                            {
                                    if(!ConstraintAbsTerm::unify(**i, **j)) { return false; }
                            }
                            return true;
                    }
		}
	private:
		Val      val_;
		Children children_;
	};

	typedef std::auto_ptr<ConstraintAbsTerm> ConstraintAbsTermPtr;

	class ConstraintSubstitution
	{
	public:
		typedef boost::ptr_vector<ConstraintAbsTerm> ConstraintAbsTermVec;
		typedef boost::ptr_vector<ConstraintAbsTerm::Ref> ConstraintAbsTermRefVec;
		typedef std::map<uint32_t, ConstraintAbsTerm::Ref*> VarMap;
		
		ConstraintSubstitution()
		{
		}
		ConstraintAbsTerm::Ref *anyVar()
		{
			return addTerm(new ConstraintAbsTerm());
		}
		ConstraintAbsTerm::Ref *mapVar(uint32_t var)
		{
			VarMap::iterator it = varMap_.find(var);
			if(it != varMap_.end()) { return it->second; }
			else
			{
				ConstraintAbsTerm::Ref *ref = addTerm(new ConstraintAbsTerm());
				varMap_.insert(VarMap::value_type(var, ref));
				return ref;
			}
		}
		void clearMap()
		{
			varMap_.clear();
		}
		
		ConstraintAbsTerm::Ref *addTerm(ConstraintAbsTerm *term)
		{
			terms_.push_back(term);
			refs_.push_back(new ConstraintAbsTerm::Ref(&terms_.back()));
			return &refs_.back();
		}
		
	private:
		VarMap        varMap_;
		ConstraintAbsTermVec    terms_;
		ConstraintAbsTermRefVec refs_;
	};

	typedef std::auto_ptr<ConstraintAbsTerm> ConstraintAbsTermPtr;

		
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
		virtual Val val(Grounder *grounder) const = 0;
		virtual Split split() { return Split(0, 0); }
                virtual void normalize(Lit *parent, const Ref &ref, Grounder *g, const Lit::Expander& expander, bool unify) = 0;
		virtual bool unify(Grounder *grounder, const Val &v, int binder) const = 0;
		virtual ConstraintAbsTerm::Ref *abstract(ConstraintSubstitution &subst) const = 0;
                virtual bool match(Grounder* g) = 0;
		virtual void vars(VarSet &v) const = 0;
		virtual void visit(PrgVisitor *visitor, bool bind) = 0;
		virtual bool constant() const = 0;
		virtual void print(Storage *sto, std::ostream &out) const = 0;
		virtual Term* toTerm() const = 0;
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
