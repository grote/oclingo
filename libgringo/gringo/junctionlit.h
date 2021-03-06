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
#include <gringo/printer.h>
#include <gringo/formula.h>

struct JunctionIndex;
class JunctionLit;

class JunctionCond : public Formula
{
	friend class JunctionLit;
public:
	JunctionCond(const Loc &loc, Lit *head, LitPtrVec &body);
	void normalize(Grounder *g, const Lit::Expander &headExp, const Lit::Expander &bodyExp, JunctionLit *parent);

	bool complete() const;
	bool grounded(Grounder *g, Index &head, JunctionIndex &index);
	void finish();
	bool ground(Grounder *g);
	void init(Grounder *g, JunctionIndex &index);
	void enqueue(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;

	~JunctionCond();

private:
	void addIndex(Grounder *g, VarSet &bound, Lit *lit);

private:
	clone_ptr<Lit>          head_;
	LitPtrVec               body_;
	clone_ptr<Instantiator> inst_;
	JunctionLit            *parent_;
};

class JunctionLit : public Lit
{
public:
	class Printer : public ::Printer
	{
	public:
		virtual void beginHead(bool disjunction, uint32_t uidJunc, uint32_t uidSubst) = 0;
		virtual void beginBody() = 0;
		virtual void printCond(bool bodyComplete) = 0;
		virtual void printJunc(bool disjunction, uint32_t juncUid, uint32_t substUid) = 0;
		virtual ~Printer() { }
	};
public:
	JunctionLit(const Loc &loc, JunctionCondVec &conds);

	uint32_t uid() const;
	bool ground(Grounder *g);
	void finish();
	void enqueue(Grounder *g);
	bool groundedCond(Grounder *grounder, uint32_t index);

	JunctionCondVec &conds();

	void normalize(Grounder *g, const Expander &e);

	bool complete() const;
	bool fact() const;
	void setState(bool fact, uint32_t substUid);

	void print(Storage *sto, std::ostream &out) const;

	Index *index(Grounder *g, Formula *f, VarSet &bound);
	Score score(Grounder *g, VarSet &bound);

	void visit(PrgVisitor *visitor);
	void accept(::Printer *v);

	Lit *clone() const;

	~JunctionLit();
private:
	void expandHead(const Lit::Expander &ruleExp, JunctionCond &cond, Lit *lit, Lit::ExpansionType type);

private:
	JunctionCondVec   conds_;
	Formula          *parent_;
	JunctionIndex    *index_;
	uint32_t          uid_;
	// Note: fluent state information (best find another way)
	uint32_t          substUid_;
	bool              fact_;

};

/////////////////////////// JunctionLit ///////////////////////////

inline JunctionCondVec &JunctionLit::conds() { return conds_; }
