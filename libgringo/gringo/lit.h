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
	virtual ~Expander();
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
	Lit(const Loc &loc);
	virtual Lit *clone() const = 0;

	virtual void normalize(Grounder *g, Expander *expander) = 0;

	// NOTE: match has to be called before:
	//       isFalse, fact, state, grounded, addDomain, forcePrint, and finish!
	virtual bool match(Grounder *grounder);
	virtual bool isFalse(Grounder *grounder) = 0;
	virtual bool fact() const = 0;

	bool head() const;
	void head(bool head);
	virtual void doHead(bool head);
	virtual bool complete() const;
	virtual bool edbFact() const;
	virtual Monotonicity monotonicity();

	virtual void grounded(Grounder *grounder);
	virtual void addDomain(Grounder *grounder, bool fact);
	virtual bool forcePrint();
	virtual void finish(Grounder *grounder);

	virtual void print(Storage *sto, std::ostream &out) const = 0;

	virtual void index(Grounder *g, Groundable *gr, VarSet &bound) = 0;
	virtual Score score(Grounder *g, VarSet &bound);

	virtual void visit(PrgVisitor *visitor) = 0;
	virtual void accept(Printer *v) = 0;

	// NOTE: so far PredLits are the only literals that store state
	//       if anything else should store state
	//       anothere abstraction is needed here
	PredLitRep *state();

	virtual ~Lit();
public:
	static bool cmpPos(const Lit *a, const Lit *b);

private:
	uint32_t head_ : 1;
public:
	uint32_t position : 31;
};

class Lit::Decorator : public Lit
{
public:
	Decorator(const Loc &loc);
	virtual Lit *decorated() const = 0;

	// Note: careful when forwarding this function
	virtual void normalize(Grounder *g, Expander *expander);
	virtual void init(Grounder *grounder, const VarSet &bound);

	virtual bool match(Grounder *grounder);
	virtual bool isFalse(Grounder *grounder);
	virtual bool fact() const;

	virtual bool complete() const;
	virtual void doHead(bool head);
	virtual bool edbFact() const;
	virtual Monotonicity monotonicity();

	virtual void grounded(Grounder *grounder);
	virtual void addDomain(Grounder *grounder, bool fact);
	virtual bool forcePrint();
	virtual void finish(Grounder *grounder);

	virtual void print(Storage *sto, std::ostream &out) const;

	virtual void index(Grounder *g, Groundable *gr, VarSet &bound);
	virtual Score score(Grounder *grounder, VarSet &bound);

	virtual void visit(PrgVisitor *visitor);
	virtual void accept(Printer *v);

	virtual PredLitRep *state();

	virtual ~Decorator();
};

Lit* new_clone(const Lit& a);

//////////////////////////// Expander ////////////////////////////

inline Expander::~Expander() { }

//////////////////////////// Lit ////////////////////////////

inline Lit::Lit(const Loc &loc) : Locateable(loc), head_(false), position(0) { }

inline bool Lit::match(Grounder *grounder) { return !isFalse(grounder); }

inline bool Lit::head() const { return head_; }
inline void Lit::head(bool head) { head_ = head; doHead(head); }
inline void Lit::doHead(bool) { }
inline bool Lit::complete() const { return true; }
inline bool Lit::edbFact() const { return false; }
inline Lit::Monotonicity Lit::monotonicity() { return MONOTONE; }

inline void Lit::grounded(Grounder *) { }
inline void Lit::addDomain(Grounder *, bool) { }
inline bool Lit::forcePrint() { return false; }
inline void Lit::finish(Grounder *) { }

inline Lit::Score Lit::score(Grounder *, VarSet &) { return Score(HIGHEST, std::numeric_limits<double>::min()); }

inline PredLitRep *Lit::state() { return 0; }

inline Lit::~Lit() { }

inline bool Lit::cmpPos(const Lit *a, const Lit *b) { return a->position < b->position; }

//////////////////////////// Lit::Decoreator ////////////////////////////

inline Lit::Decorator::Decorator(const Loc &loc) : Lit(loc) { }

inline void Lit::Decorator::normalize(Grounder *g, Expander *expander) { decorated()->normalize(g, expander); }

inline bool Lit::Decorator::match(Grounder *grounder) { return decorated()->match(grounder); }
inline bool Lit::Decorator::isFalse(Grounder *grounder) { return decorated()->isFalse(grounder); }
inline bool Lit::Decorator::fact() const { return decorated()->fact(); }

inline bool Lit::Decorator::complete() const { return decorated()->complete(); }
inline void Lit::Decorator::doHead(bool head) { decorated()->head(head); }
inline bool Lit::Decorator::edbFact() const { return decorated()->edbFact(); }
inline Lit::Monotonicity Lit::Decorator::monotonicity() { return decorated()->monotonicity(); }

inline void Lit::Decorator::grounded(Grounder *grounder) { decorated()->grounded(grounder); }
inline void Lit::Decorator::addDomain(Grounder *grounder, bool fact) { decorated()->addDomain(grounder, fact); }
inline bool Lit::Decorator::forcePrint() { return decorated()->forcePrint(); }
inline void Lit::Decorator::finish(Grounder *grounder) { decorated()->finish(grounder); }

inline void Lit::Decorator::print(Storage *sto, std::ostream &out) const { decorated()->print(sto, out); }

inline void Lit::Decorator::index(Grounder *g, Groundable *gr, VarSet &bound) { decorated()->index(g, gr, bound); }
inline Lit::Score Lit::Decorator::score(Grounder *grounder, VarSet &bound) { return decorated()->score(grounder, bound); }

inline void Lit::Decorator::visit(PrgVisitor *visitor) { decorated()->visit(visitor); }
inline void Lit::Decorator::accept(Printer *v) { decorated()->accept(v); }

inline PredLitRep *Lit::Decorator::state() { return decorated()->state(); }

inline Lit::Decorator::~Decorator() { }

//////////////////////////// other ////////////////////////////

inline Lit* new_clone(const Lit& a)
{
	return a.clone();
}
