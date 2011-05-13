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
#include <gringo/printer.h>
#include <gringo/formula.h>
#include <gringo/predlit.h>

class OptimizeSetLit : public Lit, public Matchable
{
public:
	OptimizeSetLit(const Loc &loc, TermPtrVec &terms);

	TermPtrVec *terms();

	bool match(Grounder *g);
	bool fact() const;

	void addDomain(Grounder *g, bool fact);
	Index *index(Grounder *g, Formula *gr, VarSet &bound);
	bool edbFact() const;
	void normalize(Grounder *g, Expander *e);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	void accept(Printer *v);
	Lit *clone() const;
	~OptimizeSetLit();

private:
	TermPtrVec terms_;
};

class Optimize : public SimpleStatement
{
public:
	class Printer : public ::Printer
	{
	public:
		void print(PredLitRep *l) { (void)l; assert(false); }
		virtual void begin(bool maximize, bool set) = 0;
		virtual void print(PredLitRep *l, int32_t weight, int32_t prio) = 0;
		virtual void end() = 0;
		virtual ~Printer() { }
	};

public:
	Optimize(const Loc &loc, TermPtrVec &terms, LitPtrVec &body, bool maximize, bool set, bool headLike);
	Optimize(const Optimize &opt, Lit *head);
	Optimize(const Optimize &opt, const OptimizeSetLit &setLit);
	void normalize(Grounder *g);
	void append(Lit *lit);
	bool grounded(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	~Optimize();

private:
	OptimizeSetLit setLit_;
	LitPtrVec      body_;
	bool           maximize_;
	bool           set_;
	bool           headLike_;
};

/*
typedef boost::shared_ptr<PredLitSet> PredLitSetPtr;

class Optimize : public SimpleStatement
{
private:
	class PrioLit;
	typedef std::vector<std::pair<int32_t, int32_t> > PrioVec;
	friend PrioLit* new_clone(const PrioLit& a);
public:
	class Printer : public ::Printer
	{
	public:
		void print(PredLitRep *l) { (void)l; assert(false); }
		virtual void begin(bool maximize, bool set) = 0;
		virtual void print(PredLitRep *l, int32_t weight, int32_t prio) = 0;
		virtual void end() = 0;
		virtual ~Printer() { }
	};
public:
	Optimize(const Loc &loc, PredLit *head, Term *weight, Term *prio, LitPtrVec &body, bool maximize);
	Term *weight();
	Term *prio();
	PredLit *head() const { return head_.get(); }
	LitPtrVec &body() { return body_; }
	void append(Lit *lit);
	void ground(Grounder *g);
	bool grounded(Grounder *g);
	void normalize(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	bool maximize() { return maximize_; }
	bool set() { return set_; }
	void uniques(PredLitSetPtr uniques) { uniques_ = uniques; set_ = true; }
	PredLitSetPtr uniques() { return uniques_; }
	~Optimize();
private:
	clone_ptr<PredLit> head_;
	clone_ptr<PrioLit> prio_;
	LitPtrVec          body_;
	PrioVec            prios_;
	bool               maximize_;
	//! whether the optimize occurs in a set (or else multiset)
	bool               set_;
	//! set for uniqueness check when set_ is true
	PredLitSetPtr      uniques_;
};

Optimize::PrioLit* new_clone(const Optimize::PrioLit& a);

*/
