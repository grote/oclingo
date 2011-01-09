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

class Expander
{
public:
	enum Type { POOL, RANGE, RELATION };
public:
	virtual void expand(Lit *lit, Type type) = 0;
	virtual ~Expander() { }
};

class Lit : public Locateable
{
public:
	struct LitCmp
	{
		LitCmp(Grounder &g, VarSet &b) : grounder(g), bound(b) {}
		Grounder &grounder;
		VarSet &bound;
		bool operator()(Lit *a, Lit *b)
		{
			return a->score(&grounder, bound) < b->score(&grounder, bound);
		}
	};
public:
	class Decorator;
public:
	enum Monotonicity { MONOTONE, ANTIMONOTONE, NONMONOTONE };
	enum Priority { HIGHEST=0, CHECK_ONLY=8, RECURSIVE=32, NON_RECURSIVE=64, LOWEST=255};
	typedef std::pair<Priority,double> Score;
	
public:
	Lit(const Loc &loc) : Locateable(loc), head_(false), position(0) { }
	virtual void normalize(Grounder *g, Expander *expander) = 0;
	virtual Monotonicity monotonicity() { return MONOTONE; }
	virtual bool fact() const = 0;
	virtual bool forcePrint() { return false; }
	virtual bool match(Grounder *grounder) = 0;
	virtual void addDomain(Grounder *grounder, bool fact) { (void)grounder; (void)fact; assert(false); }
	virtual void grounded(Grounder *grounder) { (void)grounder; }
	virtual bool complete() const { return true; }
	virtual void finish(Grounder *) { }

	/** whether the literal is the head of a rule. */
	virtual bool head() const { return head_; }

	virtual void head(bool head)  { head_ = head; }
	virtual void index(Grounder *g, Groundable *gr, VarSet &bound) = 0;
	virtual void visit(PrgVisitor *visitor) = 0;
	virtual bool edbFact() const { return false; }
	virtual void print(Storage *sto, std::ostream &out) const = 0;
	virtual void accept(Printer *v) = 0;
	virtual void init(Grounder *grounder, const VarSet &bound) { (void)grounder; (void)bound; }
	virtual Lit *clone() const = 0;
	virtual void push() { }

	/** whether the literal is in the set. Used in aggregates and optimize statements.
	 * \param val additional information (e.g. used to distinguish literals with priorities in optimize statements)
	 */
	virtual bool testUnique(PredLitSet&, Val=Val::create()) { return true; }

	virtual void pop() { }
	virtual void move(size_t p) { (void)p; }
	virtual void clear() { }
	virtual Score score(Grounder *, VarSet &) { return Score(HIGHEST, std::numeric_limits<double>::min()); }
	virtual bool isFalse(Grounder *grounder) { (void)grounder; assert(false); return false; }
	virtual ~Lit() { }

public:
	static bool cmpPos(const Lit *a, const Lit *b)
	{
		return a->position < b->position;
	}

private:
	uint32_t head_ : 1;

public:
	uint32_t position : 31;
};

class Lit::Decorator : public Lit
{
public:
	Decorator(const Loc &loc) : Lit(loc) { }
	virtual Lit *decorated() const = 0;
	virtual void normalize(Grounder *g, Expander *expander) { decorated()->normalize(g, expander); }
	virtual Monotonicity monotonicity() { return decorated()->monotonicity(); }
	virtual bool fact() const { return decorated()->fact(); }
	virtual bool forcePrint() { return decorated()->forcePrint(); }
	virtual bool match(Grounder *grounder) { return decorated()->match(grounder); }
	virtual void addDomain(Grounder *grounder, bool fact) { decorated()->addDomain(grounder, fact); }
	virtual void grounded(Grounder *grounder) { decorated()->grounded(grounder); }
	virtual bool complete() const { return decorated()->complete(); }
	virtual void finish(Grounder *grounder) { decorated()->finish(grounder); }
	virtual bool head() const { return decorated()->head(); }
	virtual void head(bool head) { decorated()->head(head); }
	virtual void index(Grounder *g, Groundable *gr, VarSet &bound) { decorated()->index(g, gr, bound); }
	virtual void visit(PrgVisitor *visitor) { decorated()->visit(visitor); }
	virtual bool edbFact() const { return decorated()->edbFact(); }
	virtual void print(Storage *sto, std::ostream &out) const { decorated()->print(sto, out); }
	virtual void accept(Printer *v) { decorated()->accept(v); }
	virtual void init(Grounder *grounder, const VarSet &bound) { decorated()->init(grounder, bound); }
	virtual void push() { decorated()->push() ; }
	virtual bool testUnique(PredLitSet &set, Val v)  { return decorated()->testUnique(set, v); }
	virtual void pop() { decorated()->pop(); }
	virtual void move(size_t p) { decorated()->move(p); }
	virtual void clear() { decorated()->clear(); }
	virtual Score score(Grounder *grounder, VarSet &bound) { return decorated()->score(grounder, bound); }
	virtual bool isFalse(Grounder *grounder) { return decorated()->isFalse(grounder); }
	virtual ~Decorator() { }
};

inline Lit* new_clone(const Lit& a)
{
	return a.clone();
}

