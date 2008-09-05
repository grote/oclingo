// 
// Copyright (c) 2006-2007, Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#ifndef CLASP_ENUMERATOR_H_INCLUDED
#define CLASP_ENUMERATOR_H_INCLUDED

#include <clasp/include/solve_algorithms.h>
#include <clasp/include/smodels_constraints.h>
#include <clasp/include/program_builder.h>
#include <clasp/include/clause.h>

#include <iosfwd>

namespace Clasp {
	
class StdOutPrinter : public ModelPrinter {
public:
	StdOutPrinter();
	void printModel(const Solver& s);
	const AtomIndex*	index;
};

class MinimizeEnumerator : public Enumerator {
public:
	MinimizeEnumerator(MinimizeConstraint* c) : mini_(c) {}
	~MinimizeEnumerator();
	void		updateModel(Solver& s);
	bool		backtrackFromModel(Solver& s)			{ return mini_->backtrackFromModel(s); }
	uint64	numModels(const Solver&)		const {	return mini_->models(); }
	bool		allowRestarts()							const { return mini_->allowRestart(); }
private:
	MinimizeConstraint* mini_;
};

class CBConsequences : public Enumerator {
public:
	enum Consequences_t { brave_consequences, cautious_consequences };
	CBConsequences(Solver& s, AtomIndex* index, Consequences_t type, bool quiet);
	~CBConsequences();
	void		updateModel(Solver& s);
	bool		backtrackFromModel(Solver& s);
	bool		allowRestarts()							const { return true; }
	uint64	numModels(const Solver&)		const {	return 1; }
	void		report(const Solver& s)			const;
private:
	void add(const Solver& s, Literal p);
	void updateBraveModel(Solver& s);
	void updateCautiousModel(Solver& s);
	void printMarked() const;
	typedef PodVector<Clause*>::type	ConstraintDB;
	ConstraintDB		locked_;
	LitVec					C_;
	Clause*					current_;
	AtomIndex*			index_;
	Consequences_t	type_;
	bool						quiet_;
};


}
#endif
