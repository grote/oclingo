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

class Formula : public Locateable
{
public:
	Formula(const Loc &loc);

	virtual void initInst(Grounder *g) = 0;
	virtual void visit(PrgVisitor *visitor) = 0;
	virtual void print(Storage *sto, std::ostream &out) const = 0;
	virtual void enqueue(Grounder *g) = 0;

	VarVec &vars();
	uint32_t level() const;
	void level(uint32_t level);
	void litDep(LitDep::FormulaNode *litDep);
	LitDep::FormulaNode *litDep();

	void simpleInitInst(Grounder *g, Instantiator &inst);

	virtual ~Formula();

protected:
	uint32_t                       level_;
	clone_ptr<LitDep::FormulaNode> litDep_;
	VarVec                         vars_;
};

class Groundable
{
public:
	virtual void ground(Grounder *g) = 0;
	virtual ~Groundable();
};

class Statement : public Formula
{
public:
	Statement(const Loc &loc);
	void check(Grounder *g);
	void init(Grounder *g);

	virtual void normalize(Grounder *g) = 0;
	virtual void append(Lit *lit) = 0;
	virtual bool edbFact() const;
	virtual bool choice() const;
	virtual ~Statement();
};

class SimpleStatement : public Statement, public Groundable
{
public:
	SimpleStatement(const Loc &loc);

	virtual bool grounded(Grounder *g) = 0;

	void initInst(Grounder *g);
	void ground(Grounder *g);
	void enqueue(Grounder *g);

	virtual ~SimpleStatement();

protected:
	virtual void doGround(Grounder *g) = 0;

protected:
	clone_ptr<Instantiator> inst_;
	bool                    enqueued_;
};

///////////////////////////////////// Formula /////////////////////////////////////

inline VarVec &Formula::vars() { return vars_; }
inline uint32_t Formula::level() const { return level_; }
inline void Formula::level(uint32_t level) { level_ = level; }
inline LitDep::FormulaNode *Formula::litDep() { return litDep_.get(); }

///////////////////////////////////// Groundable /////////////////////////////////////

inline Groundable::~Groundable() { }

///////////////////////////////////// Statement /////////////////////////////////////

inline bool Statement::edbFact() const { return false; }
inline bool Statement::choice() const { return false; }
inline Statement::~Statement() { }
