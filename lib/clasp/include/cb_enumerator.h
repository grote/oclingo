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
#ifndef CLASP_CB_ENUMERATOR_H
#define CLASP_CB_ENUMERATOR_H

#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/include/solve_algorithms.h>
#include <clasp/include/program_builder.h>

namespace Clasp { 
//! Enumerator for computing the brave/cautious consequences of a logic program
class CBConsequences : public Enumerator {
public:
	enum Consequences_t { brave_consequences, cautious_consequences };
	//! Interface for printing brave/cautious consequences
	class CBPrinter {
	public:
		CBPrinter();
		virtual ~CBPrinter();
		//! Print the current consequences
		/*!
		 * Note: The current consequences are those literals from index whose watch-flags are set.
		 */
		virtual void printConsequences(const Solver& s, const CBConsequences& enumerator) = 0;
		//! Print the result of the computation
		virtual void printReport(const Solver& s, const CBConsequences& enumerator)       = 0;
	private:
		CBPrinter(const CBPrinter&);
		CBPrinter& operator=(const CBPrinter&);
	};

	/*!
	 * \param s the solver in which the program is stored.
	 * \param index the atom symbol table of the logic program
	 * \param type type of consequences to compute
	 * \param c an (optional) minimize constraint. If != NULL, c is used for optimization.
	 * \param printer the printer to use for outputting results.
	 * \param restartOnModel if true, search is restartet after each model.
	 * \note printer must point to a heap allocated object and is owned by the new CBConsequences object!
	 */
	CBConsequences(Solver& s, AtomIndex* index, Consequences_t type, MinimizeConstraint* c, CBPrinter* printer, bool restartOnModel);
	~CBConsequences();
	void    setNumModels(uint64) {}
	bool    backtrackFromModel(Solver& s);
	bool    allowRestarts()             const { return true; }
	void    endSearch(Solver& s);

	Consequences_t      cbType() const { return type_; }
	const AtomIndex&    index()  const { return *index_; }
	MinimizeConstraint* mini()   const { return mini_; }
private:
	void add(const Solver& s, Literal p);
	void updateLastModel(Solver& s);
	void updateBraveModel(Solver& s);
	void updateCautiousModel(Solver& s);
	void printMarked() const;
	typedef PodVector<Constraint*>::type  ConstraintDB;
	ConstraintDB    locked_;
	LitVec          C_;
	CBPrinter*          printer_;
	Constraint*         current_;
	AtomIndex*          index_;
	MinimizeConstraint* mini_;
	Consequences_t      type_;
	bool                restart_;
};


}
#endif
