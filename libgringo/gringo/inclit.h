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

struct IncConfig
{
	int  incStep;
	int  maxVolStep;

	IncConfig()
		: incStep(std::numeric_limits<int>::min()),
		  maxVolStep(1)
	{ }
};

class IncLit : public Lit
{
public:
	class Printer : public ::Printer
	{
	public:
		using ::Printer::print;
		virtual void print(int vol_window) { (void) vol_window; }
		virtual ~Printer() { }
	};
	enum Type { CUMULATIVE, VOLATILE };

public:
	IncLit(const Loc &loc, IncConfig &config_, Type type, uint32_t varId, int vol_window=1);
	void normalize(Grounder *g, const Expander &e);
	bool fact() const { return true; }
	bool forcePrint() { return type_ == VOLATILE; }
	void accept(::Printer *v);
	Index *index(Grounder *g, Formula *gr, VarSet &bound);
	void visit(PrgVisitor *visitor);
	bool match(Grounder *grounder);
	void print(Storage *sto, std::ostream &out) const;
	Score score(Grounder *g, VarSet &bound);
	Lit *clone() const;
	const IncConfig &config() const { return config_; }
	const VarTerm *var() const { return var_.get(); }
	Type type() const { return type_; }
	~IncLit();
private:
	IncConfig         &config_;
	Type               type_;
	clone_ptr<VarTerm> var_;
	int                vol_window_;
};
