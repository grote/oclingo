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
#include <gringo/lit.h>

/********************************************************
 * TODO: i am not satisfied with the way the
 * (incremental) grounding algorithm works right now
 * the semi-naive evaluation itself should be used to trigger grounding
 * this carries over to changing the overall grounding algorithm
 * the domains associated to indices should guide the grounding
 * at no point the ground method of a rule should be called directly
 * algorithm:
 * loop over components:
 *   loop over domains in component:
 *     if domain changed:
 *       domain should enqueue rules from active component
 * datastructures:
 *   abstract from domain
 *   incremental literals have a special associated domain
 *   all rules need to be grounded initially
 *     pseudo domain that enqueues every rule once
 *     the instantiator initially ignores hasNew values
 *       it is implicit that first match is new
 *       no need to implement this function for match only
 *       and related indices anymore
 */

struct IncConfig
{
	int  incStep;
	bool incVolatile;

	IncConfig()
		: incStep(std::numeric_limits<int>::min())
		, incVolatile(false)
	{ }

	bool curBase() const
	{
		assert(incStep != std::numeric_limits<int>::min() || !incVolatile);
		return incStep == std::numeric_limits<int>::min();
	}

	bool curCumulative() const
	{
		return !incVolatile && !curBase();
	}

	bool curVolatile() const
	{
		assert(!incVolatile || !curBase());
		return incVolatile;
	}

};

class IncLit : public Lit
{
public:
	class Printer : public ::Printer
	{
	public:
		virtual void print() { }
		virtual ~Printer() { }
	};
	enum Type { BASE, UNBOUND, CUMULATIVE, VOLATILE };

public:
	IncLit(const Loc &loc, IncConfig &config_, Type type, uint32_t varId);
	void normalize(Grounder *g, Expander *expander);
	bool fact() const { return true; }
	bool isFalse(Grounder *g);
	bool forcePrint() { return type_ == VOLATILE; }
	void accept(::Printer *v);
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	void visit(PrgVisitor *visitor);
	bool match(Grounder *grounder);
	void print(Storage *sto, std::ostream &out) const;
	void bind();
	double score(Grounder *) const;
	Lit *clone() const;
	const IncConfig &config() const { return config_; }
	const VarTerm *var() const { return var_.get(); }
	Type type() const { return type_; }
	~IncLit();
private:
	IncConfig         &config_;
	Type               type_;
	clone_ptr<VarTerm> var_;
};
