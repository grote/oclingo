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
#include <gringo/lit.h>

class Compute : public SimpleStatement
{
private:
	class Head;
	
public:
	class Printer : public ::Printer
	{
	public:
		void print(PredLitRep *l) { (void)l; assert(false); }
		virtual ~Printer() { }
	};
public:
	Compute(const Loc &loc, PredLit *head, LitPtrVec &body);
	void append(Lit *lit);
	bool grounded(Grounder *g);
	void normalize(Grounder *g);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	~Compute();
private:
	void expandHead(Grounder *g, Lit *lit, Lit::ExpansionType type);
private:
	clone_ptr<Head> head_;
	LitPtrVec       body_;
};

