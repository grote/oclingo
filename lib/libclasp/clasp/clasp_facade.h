// 
// Copyright (c) 2006-2009, Benjamin Kaufmann
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
#ifndef CLASP_CLASP_FACADE_H_INCLUDED
#define CLASP_CLASP_FACADE_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif
#include <clasp/literal.h>
#include <clasp/solve_algorithms.h>
#include <clasp/solver.h>
#include <clasp/heuristics.h>
#include <clasp/lookahead.h>
#include <clasp/program_builder.h>
#include <clasp/unfounded_check.h>
#include <clasp/reader.h>
#include <clasp/util/misc_types.h>
#include <string>

/*!
 * \file 
 * This file provides a facade around the clasp library. 
 * I.e. a simplified interface for (incrementally) solving a problem using
 * some configuration (set of parameters).
 */
namespace Clasp {

/////////////////////////////////////////////////////////////////////////////////////////
// Parameter configuration
/////////////////////////////////////////////////////////////////////////////////////////
//! Options for configuaring the ProgramBuilder
struct ApiOptions {
	ApiOptions();
	ProgramBuilder* createApi(AtomIndex& index);
	typedef ProgramBuilder::ExtendedRuleMode ExtRuleMode;
	typedef DefaultUnfoundedCheck::ReasonStrategy LoopMode;
	ExtRuleMode transExt; /**< handling of extended rules	*/
	LoopMode    loopRep;  /**< how to represent loops? */
	int  eq;              /**< iterations of equivalence preprocessing */
	bool backprop;        /**< enable backpropagation in eq-preprocessing? */
	bool supported;       /**< compute supported models? */
};

//! Options that control the enumeration algorithm
struct EnumerateOptions {
	EnumerateOptions();
	Enumerator* createEnumerator() const;
	bool consequences() const { return brave || cautious; }
	const char* cbType()const {
		if (consequences()) { return brave ? "Brave" : "Cautious"; }
		return "none";
	}
	int  numModels;       /**< number of models to compute */
	int  projectOpts;     /**< options for projection */
	struct Optimize {     /**< Optimization options */
		Optimize() : no(false), all(false), heu(false) {}
		WeightVec vals;     /**< initial values for optimize statements */
		bool   no;          /**< ignore optimize statements */
		bool   all;         /**< compute all optimal models */
		bool   heu;         /**< consider optimize statements in heuristics */
	}    opt;
	bool project;         /**< enable projection */
	bool record;          /**< enable solution recording */
	bool restartOnModel;  /**< restart after each model */
	bool brave;           /**< enable brave reasoning */
	bool cautious;        /**< enable cautious reasoning */
};
//! Options that control the decision heuristic
struct HeuristicOptions {
	HeuristicOptions();
	typedef Lookahead::Type LookaheadType;
	DecisionHeuristic* createHeuristic() const;
	std::string   heuristic;   /**< decision heuristic */
	LookaheadType lookahead;   /**< type of lookahead */
	int           lookaheadNum;/**< number of times lookahead is used */
	int           loops;       /**< consider loops in heuristic (0: no, 1: yes, -1: let heuristic decide) */
	union Extra {
	int      berkMax;          /**< only for Berkmin */
	int      vmtfMtf;          /**< only for Vmtf    */
	} extra;
	bool     berkMoms;         /**< only for Berkmin */
	bool     berkHuang;	       /**< only for Berkmin */
	bool     nant;             /**< only for unit    */
};

struct IncrementalConfig {
	IncrementalConfig() : minSteps(1), maxSteps(uint32(-1)), stopUnsat(false), keepLearnt(true), keepHeuristic(false) {}
	uint32 minSteps;      /**< Perform at least minSteps incremental steps */
	uint32 maxSteps;      /**< Perform at most maxSteps incremental steps */
	bool   stopUnsat;     /**< Stop on first unsat problem? */
	bool   keepLearnt;    /**< Keep learnt nogoods between incremental steps? */
	bool   keepHeuristic; /**< Keep heuristic values between incremental steps? */
};

//! Parameter-object that groups & validates options
class ClaspConfig {
public:
	explicit ClaspConfig(Solver* s = 0) : solver(s) {}
	bool validate(std::string& err);
	ApiOptions       api;
	EnumerateOptions enumerate;
	SolveParams      solve;
	HeuristicOptions heuristic;
	Solver*          solver;
	bool             onlyPre;    /**< stop after preprocessing step? */
private:
	ClaspConfig(const ClaspConfig&);
	ClaspConfig& operator=(const ClaspConfig&);
};
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspFacade
/////////////////////////////////////////////////////////////////////////////////////////
//! Provides a simplified interface for (incrementally) solving a given problem
class ClaspFacade : public Enumerator::Report {
public:
	//! Defines the possible solving states
	enum State { 
		state_not_started,
		state_read,        /*!< problem is read from input */
		state_preprocess,  /*!< problem is prepared */
		state_solve,       /*!< search is active */
		num_states
	};
	//! Defines important event types
	enum Event { 
		event_state_enter, /*!< a new state was entered */
		event_state_exit,  /*!< about to exit from the active state */
		event_p_prepared,  /*!< problem was transformed to nogoods */
		event_restart,     /*!< search restarted */
		event_model        /*!< a model was found */
	};
	//! Defines possible solving results
	enum Result { result_unsat, result_sat, result_unknown }; 
	//! Callback interface for notifying about important steps in solve process
	class Callback {
	public:
		virtual ~Callback() {}
		//! State transition. Called on entering/exiting a state
		/*!
		 * \param e is either event_state_enter or event_state_exit
		 * \note f.state() returns the active state
		 */
		virtual void state(Event e, ClaspFacade& f) = 0;
		//! Some operation triggered an important event
		/*!
		 * \param e an event that is neither event_state_enter nor event_state_exit 
		 */
		virtual void event(Event e, ClaspFacade& f)          = 0;
		//! Some configuration option is unsafe/unreasonable w.r.t the current problem
		virtual void warning(const char* msg)                = 0;
	};
#if defined(PRINT_SEARCH_PROGRESS) && PRINT_SEARCH_PROGRESS == 1
	struct SearchLimits {
		SearchLimits() : conflicts(0), learnts(0) {}
		uint64 conflicts;
		uint32 learnts;
	};
#endif
	ClaspFacade();
	/*!
	 * Solves the problem given in problem using the given configuration.
	 * \pre config is valid, i.e. config.valid() returned true
	 * \note Once solve() returned, the result of the computation can be
	 * queried via the function result().
	 * \note if config.onlyPre is true, solve() returns after
	 * the preprocessing step (i.e. once the solver is prepared) and does not start a search.
	 */
	void solve(Input& problem, ClaspConfig& config, Callback* c);

	/*!
	 * Incrementally solves the problem given in problem using the given configuration.
	 * \pre config is valid, i.e. config.valid() returned true
	 * \note Once solve() returned, the result of the computation can be
	 * queried via the function result().
	 * \note config.onlyPre is ignored in incremental setting!
	 */
	void solveIncremental(Input& problem, ClaspConfig& config, IncrementalConfig& inc, Callback* c);

	//! returns the result of a computation
	Result result() const { return result_; }
	//! returns true if search-space was completed. Otherwise false.
	bool   more()   const { return more_; }
	//! returns the active state
	State  state()  const { return state_; }
	//! returns the current incremental step (starts at 0)
	int    step()   const { return step_; }
	//! returns the current input problem
	Input* input() const { return input_; }
	//! returns the ProgramBuilder-object that was used to transform a logic program into nogoods
	/*!
	 * \note A ProgramBuilder-object is only created if input()->format() == Input::SMODELS
	 * \note The ProgramBuilder-object is destroyed after the event
	 *       event_p_prepared was fired. Call releaseApi to disable auto-deletion of api.
	 *       In that case you must later manually delete it!
	 */
	ProgramBuilder* api() const  { return api_.get();     }
	ProgramBuilder* releaseApi() { return api_.release(); }

#if defined(PRINT_SEARCH_PROGRESS) && PRINT_SEARCH_PROGRESS == 1
	const SearchLimits& limits() const { return limits_; }
#endif
private:
	ClaspFacade(const ClaspFacade&);
	ClaspFacade& operator=(const ClaspFacade&);
	typedef SingleOwnerPtr<ProgramBuilder> Api;
	// -------------------------------------------------------------------------------------------  
	// Status information
	void setState(State s, Event e)   { state_ = s; if (cb_) cb_->state(e, *this); }
	void fireEvent(Event e)           { if (cb_) cb_->event(e, *this); }
	void warning(const char* w) const { if (cb_) cb_->warning(w); }
	// -------------------------------------------------------------------------------------------
	// Enumerator::Report interface
	void reportModel(const Solver&, const Enumerator&) {
		result_ = result_sat;
		fireEvent(event_model);
	}
	void reportSolution(const Solver& s, const Enumerator&, bool complete) {
		more_ = !complete;
		if (!more_ && s.stats.solve.models == 0) {
			result_ = result_unsat;
		}
		setState(state_, event_state_exit);
	}
	void reportStatus(const Solver&, uint64 maxCfl, uint32 maxL) {
		(void)maxCfl; (void)maxL;
#if defined(PRINT_SEARCH_PROGRESS) && PRINT_SEARCH_PROGRESS == 1
		limits_.conflicts = maxCfl;
		limits_.learnts   = maxL;
		fireEvent(event_restart);
#endif
	}
	// -------------------------------------------------------------------------------------------
	// Internal setup functions
	void   validateWeak();
	void   init(Input&, ClaspConfig&, IncrementalConfig*, Callback* c);
	bool   read();
	bool   preprocess();
	uint32 computeProblemSize() const;
	void   configureMinimize(MinimizeConstraint* min)  const;
	void   initEnumerator()     const;
	// -------------------------------------------------------------------------------------------
#if defined(PRINT_SEARCH_PROGRESS) && PRINT_SEARCH_PROGRESS == 1
	SearchLimits  limits_;
#endif
	ClaspConfig*       config_;
	IncrementalConfig* inc_;
	Callback*          cb_;
	Input*             input_;
	Api                api_;
	Result             result_;
	State              state_;
	int                step_;
	bool               more_;
};

}
#endif
