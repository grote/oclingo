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
#include <gringo/locateable.h>

class AbsTerm
{
	enum Type { ANY = Val::SUP + 1 };
public:
	class Ref
	{
	public:
		Ref(AbsTerm *term) 
			: term_(term)
		{
		}
		void replace(Ref &ref)
		{
			term_ = ref.term_;
		}
		AbsTerm *operator->() const
		{
			return term_; 
		}
		bool operator==(const Ref &b) const
		{
			return term_ == b.term_;
		}
	private:
		AbsTerm *term_;
	};
	typedef std::vector<AbsTerm::Ref*> Children;
public:
	AbsTerm()
	{
		val_.type  = ANY;
	}
	AbsTerm(const Val &v, const Children &children = Children())
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
			typedef boost::tuple<AbsTerm::Ref*, AbsTerm::Ref*> TermPair;
			foreach(const TermPair &tuple, _boost::combine(a->children_, b->children_))
			{
				if(!AbsTerm::unify(*tuple.get<0>(), *tuple.get<1>())) { return false; }
			}
			return true;
		}
	}
private:
	Val      val_;
	Children children_;
};

typedef std::auto_ptr<AbsTerm> AbsTermPtr;

class Substitution
{
public:
	typedef boost::ptr_vector<AbsTerm> AbsTermVec;
	typedef boost::ptr_vector<AbsTerm::Ref> AbsTermRefVec;
	typedef std::map<uint32_t, AbsTerm::Ref*> VarMap;
	
	Substitution()
	{
	}
	AbsTerm::Ref *anyVar()
	{
		return addTerm(new AbsTerm());
	}
	AbsTerm::Ref *mapVar(uint32_t var)
	{
		VarMap::iterator it = varMap_.find(var);
		if(it != varMap_.end()) { return it->second; }
		else
		{
			AbsTerm::Ref *ref = addTerm(new AbsTerm());
			varMap_.insert(VarMap::value_type(var, ref));
			return ref;
		}
	}
	void clearMap()
	{
		varMap_.clear();
	}
	
	AbsTerm::Ref *addTerm(AbsTerm *term)
	{
		terms_.push_back(term);
		refs_.push_back(new AbsTerm::Ref(&terms_.back()));
		return &refs_.back();
	}
	
private:
	VarMap        varMap_;
	AbsTermVec    terms_;
	AbsTermRefVec refs_;
};

typedef std::auto_ptr<AbsTerm> AbsTermPtr;

class Term : public Locateable
{
public:
	typedef std::pair<Term*, TermPtrVec*> Split;
	class Ref
	{
	public:
		virtual void reset(Term *a) const = 0;
		virtual void replace(Term *a) const = 0;
		virtual ~Ref() { }
	};
	class VecRef : public Ref
	{
	public:
		VecRef(TermPtrVec &vec, TermPtrVec::iterator pos) : vec_(vec), pos_(pos) { }
		void reset(Term *a) const { vec_.replace(pos_, a); }
		void replace(Term *a) const { vec_.replace(pos_, a).release(); }
	private:
		TermPtrVec          &vec_;
		TermPtrVec::iterator pos_;
	};
	class PtrRef : public Ref
	{
	public:
		PtrRef(clone_ptr<Term> &ptr) : ptr_(ptr) { }
		void reset(Term *a) const { ptr_.reset(a); }
		void replace(Term *a) const { ptr_.release(); ptr_.reset(a); }
	private:
		clone_ptr<Term> &ptr_;
	};
public:
	Term(const Loc &loc) : Locateable(loc) { }
	virtual Val val(Grounder *grounder) const = 0;
	virtual Split split() { return Split(0, 0); }
	virtual void normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify) = 0;
	virtual bool unify(Grounder *grounder, const Val &v, int binder) const = 0;
	virtual AbsTerm::Ref *abstract(Substitution &subst) const = 0;
	virtual void vars(VarSet &v) const = 0;
	virtual void visit(PrgVisitor *visitor, bool bind) = 0;
	virtual bool constant() const = 0;
	virtual void print(Storage *sto, std::ostream &out) const = 0;
	virtual Term *clone() const = 0;
	virtual ~Term() { }
};

inline Term* new_clone(const Term& a)
{
	return a.clone();
}

