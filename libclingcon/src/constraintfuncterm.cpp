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

#include <clingcon/constraintfuncterm.h>
#include <gringo/grounder.h>
#include <gringo/storage.h>
#include <gringo/func.h>
#include <gringo/lit.h>
#include <gringo/litdep.h>

namespace Clingcon
{

	ConstraintFuncTerm::ConstraintFuncTerm(const Loc &loc, uint32_t name, ConstraintTermPtrVec &args)
		: ConstraintTerm(loc)
		, name_(name)
		, args_(args)
	{
	}

	Val ConstraintFuncTerm::val(Grounder *grounder) const
	{
		ValVec vals;
		foreach(const ConstraintTerm &term, args_) vals.push_back(term.val(grounder));
		return Val::create(Val::FUNC, grounder->index(Func(grounder, name_, vals)));
	}

        void ConstraintFuncTerm::normalize(Lit *parent, const Ref &ref, Grounder *g, const Lit::Expander& expander, bool unify)
	{
		(void)ref;
		for(ConstraintTermPtrVec::iterator i = args_.begin(); i != args_.end(); i++)
		{
			for(ConstraintTerm::Split s = i->split(); s.first; s = i->split())
			{
				clone_.reset(new ConstraintFuncTerm(loc(), name_, (*s.second)));
                                expander(parent->clone(), Lit::POOL);
				args_.replace(i, s.first);
			}
			i->normalize(parent, ConstraintTerm::VecRef(args_, i), g, expander, unify);
		}
	}

	ConstraintAbsTerm::Ref* ConstraintFuncTerm::abstract(ConstraintSubstitution& subst) const
	{
		ConstraintAbsTerm::Children children;
		foreach(const ConstraintTerm &t, args_) { children.push_back(t.abstract(subst)); }
		return subst.addTerm(new ConstraintAbsTerm(Val::create(Val::ID, name_), children));
	}

	bool ConstraintFuncTerm::unify(Grounder *grounder, const Val &v, int binder) const
	{
		if(v.type != Val::FUNC) return false;
		const Func &f = grounder->func(v.index);
		if(name_ != f.getName()) return false;
		if(args_.size() != f.getArgs().size()) return false;
		for(ConstraintTermPtrVec::size_type i = 0; i < args_.size(); i++)
			if(!args_[i].unify(grounder, f.getArgs()[i], binder)) return false;
		return true;
	}

	void ConstraintFuncTerm::vars(VarSet &v) const
	{
		foreach(const ConstraintTerm &t, args_) t.vars(v);
	}

	void ConstraintFuncTerm::visit(PrgVisitor *visitor, bool bind)
	{
		foreach(ConstraintTerm &t, args_) t.visit(visitor, bind);
	}

	bool ConstraintFuncTerm::constant() const
	{
		foreach(const ConstraintTerm &t, args_)
			if(!t.constant()) return false;
		return true;
	}

	void ConstraintFuncTerm::print(Storage *sto, std::ostream &out) const
	{
		out << sto->string(name_);
		out << "(";
		bool comma = false;
		foreach(const ConstraintTerm &t, args_)
		{
			if(comma) out << ",";
			else comma = true;
			t.print(sto, out);
		}
		out << ")";
	}

        ConstraintFuncTerm *ConstraintFuncTerm::clone() const
	{
		// explicit downcast
		if(clone_.get()) return clone_.release();
		else return new ConstraintFuncTerm(*this);
	}

        GroundConstraint* ConstraintFuncTerm::toGroundConstraint(Grounder* g)
	{
                return new GroundConstraint(g,val(g));
	}


	ConstraintFuncTerm::~ConstraintFuncTerm()
	{
	}
}
