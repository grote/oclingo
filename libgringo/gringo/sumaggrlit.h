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
#include <gringo/aggrlit.h>

class SumAggrLit : public AggrLit
{
public:
	class Printer : public ::Printer
	{
	public:
		typedef CondLit::Printer::State State;
	public:
		virtual void begin(State state, bool head, bool sign, bool complete) = 0;
		virtual void lower(int32_t l, bool leq) = 0;
		virtual void upper(int32_t u, bool leq) = 0;
		virtual void end() = 0;
		virtual ~Printer();
	};
public:
	SumAggrLit(const Loc &loc, CondLitVec &conds, bool posWeights, bool set);
	AggrState *newAggrState(Grounder *g);
	void print(Storage *sto, std::ostream &out) const;
	Lit *clone() const;
	void accept(::Printer *v);
	Monotonicity monotonicity() const;
	bool fact() const;
	void checkWeight(Grounder *g, const Val &weight);
private:
	bool posWeights_;
};

////////////////////////////// SumAggrLit::Printer //////////////////////////////

inline SumAggrLit::Printer::~Printer() { }
