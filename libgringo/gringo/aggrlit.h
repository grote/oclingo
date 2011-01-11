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
#include <gringo/lit.h>
#include <gringo/predlit.h>
#include <gringo/printer.h>
#include <gringo/groundable.h>

class AggrLit : public Lit
{
public:
	class Printer : public ::Printer
	{
	public:
		virtual void weight(const Val &v) = 0;
		virtual ~Printer() { }
	};
public:
	AggrLit(const Loc &loc, CondLitVec &conds);
	AggrLit(const AggrLit &aggr);
	void lower(Term *l);
	void upper(Term *u);
	void assign(Term *a);
	Term *assign() const { assert(assign_); return lower_.get(); }
	void add(CondLit *cond);
	const CondLitVec &conds() { return conds_; }
	void sign(bool s) { sign_ = s; }
	bool fact() const { return fact_; }
	virtual Monotonicity monotonicity() { return NONMONOTONE; }
	virtual void normalize(Grounder *g, Expander *expander);
	virtual tribool accumulate(Grounder *g, const Val &weight, Lit &lit) throw(const Val*) = 0;
	void addDomain(Grounder *g, bool fact);
	void finish(Grounder *g);
	void visit(PrgVisitor *visitor);
	void grounded(Grounder *grounder) { (void)grounder; }
	#pragma message "aggrolits are complete if the conditionals are complete!"
	bool complete() const { return false; }
	void init(Grounder *g, const VarSet &bound);
	Score score(Grounder *, VarSet &) { return Score(LOWEST,std::numeric_limits<double>::max()); }
	~AggrLit();
protected:
	bool            sign_;
	bool            assign_;
	bool            fact_;
	clone_ptr<Term> lower_;
	clone_ptr<Term> upper_;
	CondLitVec      conds_;
	/*
	 * AggreIndx : Global Substitution -> Set of Local Substitutions
	 * use push/pop + top mechanism or implement somethig own?
	 * local substitution could be a set of top values?
	 *   i.e.: varvec -> intvec
	 * would need this per conditional store in conditional?
	 */
};

// TODO: implementation detail; put into source file
//       needs normalziation?

class SetLit : public Lit
{
public:
	SetLit(const Loc &loc, Term *weight);

	bool match(Grounder *) { return true; }
	bool isFalse(Grounder *) { return false; }
	bool fact() const { return true; }

	void addDomain(Grounder *, bool) { assert(false); }
	void index(Grounder *, Groundable *, VarSet &) { }
	bool edbFact() const { return false; }
	void normalize(Grounder *, Expander *) { assert(false); }
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	void accept(Printer *v) { (void)v; }
	Lit *clone() const;
	~SetLit();
private:
	TermPtrVec terms_; //! determines the uniqueness + holds the weight
};

inline SetLit* new_clone(const SetLit& a)
{
	return static_cast<SetLit*>(a.clone());
}

class CondLit : public Groundable, public Locateable
{
	friend class AggrLit;
public:
	CondLit(const Loc &loc, Lit *head, Term *weight, LitPtrVec &body);
	void add(Lit *lit) { lits_.push_back(lit); }
	bool grounded(Grounder *g);
	void normalize(Grounder *g, Expander *headExp, Expander *bodyExp);
	void init(Grounder *g, const VarSet &bound);
	void ground(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	void accept(AggrLit::Printer *v);
	void addDomain(Grounder *g, bool fact) ;
	void finish(Grounder *g);
	~CondLit();
private:
	SetLit     set_;   //! determines uniqueness + holds weight (first position)
	LitPtrVec  lits_;  //! list of conditionals optionally including head at first position
	AggrLit    *aggr_; //! aggregate this conditional belongs to
};

