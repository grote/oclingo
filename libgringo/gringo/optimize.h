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
#include <gringo/formula.h>
#include <gringo/predlit.h>

class OptimizeSetLit : public Lit, public Matchable
{
public:
	OptimizeSetLit(const Loc &loc, TermPtrVec &terms);

	TermPtrVec *terms();

	bool match(Grounder *g);
	bool fact() const;
	void grounded(Grounder *g);
	const ValVec &vals() const;

	void addDomain(Grounder *g, bool fact);
	Index *index(Grounder *g, Formula *gr, VarSet &bound);
	bool edbFact() const;
	void normalize(Grounder *g, Expander *e);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	void accept(Printer *v);
	Lit *clone() const;
	~OptimizeSetLit();

private:
	TermPtrVec terms_;
	ValVec     vals_;
};

class Optimize : public SimpleStatement
{
public:
	class Printer : public ::Printer
	{
	public:
		virtual void begin(bool maximize) = 0;
		virtual void print(const ValVec &set) = 0;
		virtual void end() = 0;
		virtual ~Printer() { }
	};

public:
	Optimize(const Loc &loc, TermPtrVec &terms, LitPtrVec &body, bool maximize, bool set, bool headLike);
	Optimize(const Optimize &opt, Lit *head);
	Optimize(const Optimize &opt, const OptimizeSetLit &setLit);
	void normalize(Grounder *g);
	void append(Lit *lit);
	bool grounded(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	~Optimize();

private:
	OptimizeSetLit setLit_;
	LitPtrVec      body_;
	bool           maximize_;
	bool           set_;
	bool           headLike_;
};
