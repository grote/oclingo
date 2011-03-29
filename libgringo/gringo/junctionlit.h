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

/*
JunctionCond::ground
	- ground the conditionals (depends on current substitution)
	- this fills a table
	- check whether all heads match
*/

class JunctionLitDomain
{
public:
	void init(Formula *grd, const VarVec &global);
private:
	VarVec  &global_;
	Formula *grd_;
};

class JunctionCondTableEntry
{
public:

private:
	ValVec local_;
	bool   matched_;
};

class JunctionCondTable
{
	typedef std::vector<JunctionCondTableEntry> Table;
public:

private:
	Table    table_;
	uint32_t matched_;
	uint32_t fact_;
};

class JunctionCondDomain
{
	typedef boost::unordered_map<ValVec, JunctionCondTable> TableMap;
public:
	//JunctionCondDomain();
	void init(JunctionLitDomain &parent, const VarVec &local);

private:
	VarVec             local_;
	TableMap           tableMap_;
	JunctionLitDomain *parent_;
};

class JunctionCond : public Formula
{
public:
	JunctionCond(const Loc &loc, Lit *head, LitPtrVec &body);
	void normalize(Grounder *g, Expander *headExp, Expander *bodyExp);
	LitPtrVec &body();
	void init(JunctionLitDomain &dom);
	void initInst(Grounder *g);

	void enqueue(Grounder *g);
	bool grounded(Grounder *g);
	void ground(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;

	~JunctionCond();
private:
	clone_ptr<Lit>     head_;
	LitPtrVec          body_;
	JunctionCondDomain dom_;
};

class JunctionLit : public Lit
{
public:
	class Printer : public ::Printer
	{
	public:
		virtual void begin(bool head) = 0;
		virtual void end() = 0;
		virtual ~Printer() { }
	};
public:
	JunctionLit(const Loc &loc, JunctionCondVec &conds);

	JunctionCondVec &conds();

	void normalize(Grounder *g, Expander *expander);

	bool fact() const;

	void print(Storage *sto, std::ostream &out) const;

	Index *index(Grounder *g, Formula *gr, VarSet &bound);
	Score score(Grounder *g, VarSet &bound);

	void visit(PrgVisitor *visitor);
	void accept(::Printer *v);

	Lit *clone() const;

	~JunctionLit();

private:
	bool              fact_;
	JunctionCondVec   conds_;
	JunctionLitDomain dom_;
};

/////////////////////////// JunctionCond ///////////////////////////

inline LitPtrVec &JunctionCond::body() { return body_; }

/////////////////////////// JunctionLit ///////////////////////////

inline JunctionCondVec &JunctionLit::conds() { return conds_; }
