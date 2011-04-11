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
#include <gringo/index.h>

class BooleanLit : public Lit, public Matchable
{
public:
	BooleanLit(const Loc &loc, bool truth);
	void normalize(Grounder *g, Expander *expander);
	bool fact() const;
	void accept(Printer *v);
	Index *index(Grounder *g, Formula *gr, VarSet &bound);
	void visit(PrgVisitor *visitor);
	bool match(Grounder *grounder);
	void print(Storage *sto, std::ostream &out) const;
	BooleanLit *clone() const;
	~BooleanLit();
private:
	bool truth_;
};

///////////////////////////// BooleanLit /////////////////////////////

inline BooleanLit::BooleanLit(const Loc &loc, bool truth) : Lit(loc), truth_(truth) { }
inline void BooleanLit::normalize(Grounder *, Expander *) { }
inline bool BooleanLit::fact() const { return truth_; }
inline void BooleanLit::accept(Printer *) { }
inline Index *BooleanLit::index(Grounder *, Formula *, VarSet &) { return new MatchIndex(this); }
inline void BooleanLit::visit(PrgVisitor *) { }
inline bool BooleanLit::match(Grounder *) { return truth_; }
inline void BooleanLit::print(Storage *, std::ostream &out) const { out << truth_ ? "#true" : "#false"; }
inline BooleanLit *BooleanLit::clone() const { return new BooleanLit(*this); }
inline BooleanLit::~BooleanLit() { }
