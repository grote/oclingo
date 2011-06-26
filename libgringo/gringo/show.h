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
#include <gringo/index.h>
#include <gringo/lit.h>

class DisplayHeadLit : public Lit, public Matchable
{
public:
	DisplayHeadLit(const Loc &loc, Term *term);

	DisplayHeadLit *clone() const;
	void normalize(Grounder *g, const Expander &e);
	bool fact() const;
	void print(Storage *sto, std::ostream &out) const;
	Index *index(Grounder *g, Formula *gr, VarSet &bound);
	void visit(PrgVisitor *visitor);
	void accept(Printer *v);

	bool match(Grounder *grounder);

	Val val(Grounder *g);
	std::auto_ptr<PredLit> toPred(Grounder *g);

private:
	clone_ptr<Term> term_;
};

class Display : public SimpleStatement
{
public:
	enum Type { SHOWPRED, HIDEPRED, SHOWTERM };
	class Printer : public ::Printer
	{
	public:
		typedef Display::Type Type;
	public:
		virtual void begin(const Val &head, Type type) = 0;
		virtual void end() = 0;
		virtual ~Printer() { }
	};
public:
	Display(const Loc &loc, Term *term, LitPtrVec &body, Type type);
	Display(const Loc &loc, DisplayHeadLit *lit, LitPtrVec &body, Type type);
	void append(Lit *lit);
	bool grounded(Grounder *g);
	void normalize(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	~Display();
private:
	void expandHead(Grounder *g, Lit *lit, Lit::ExpansionType type);

private:
	clone_ptr<Lit> head_;
	LitPtrVec      body_;
	Type           type_;
};
