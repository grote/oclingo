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

class JunctionCond
{
public:
	JunctionCond(const Loc &loc, Lit *head, LitPtrVec &body);
	~JunctionCond();

private:
	clone_ptr<Lit> head_;
	LitPtrVec      body_;
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

	void normalize(Grounder *g, Expander *expander);

	bool match(Grounder *grounder);
	bool fact() const;

	void print(Storage *sto, std::ostream &out) const;

	void index(Grounder *g, Groundable *gr, VarSet &bound);
	Score score(Grounder *g, VarSet &bound);

	void visit(PrgVisitor *visitor);
	void accept(::Printer *v);

	Lit *clone() const;

	~JunctionLit();

private:
	JunctionCondVec conds_;

};
