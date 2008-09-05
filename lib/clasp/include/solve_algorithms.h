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
#ifndef CLASP_SOLVE_ALGORITHMS_H_INCLUDED
#define CLASP_SOLVE_ALGORITHMS_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <clasp/include/literal.h>

/*!
 * \file 
 * Defines top-level functions for solving csp-problems.
 */
namespace Clasp { 

class		Solver;

//! Interface for printing models
/*!
 * The base class is a "Null-Object".
 */
class ModelPrinter {
public:
	virtual ~ModelPrinter();
	virtual void printModel(const Solver&) = 0;
};

//! Interface for enumerating models
class Enumerator {
public:
	Enumerator();
	virtual ~Enumerator();
	void	setPrinter(ModelPrinter* p) { printer_ = p; }
	//! Defaults to a noop
	virtual void		updateModel(Solver& s);
	//! Defaults to return s.backtrack()
	virtual bool		backtrackFromModel(Solver& s);
	//! Defaults to return false
	virtual bool		allowRestarts()							const;
	//! Defaults to return s.stats.models
	virtual uint64	numModels(const Solver& s)	const;
	//! Defaults to a noop
	virtual void		report(const Solver& s)			const;
protected:
	ModelPrinter* printer_;
private:
	Enumerator(const Enumerator&);
	Enumerator& operator=(const Enumerator&);
};

///////////////////////////////////////////////////////////////////////////////
// Parameter-object for the solve function
// 
// Default Restart and Deletion parameters are adapted from MiniSAT
///////////////////////////////////////////////////////////////////////////////

//! Parameter-Object for configuring search-parameters
/*!
 * \ingroup solver
 */
struct SolveParams {
	//! creates a default-initialized object.
	/*!
	 * The following parameters are used:
	 * restart			: quadratic: 100*1.5^k / no restarts after first solution
	 * shuffle			: disabled
	 * deletion			: initial size: vars()/3, grow factor: 1.1, max factor: 3.0, do not reduce on restart
	 * randomization: disabled
	 * randomProp		: 0.0 (disabled)
	 * enumerator		: default
	 */
	SolveParams();
	
	void setEnumerator(Enumerator& e)			{ enumerator_ = &e; }
	
	//! sets the restart-parameters to use during search.
	/*!
	 * clasp currently supports four different restart-strategies:
	 *  - fixed-interval restarts: restart every n conflicts
	 *  - geometric-restarts: restart every n1 * n2^k conflict (k >= 0)
	 *	- inner-outer-geometric: similar to geometric but sequence is repeated once bound outer is reached. Then, outer = outer*n2
	 *  - luby's restarts: see: Luby et al. "Optimal speedup of las vegas algorithms."
	 *  .
	 * \param base		initial interval or run-length
	 * \param inc			grow factor
	 * \param outer		max restart interval, repeat sequence if reached
	 * \param localR	Use local restarts, i.e. restart if number of conflicts in *one* branch exceed threshold
	 * \param bounded	allow bounded restarts after first solution was found
	 * \note
	 *  if base is equal to 0, restarts are disabled.
	 *  if inc is equal to 0, luby-restarts are used and base is interpreted as run-length
	 */
	void setRestartParams(uint32 base, double inc, uint32 outer = 0, bool localR = false, bool bounded = false) {
		restartBounded_	= bounded;
		restartLocal_		= localR;
		restartBase_		= base;
		restartOuter_		= outer;
		if (inc >= 1.0 || inc == 0.0)	restartInc_		= inc;
	}

	//! sets the shuffle-parameters to use during search.
	/*!
	 * \param first		Shuffle program after first restarts
	 * \param next		Re-Shuffle program every next restarts
	 * \note
	 *  if first is equal to 0, shuffling is disabled.
	 */
	void setShuffleParams(uint32 first, uint32 next) {
		shuffleFirst_ = first;
		shuffleNext_	= next;
	}

	//! sets the deletion-parameters to use during search.
	/*!
	 * \param base initial size is vars()/base
	 * \param inc Grow factor applied after each restart
	 * \param maxF Max factor, i.e. stop growth if db >= maxF*vars()
	 * \param redOnRestart true if learnt db should be reduced after restart
	 * \note if base is equal to 0, the solver won't remove any learnt nogood.
	 * \note if maxF is equal to 0, growth is not limited.
	 */
	void setReduceParams(double base, double inc, double maxF, bool redOnRestart) {
		if (base >= 0)		{	reduceBase_	= base; }
		if (inc >= 1.0)		{ reduceInc_	= inc; }
		if (maxF >= 0.0)	{ reduceMaxF_ = maxF; }
		reduceOnRestart_ = redOnRestart;
	}

	//! sets the randomization-parameters to use during search.
	/*!
	 * \param runs number of initial randomized-runs
	 * \param cfl number of conflicts comprising one randomized-run
	 */
	void setRandomizeParams(uint32 runs, uint32 cfls) {
		if (runs > 0 && cfls > 0) {
			randRuns_ = runs;
			randConflicts_ = cfls;
		}
	}

	//! sets the probability with which choices are made randomly instead of with the installed heuristic.
	void setRandomPropability(double p) {
		if (p >= 0.0 && p <= 1.0) {
			randProp_ = p;
		}
	}
	// accessors
	double	reduceBase()		const { return reduceBase_; }
	double	reduceInc()			const { return reduceInc_; }
	double	reduceMax()			const { return reduceMaxF_; }
	bool		reduceRestart() const { return reduceOnRestart_; }
	uint32	restartBase()		const { return restartBase_; }
	uint32	restartOuter()	const { return restartOuter_; }
	double	restartInc()		const { return restartInc_; }
	bool		restartBounded()const { return restartBounded_; }
	bool		restartLocal()	const { return restartLocal_; }
	uint32	randRuns()			const { return randRuns_; }
	uint32	randConflicts()	const { return randConflicts_; }
	double	randomPropability() const { return randProp_; }
	
	uint32	shuffleBase() const { return shuffleFirst_; }
	uint32	shuffleNext() const { return shuffleNext_; }
	Enumerator*		enumerator()	const { return enumerator_; }
private:
	double				reduceBase_;
	double				reduceInc_;
	double				reduceMaxF_;
	double				restartInc_; 
	double				randProp_;
	Enumerator*		enumerator_;
	uint32				restartBase_;
	uint32				restartOuter_;
	uint32				randRuns_;
	uint32				randConflicts_;
	uint32				shuffleFirst_;
	uint32				shuffleNext_;
	bool					restartBounded_;	
	bool					restartLocal_;
	bool					reduceOnRestart_;
};

//! Search without assumptions
/*!
 * \ingroup solver
 * \relates Solver
 * Starts a search for at most maxAs models using the supplied parameters.
 * \param s The Solver containing the problem.
 * \param maxModels maximal number of models to search for or 0 if solve should perform an exhaustive search.
 * \param p solve parameters to use.
 * \param minCon If the problem contains minize statements a pointer to the corresponding constraint.
 *
 * \return
 *	- true: if the search stopped before the search-space was exceeded.
 *	- false: if the search-space was completely examined.
 * 
 */
bool solve(Solver& s, uint32 maxModels, const SolveParams& p);

//! Search under a set of initial assumptions 
/*!
 * \ingroup solver
 * \relates Solver
 * Starts a search for at most maxAs models using the supplied parameters.
 * The use of assumptions allows for incremental solving. Literals contained
 * in assumptions are assumed to be true during search but are undone before solve returns.
 *
 * \param s The Solver containing the problem.
 * \param assumptions list of initial unit-assumptions
 * \param maxModels maximal number of models to search for or 0 if solve should perform an exhaustive search.
 * \param p solve parameters to use.
 * \param minCon If the problem contains minize statements a pointer to the corresponding constraint.
 *
 * \return
 *	- true: if the search stopped before the search-space was exceeded.
 *	- false: if the search-space was completely examined.
 * 
 */
bool solve(Solver& s, const LitVec& assumptions, uint32 maxModels, const SolveParams& p);

}
#endif
