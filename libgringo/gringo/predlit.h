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
#include <gringo/predlitrep.h>

struct VarDomains
{
	VarDomains() : offset(0) { }
	typedef std::map<uint32_t, boost::unordered_set<Val> > Map;
	uint32_t offset;
	Map      map;
};

class BodyOrderHeuristic
{
public:
	virtual ~BodyOrderHeuristic() {}
	//! Estimates the number of possible instantiations for a term
	/** \param score literal score
	  * \param vars variables in the literal
	  * \param free variables that are not bound
	  * \param varDoms possible values for each variable
	  */
	virtual Lit::Score score(Grounder *g, VarSet &bound, PredLit *pred) = 0;
};

class BasicBodyOrderHeuristic : public BodyOrderHeuristic
{
public:
	virtual ~BasicBodyOrderHeuristic() {}
	virtual Lit::Score score(Grounder *g, VarSet &bound, PredLit *pred);
};

class UnifyBodyOrderHeuristic : public BodyOrderHeuristic
{
public:
	virtual ~UnifyBodyOrderHeuristic() {}
	virtual Lit::Score score(Grounder *g, VarSet &bound, PredLit *pred);
};

class PredLitSet
{
private:
	typedef std::pair<PredLit*, size_t> PredSig;
	struct PredCmp
	{
		PredCmp(const ValVec &vals);
		size_t operator()(const PredSig &a) const;
		bool operator()(const PredSig &a, const PredSig &b) const;
		const ValVec &vals_;
	};
	typedef boost::unordered_set<PredSig, PredCmp, PredCmp> PredSet;
public:
	PredLitSet();
	bool insert(PredLit *pred, size_t pos, Val &val);
	void clear();
private:
	PredSet set_;
	ValVec  vals_;
};

class PredLit : public Lit, public PredLitRep
{
private:
	typedef std::vector<PredLit*> PredLitVec;
public:
	PredLit(const Loc &loc, Domain *dom, TermPtrVec &terms);
	void normalize(Grounder *g, Expander *expander);
	bool fact() const;
	bool isFalse(Grounder *g);
	Monotonicity monotonicity();
	void addDomain(Grounder *g, bool fact);
	void accept(Printer *v);
	/** \note grounded has to be called before reading top_ or vals_. */
	void grounded(Grounder *grounder);
	bool match(Grounder *grounder);
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	void visit(PrgVisitor *visitor);
	bool edbFact() const;
	void complete(bool complete) { complete_ = complete; }
	bool complete() const { return complete_; }
	void finish(Grounder *g);
	void print(Storage *sto, std::ostream &out) const;
	void push();
	bool testUnique(PredLitSet &set, Val val=Val::create());
	void pop();
	void move(size_t p);
	void clear();
	Score score(Grounder *g, VarSet &bound);
	Lit *clone() const;
	const VarDomains &allVals(Grounder *g);
	const TermPtrVec &terms() const { return terms_; }
	void vars(VarSet &vars) const;
    bool compatible(PredLit* pred);
	void provide(PredLit *pred);
private:
	TermPtrVec         terms_;
	VarDomains         varDoms_;
	PredLitVec         provide_;
	bool               complete_;
	uint32_t           startNew_;
	PredIndex         *index_;
	Groundable        *parent_;
};

