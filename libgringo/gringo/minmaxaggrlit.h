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

class MinMaxAggrLit : public AggrLit
{
public:
	enum Type { MINIMUM, MAXIMUM };
public:
	MinMaxAggrLit(const Loc &loc, AggrCondVec &conds, Type type, bool set);
	AggrState *newAggrState(Grounder *g);
	void print(Storage *sto, std::ostream &out) const;
	Lit *clone() const;
	void accept(::Printer *v);
	Monotonicity monotonicity() const;
	bool fact() const;
	void checkWeight(Grounder *g, const Val &weight);
	Type type() const;
private:
	template<uint32_t T>
	void _accept(::Printer *v);
private:
	Type type_;
};

/////////////////////////////////// MinMaxAggrLit ///////////////////////////////////

inline MinMaxAggrLit::Type MinMaxAggrLit::type() const { return type_; }
