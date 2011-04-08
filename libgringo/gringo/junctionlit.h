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

class JunctionLit;

struct JunctionState
{
	JunctionState();

	bool   match;
	bool   fact;
	bool   isNew;
	ValVec vals;
};

class JunctionLitDomain
{
	typedef std::vector<VarVec> VarVecVec;
	typedef boost::unordered_map<ValVec, JunctionState>  StateMap;
public:
	JunctionLitDomain();

	void state(Grounder *g);
	void accumulate(Grounder *g, uint32_t index);
	BoolPair match(Grounder *g);
	void print(Printer *p);

	bool fact() const;
	bool hasNew() const;
	bool newState() const;
	void finish();
	void enqueue(Grounder *g);

	void initGlobal(Grounder *g, Formula *f, const VarVec &global, bool head_);
	void initLocal(Grounder *g, Formula *f, uint32_t index, Lit &head);
private:
	VarVec         global_;
	VarVecVec      local_;
	LitVec         heads_;
	StateMap       state_;
	IndexPtrVec    indices_;
	Grounder      *g_;
	JunctionState *current_;
	Formula       *f_;
	bool           new_;
	bool           newState_;
	bool           head_;
};

class JunctionCond : public Formula
{
public:
	JunctionCond(const Loc &loc, Lit *head, LitPtrVec &body);
	LitPtrVec &body();
	void init(Grounder *g, JunctionLitDomain &dom);
	void normalize(Grounder *g, Expander *headExp, Expander *bodyExp, JunctionLit *parent, uint32_t index);

	void finish();
	void ground(Grounder *g);
	void initInst(Grounder *g);
	void enqueue(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;

	~JunctionCond();
private:
	clone_ptr<Lit>          head_;
	LitPtrVec               body_;
	clone_ptr<Instantiator> inst_;
	uint32_t                index_;
	JunctionLit            *parent_;
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

	BoolPair ground(Grounder *g);
	void enqueue(Grounder *g);
	bool groundedCond(Grounder *grounder, uint32_t index);

	JunctionLitDomain &domain();
	JunctionCondVec &conds();

	void normalize(Grounder *g, Expander *expander);

	bool fact() const;

	void print(Storage *sto, std::ostream &out) const;

	Index *index(Grounder *g, Formula *f, VarSet &bound);
	Score score(Grounder *g, VarSet &bound);

	void visit(PrgVisitor *visitor);
	void accept(::Printer *v);

	Lit *clone() const;

	~JunctionLit();

private:
	bool              match_;
	bool              fact_;
	JunctionCondVec   conds_;
	JunctionLitDomain dom_;
};

/////////////////////////// JunctionLitDomain ///////////////////////////
inline bool JunctionLitDomain::hasNew() const { return new_; }
inline bool JunctionLitDomain::newState() const { return newState_; }

/////////////////////////// JunctionCond ///////////////////////////

inline LitPtrVec &JunctionCond::body() { return body_; }

/////////////////////////// JunctionLit ///////////////////////////

inline JunctionCondVec &JunctionLit::conds() { return conds_; }
inline JunctionLitDomain &JunctionLit::domain() { return dom_; }

