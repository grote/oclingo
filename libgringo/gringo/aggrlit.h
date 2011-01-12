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
		virtual ~Printer();
	};
public:
	AggrLit(const Loc &loc, CondLitVec &conds);
	AggrLit(const AggrLit &aggr);

	void lower(Term *l);
	void upper(Term *u);
	void assign(Term *a);
	Term *assign() const;
	void sign(bool s);

	void add(CondLit *cond);
	const CondLitVec &conds();

	~AggrLit();

	// Lit interface
	virtual void normalize(Grounder *g, Expander *expander);

	bool fact() const;

	bool complete() const;

	virtual Monotonicity monotonicity();

	void grounded(Grounder *grounder);
	void addDomain(Grounder *g, bool fact);
	void finish(Grounder *g);

	void visit(PrgVisitor *visitor);

	Score score(Grounder *g, VarSet &bound);

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
	VarVec varsGlobal_;
	class AggrState
	{
		// TODO: do the accumulation here (interface)
		// ValVec -> stuff in conditionals; number of conditional + number of offsets + offsets
		// the conditional doesn't need to know anything
		boost::unordered_map<ValVec, std::vector<uint32_t> > offsets_;
	};
	// maps global substitution to State
	typedef boost::ptr_map<ValVec, AggrState> AggrIndex;

};

class SetLit : public Lit
{
public:
	SetLit(const Loc &loc, TermPtrVec &terms);

	TermPtrVec *terms() { return &terms_; }

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

SetLit* new_clone(const SetLit& a);

class CondLit : public Groundable, public Locateable
{
public:
	enum Style { STYLE_DLV, STYLE_LPARSE, STYLE_LPARSE_SET };
	friend class AggrLit;
public:
	CondLit(const Loc &loc, TermPtrVec &terms, LitPtrVec &lits, Style style);
	AggrLit *aggr();
	TermPtrVec *terms();
	LitPtrVec *lits();
	Style style();
	bool complete() const;
	void add(Lit *lit) { lits_.push_back(lit); }

	bool grounded(Grounder *g);
	void normalize(Grounder *g, uint32_t number);
	void init(Grounder *g, const VarSet &bound);
	void ground(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	void accept(AggrLit::Printer *v);
	void addDomain(Grounder *g, bool fact) ;
	void finish(Grounder *g);
	~CondLit();
private:
	Style      style_; //! only affects normalization
	SetLit     set_;   //! determines uniqueness + holds weight (first position)
	LitPtrVec  lits_;  //! list of conditionals optionally including head at first position
	AggrLit    *aggr_; //! aggregate this conditional belongs to
};

/////////////////////////////// AggrLit::Printer ///////////////////////////////

inline AggrLit::Printer::~Printer() { }

/////////////////////////////// AggrLit ///////////////////////////////

inline Term *AggrLit::assign() const
{
	assert(assign_);
	return lower_.get();
}
inline void AggrLit::sign(bool s) { sign_ = s; }

inline const CondLitVec &AggrLit::conds() { return conds_; }

inline bool AggrLit::fact() const { return fact_; }

inline Lit::Monotonicity AggrLit::monotonicity() { return NONMONOTONE; }

inline void AggrLit::grounded(Grounder *) { }

inline Lit::Score AggrLit::score(Grounder *, VarSet &) { return Score(LOWEST, std::numeric_limits<double>::max()); }

/////////////////////////////// CondLit ///////////////////////////////

inline AggrLit *CondLit::aggr() { return aggr_; }

inline TermPtrVec *CondLit::terms() { return set_.terms(); }

inline LitPtrVec *CondLit::lits() { return &lits_; }

inline CondLit::Style CondLit::style() { return style_;  }

