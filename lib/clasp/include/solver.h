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
#ifndef CLASP_SOLVER_H_INCLUDED
#define CLASP_SOLVER_H_INCLUDED
#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/include/solver_types.h>
#include <string>
#include <utility>

namespace Clasp { 

///////////////////////////////////////////////////////////////////////////////
// classes used by the Solver
///////////////////////////////////////////////////////////////////////////////
class DecisionHeuristic;
class PostPropagator;
class ClauseCreator;

//! Base class for preprocessors working on clauses only
class SatPreprocessor {
public:
	SatPreprocessor(Solver& s) { setSolver(s); }
	virtual ~SatPreprocessor();
	void setSolver(Solver& s) { solver_ = &s; }
	virtual bool addClause(const LitVec& cl) = 0;
	virtual bool preprocess() = 0;
	virtual void extendModel(State& m) = 0;
	virtual bool hasSymModel() const = 0;
protected:
	Solver* solver_;
private:
	SatPreprocessor(const SatPreprocessor&);
	SatPreprocessor& operator=(const SatPreprocessor&);
};
///////////////////////////////////////////////////////////////////////////////

/**
 * \defgroup solver Solver
 */
//@{

//! Parameter-Object for configuring a solver.
struct SolverStrategies {
	typedef std::auto_ptr<DecisionHeuristic>	Heuristic;
	typedef std::auto_ptr<PostPropagator>			PostProp;
	typedef std::auto_ptr<SatPreprocessor>		SatPrePro;
	//! Clasp's two general search strategies
	enum SearchStrategy {
		use_learning, /*!< Analyze conflicts and learn First-1-UIP-clause */
		no_learning		/*!< Don't analyze conflicts - chronological backtracking */
	};
	
	//! Strategy to use during conflict clause minimization
	enum CflMinStrategy {
		beame_minimization		= 1,	/*!< minimization as described in P. Beame et al: "Understanding the power of clause learning"		*/
		een_minimization			= 2		/*!< minimization as described in N. Eén et al: "A SAT Solver with conflict-clause minimization"	*/
	};
	//! Antecedents to consider during conflict clause minimization
	enum CflMinAntes {
		no_antes							= 0,	/*!< Don't minimize first-uip-clauses */
		binary_antes					= 2,	/*!< Consider only binary antecedents					*/
		binary_ternary_antes	= 6,	/*!< Consider binary- and ternary antecedents */
		all_antes							= 7		/*!< Consider all antecedents									*/
	};

	//! creates a default-initialized object.
	/*!
	 * The following parameters are used:
	 *	- satPrePro				: none (no sat based preprocessing)
	 *  - heuristic				:	none (choose the first free literal)
	 *	-	postProp				: none (compute supported models)
	 *  - search					: use_learning
	 *  - cflMin					: beame_minimization
	 *	- cflMinAntes			: all_antes
	 *  - compression			: 60
	 *	- randomWatches		: false
	 *	- saveProgress		: false
	 *  .
	 */
	SolverStrategies();
	
	SatPrePro						satPrePro;
	Heuristic						heuristic;
	PostProp						postProp;
	SearchStrategy			search;
	CflMinStrategy			cflMin;
	CflMinAntes					cflMinAntes;
	bool								randomWatches;
	bool								saveProgress;
	void setCompressionStrategy(uint32 len) {
		compress_ = len == 0 ? static_cast<uint32>(-1) : len;
	}
	uint32							compress()		const { return compress_; }
	
	void move(SolverStrategies& o) {
		satPrePro.reset( o.satPrePro.release() );
		heuristic.reset( o.heuristic.release() );
		postProp.reset( o.postProp.release() );
		search				= o.search;
		cflMin				= o.cflMin;
		compress_			= o.compress_;
		randomWatches = o.randomWatches;
		saveProgress	= o.saveProgress;
	}
	// for testing
	PostPropagator* releasePostProp();
private:
	SolverStrategies(const SolverStrategies&);
	SolverStrategies& operator=(const SolverStrategies&);
	uint32							compress_;
};

//! clasp's Solver class
/*!
 * A Solver-object maintains the state and provides the functions 
 * necessary to implement the Conflict-Driven Nogood-Learning DPLL-algorithm. 
 *
 * The interface supports incremental solving (search under assumptions) as well as
 * solution enumeration. To make this possible the solver maintains two special  
 * decision levels: the root-level and the backtrack-level.
 *
 * The root-level is the lowest decision level to which a search
 * can return. Conflicts on the root-level are non-resolvable and end a search - the
 * root-level therefore acts as an artificial top-level during search.
 * Incremental solving is then implemented by first adding a list of unit assumptions
 * and second initializing the root-level to the current decision level.
 * Once search terminates assumptions can be undone by calling clearAssumptions
 * and a new a search can be started using different assumptions.
 *
 * For model enumeration the solver maintains a backtrack-level which restricts
 * backjumping in order to prevent repeating already enumerated solutions.
 * The solver will never backjump above that level and conflicts on the backtrack-level 
 * are resolved by backtracking, i.e. flipping the corresponding decision literal.
 * To make model enumeration possible call backtrack whenever a model was found.
 * \see "Conflict-Driven Answer Set Enumeration" for a detailed description of this approach. 
 *
 */
class Solver {
	friend class PostPropagator;
public:
	/*!
	 * \name construction/destruction/setup
	 */
	//@{
	//! creates an empty solver object with all strategies set to their default value.
	Solver();
	
	//! destroys the solver object and all contained constraints.
	~Solver();
	
	//! resets a solver object to the state it had after default-construction.
	void reset();

	//! shuffle constraints upon next simplification
	void shuffleOnNextSimplify() { shuffle_ = true; }
	
	//! initializes the strategies used by this solver-object.
	void setStrategies(SolverStrategies& st) { strategy_.move(st); }
	
	//! returns the strategies used by this solver-object. 
	SolverStrategies& strategies() { return strategy_; }

	/*!
	 * \overload SolverStrategies& Solver::strategies()
	 */
	const SolverStrategies& strategies() const { return strategy_; }
	//@}

	/*!
	 * \name Problem specification
	 * Functions for adding variables and constraints.
	 * \note Problem specification is a three-stage process:
	 * - First all variables must be added.
	 * - Second startAddConstraints must be called and constraints can be added.
	 * - Finally endAddConstraints must be called to finished problem-specification.
	 * .
	 * \note Constraints can only be added in stage two. Afterwards only learnt constraints
	 * may be added to the solver.
	 * \note This process can be repeated, i.e. if incremental solving is used,
	 * one can add new variables and problem constraints by calling
	 * addVar/startAddConstraints/endAddConstraints again after a search terminated.
	 */
	//@{
	
	//! reserve space for at least numVars variables
	/*!
	 * \note if the number of variables is known upfront, calling this method
	 * avoids repeated regrowing of the state data structure
	 */
	void reserveVars(uint32 numVars) { vars_.reserve(numVars); }
	
	//! adds a new variable of type t to the solver.
	/*!
	 * \pre startAddConstraints was not yet called.
	 * \param t type of the new variable
	 * \return The index of the new variable
	 * \note The problem variables are numbered from 1 onwards!
	 */
	Var addVar(VarType t);
	
	//! returns the type of variable v.
	VarType type(Var v) const {
		assert(validVar(v));
		return vars_[v].eq != 0 
			? Var_t::atom_body_var
			: VarType(Var_t::atom_var + vars_[v].body);
	}
	
	//! changes the type of variable v to new type
	void changeVarType(Var v, VarType t) {
		assert(validVar(v));
		vars_[v].eq			= (t == Var_t::atom_body_var);
		if (vars_[v].eq == 0) {
			vars_[v].body		= (t == Var_t::body_var);
		}
	}

	//! returns the preferred decision literal of variable v w.r.t its type
	/*!
	 * \return 
	 *	- posLit(v) if type(v) == body_var
	 *	- negLit(v) if type(v) == atom_var
	 *	- posLit(v) if type(v) changed from body_var to atom_body_var
	 *	- negLit(v) if type(v) changed from atom_var to atom_body_var
	 */
	Literal preferredLiteralByType(Var v) const {
		assert(validVar(v));
		return Literal(v, vars_[v].body == 0);
	}
	
	//! sets the preferred value of variable v
	/*!
	 * \param v the variable for which a value should be set
	 * \param value the preferred value
	 */
	void setPreferredValue(Var v, ValueRep val) {
		assert(validVar(v));
		vars_[v].pfVal = val;
	}
	//! returns the preferred value of variable v.
	ValueRep preferredValue(Var v) const {
		assert(validVar(v));
		return static_cast<ValueRep>(vars_[v].pfVal);
	}

	//! returns true if v is currently eliminated, i.e. no longer part of the problem
	bool eliminated(Var v)	const			{ assert(validVar(v)); return vars_[v].elim != 0; }
	//! returns true if v is excluded from variable elimination
	bool frozen(Var v)			const			{ assert(validVar(v)); return vars_[v].frozen != 0; }
	
	//! eliminates/ressurrects the variable v
	/*!
	 * \pre if elim is true: v must not occur in any constraint 
	 *	and frozen(v) == false and value(v) == value_free
	 */
	void eliminate(Var v, bool elim);
	void setFrozen(Var v, bool b)			{ assert(validVar(v)); vars_[v].frozen = (uint32)b; }

	//! returns true if var represents a valid variable in this solver.
	/*!
	 * \note The range of valid variables is [1..numVars()]. The variable 0
	 * is a special sentinel variable that is always true. 
	 */
	bool validVar(Var var) const { return var <= numVars(); }

	//! Must be called once before constraints can be added.
	void startAddConstraints();
	
	//! adds the constraint c to the solver.
	/*!
	 * \pre endAddConstraints was not called.
	 */
	void add(Constraint* c) { constraints_.push_back(c); }

	//! adds the unary constraint p to the solver.
	/*!
	 * \note unary constraints are immediately asserted.
	 * \return false if asserting p leads to a conflict.
	 */
	bool addUnary(Literal p);

	//! adds the binary constraint (p, q) to the solver.
	/*!
	 * \pre if asserting is true, q must be false w.r.t the current assignment
	 * \return !asserting || force(p, Antecedent(~q));
	 */
	bool addBinary(Literal p, Literal q, bool asserting);
	
	//! adds the ternary constraint (p, q, r) to the solver.
	/*!
	 * \pre if asserting is true, q and r must be false w.r.t the current assignment
	 * \return !asserting || force(p, Antecedent(~q, ~r));
	 */
	bool addTernary(Literal p, Literal q, Literal r, bool asserting);

	//! adds the learnt constraint c to the solver.
	/*!
	 * \pre endAddConstraints was called.
	 */
	void addLearnt(LearntConstraint* c) {
		learnts_.push_back(c);
		updateLearnt(stats, c->size(), c->type()); 
	}

	//! initializes the solver
	/*!
	 * endAddConstraints must be called once before a search is started. After endAddConstraints
	 * was called learnt constraints may be added to the solver.
	 * \param lookahead true if solve should simplify the problem using a 
	 * one-step lookahead-operation.
	 * \return 
	 *	- false if the constraints are initially conflicting. True otherwise.
	 * \note
	 * The solver can't recover from top-level conflicts, i.e. if endAddConstraints
	 * returned false, the solver is in an unusable state.
	 */
	bool endAddConstraints(bool lookahead = false);
	//@}
	/*!
	 * \name watch management
	 * Functions for setting/removing watches.
	 * \pre validVar(v)
	 */
	//@{
	//! returns the number of constraints watching the literal p
	uint32 numWatches(Literal p) const;
	
	//! returns true if the constraint c watches the literal p
	bool		hasWatch(Literal p, Constraint* c) const;
	//! returns c's watch-structure associated with p
	/*!
	 * \note returns 0, if hasWatch(p, c) == false
	 */
	Watch*	getWatch(Literal p, Constraint* c) const;

	//! Adds c to the watch-list of p.
	/*!
	 * When p becomes true, c->propagate(p, data, *this) is called.
	 * \post hasWatch(p, c) == true
	 */
	void addWatch(Literal p, Constraint* c, uint32 data = 0) {
		assert(validWatch(p));
		gwl(watches_[p.index()]).push_back(Watch(c, data));
	}
	
	//! removes c from p's watch-list.
	/*!
	 * \post hasWatch(p, c) == false
	 */
	void removeWatch(const Literal& p, Constraint* c);

	//! adds c to the watch-list of decision-level dl
	/*!
	 * Constraints in the watch-list of a decision level are
	 * notified when that decision level is about to be backtracked.
	 * \pre dl != 0 && dl <= decisionLevel()
	 */
	void addUndoWatch(uint32 dl, Constraint* c) {
		assert(dl != 0 && dl <= decisionLevel() );
		levels_[dl-1].second.push_back(c);
	}
	
	//! removes c from the watch-list of the decision level dl
	void removeUndoWatch(uint32 dl, Constraint* c);

	//@}

	/*!
	 * \name state inspection
	 * Functions for inspecting the state of the search.
	 * \note validVar(v) is a precondition for all functions that take a variable as 
	 * parameter.
	 */
	//@{
	//! returns the number of problem variables.
	/*!
	 * \note The special sentine-var 0 is not counted, i.e. numVars() returns
	 * the number of problem-variables.
	 * To iterate all problem-variables use a loop like:
	 * \code
	 * for (Var i = 1; i <= s.numVars(); ++i) {...}
	 * \endcode
	 */
	uint32 numVars() const { return (uint32)vars_.size() - 1; }
	//! returns the number of variables in the current assignment.
	/*!
	 * \note The special sentinel-var 0 is not counted.
	 */
	uint32 numAssignedVars()	const { return (uint32)trail_.size(); }
	
	//! returns the number of free variables.
	/*!
	 * The number of free variables is the number of vars that are neither
	 * assigned nor eliminated.
	 */
	uint32 numFreeVars()			const { return numVars()-((uint32)trail_.size()+eliminated_); }

	//! returns the number of eliminated vars
	uint32 numEliminatedVars()const	{ return eliminated_; }

	//! returns the number of initial problem constraints stored in the solver
	uint32 numConstraints()		const { return (uint32)constraints_.size() + binCons_ + ternCons_; }
	//! returns the number of binary constraints stored in the solver
	uint32 numBinaryConstraints() const { return binCons_; }
	//! returns the number of ternary constraints stored in the solver
	uint32 numTernaryConstraints() const { return ternCons_; }
	
	//! returns the number of constraints that are currently in the learnt database.
	/*!
	 * \note binary and ternary constraints are not counted as learnt constraint.
	 */
	uint32 numLearntConstraints() const { return (uint32)learnts_.size(); }
	
	//! returns the value of v w.r.t the current assignment
	ValueRep value(Var v) const {
		assert(validVar(v));
		return vars_[v].value;
	}
	//! returns true if p is true w.r.t the current assignment
	bool isTrue(Literal p) const {
		return value(p.var()) == trueValue(p);
	}
	//! returns true if p is false w.r.t the current assignment
	bool isFalse(Literal p) const {
		return value(p.var()) == falseValue(p);
	}
	//! returns the decision level on which v was assigned.
	/*!
	 * \note the returned value is only meaningful if value(v) != value_free
	 */
	uint32 level(Var v) const {
		assert(validVar(v));
		return vars_[v].level;
	}

	//! returns the reason why v is in the current assignment
	/*!
	 * \note the returned value is only meaningful if value(v) != value_free
	 * \note if solver is used in no-learning mode, the reason is always null
	 */
	const Antecedent& reason(Var v) const {
		assert(validVar(v));
		return vars_[v].reason;
	}

	//! returns true if v was assigned on level 0 or is currently analyzed by analyzeConflict 
	bool seen(Var v) const {
		assert(validVar(v));
		return vars_[v].seen != 0;
	}

	//! returns the current decision level.
	uint32 decisionLevel() const { return (uint32)levels_.size(); }

	//! returns the decision literal of the decision level dl.
	/*!
	 * \pre dl != 0 && dl <= decisionLevel()
	 */
	Literal decision(uint32 dl) const {
		assert(dl != 0 && dl <= decisionLevel() );
		return trail_[ levels_[dl-1].first ];
	}
	
	//! returns the current assignment
	/*!
	 * \note although the special var 0 always has a value it is not considered to be
	 * part of the assignment.
	 */
	const LitVec& assignment() const { return trail_; }
	
	//! returns true, if the current assignment is conflicting
	bool hasConflict() const { return !conflict_.empty(); }
	
	//! returns the current conflict as a set of literals
	const LitVec& conflict() const { return conflict_; }
	
	SolverStatistics	stats;	/**< stores some statistics about the solving process */
	//@}

	/*!
	 * \name Basic DPLL-functions
	 */
	//@{
	//! initializes the root-level to dl
	/*!
	 * The root-level is similar to the top-level in that it can not be
	 * undone during search, i.e. the solver will not resolve conflicts that are on or
	 * above the root-level. 
	 */
	void setRootLevel(uint32 dl) { 
		rootLevel_	= std::min(decisionLevel(), dl); 
		btLevel_		= std::max(btLevel_, rootLevel_);
	}

	//! returns the current root level
	uint32 rootLevel() const { return rootLevel_; }

	//! removes implications made between the top-level and the root-level.
	/*!
	 * The function also resets the current backtrack-level and re-asserts learnt
	 * facts.
	 */
	bool clearAssumptions() {
		rootLevel_ = btLevel_ = 0;
		undoUntil(0);
		assert(decisionLevel() == 0);
		for (ImpliedLits::size_type i = 0; i < impliedLits_.size(); ++i) {
			if (impliedLits_[i].level == 0) {
				force(impliedLits_[i].lit, impliedLits_[i].ante);
			}
		}
		impliedLits_.clear();
		return simplify();
	}

	//! If called on top-level removes SAT-clauses + Constraints for which Constraint::simplify returned true
	/*!
	 * \note if this method is called on a decision-level > 0 it is a noop and will
	 * simply return true.
	 * \return If a top-level conflict is detected false, otherwise true.
	 */
	bool simplify();

	//! sets the literal p to true and schedules p for propagation.
	/*!
	 * Setting a literal p to true means assigning the appropriate value to
	 * p's variable. That is: value_false if p is a negative literal and value_true
	 * if p is a positive literal.
	 * \param p the literal that should become true
	 * \param a the reason for the literal to become true or 0 if no reason exists.
	 * 
	 * \return
	 *  - false if p is already false
	 *  - otherwise true.
	 *
	 * \pre hasConflict() == false
	 * \pre a.isNull() == false || decisionLevel() <= rootLevel() || SearchStrategy == no_learning
	 * \post
	 *	p.var() == trueValue(p) || p.var() == falseValue(p) && hasConflict() == true
	 *
	 * \note if setting p to true leads to a conflict, the nogood that caused the
	 * conflict can be requested using the conflict() function.
	 */
	bool force(const Literal& p, const Antecedent& a);

	//! assumes the literal p.
	/*!
	 * Sets the literal p to true and starts a new decision level.
	 * \pre value(p) == value_free && validVar(p.var()) == true
	 * \param p the literal to assume
	 * \return always true
	 * \post if decisionLevel() was n before the call then decisionLevel() is n + 1 afterwards.
	 */
	bool assume(const Literal& p);

	//! selects and assumes the next branching literal by calling the installed decision heuristic.
	/*!
	 * \pre queueSize() == 0
	 * \note the next decision literal will be selected randomly with a
	 * propability set using the initRandomHeuristic method.
	 * \return 
	 *	- true if the assignment is not total and a literal was assumed (or forced).
	 *	- false otherwise
	 *  .
	 * \see DecisionHeuristic
	 */
	bool decideNextBranch();

	//! Marks the literals in cfl as conflicting.
	/*! 
	 * \pre !cfl.empty() && !hasConflict()
	 * \post cfl.empty() && hasConflict()
	 */
	void setConflict(LitVec& cfl) { conflict_.swap(cfl); }

	/*!
	 * propagates all enqueued literals. If a conflict arises during propagation
	 * propagate returns false and the current conflict (as a set of literals)
	 * is stored in the solver's conflict variable.
	 * \pre !hasConflict()
	 * \see Solver::force
	 * \see Solver::assume
	 */
	bool propagate();

	//! executes a one-step failed-literal test beginning at variable startVar.
	/*!
	 * Tests all currently unassigned variables (beginning at startVar). 
	 * If a literal fails, the complementary literal is forced and the function returns
	 * true.
	 * \pre validVar(startVar)
	 * \param startVar first variable to test.
	 * \param[out] scores scores[v] contains the score computed when v was tested.
	 * \param types var-types to consider during lookahead.
	 * \param uniform If true, test both literals of variable. Otherwise test only preferredLiteral.
	 * \param[out] deps For a variable v that was tested, deps contains v and all v' that were forced because of v.
	 * \return
	 *   - true if a failed-literal is detected
	 *   - false otherwise.
	 *   .
	 * \note Dependent variables are not tested and will therefore have a score of 0
	 */
	bool failedLiteral(Var& startVar, VarScores& scores, VarType types, bool uniform, VarVec& deps);
	
	//! executes a one-step lookahead.
	/*!
	 * Assumes p and propagates this assumption. If propagations leads to
	 * a conflict resolveConflict() is called and the functions returns false.
	 * Otherwise the assumption is undone and the function returns true.
	 * \pre p is free
	 */
	bool lookahead(Literal p, VarScores& scores, VarVec& deps, VarType types, bool addDeps);

	//! estimates the number of assignments following from setting p to true.
	/*!
	 * \note for the estimate only binary clauses are considered.
	 */	
	uint32 estimateBCP(const Literal& p, int maxRecursionDepth = 5) const;
	
	//! returns the idx'th learnt constraint
	/*!
	 * \pre idx < numLearntConstraints()
	 */
	const LearntConstraint& getLearnt(uint32 idx) const {
		assert(idx < numLearntConstraints());
		return *static_cast<LearntConstraint*>(learnts_[ idx ]);
	}
	
	//! removes upto remMax percent of the learnt nogoods.
	/*!
	 * \param remMax percantage of learnt nogoods that should be removed ([0.0f, 1.0f])
	 * \note nogoods that are the reason for a literal to be in the assignment
	 * are said to be locked and won't be removed.
	 */
	void reduceLearnts(float remMax);

	//! resolves the active conflict using the selected strategy
	/*!
	 * If the SearchStrategy is set to learning, resolveConflict implements
	 * First-UIP learning and backjumping. Otherwise it simply applies
	 * chronological backtracking.
	 * \pre hasConflict
	 * \return
	 *	- true if the conflict was successfully resolved
	 *  - false otherwise
	 * \note
	 *	if decisionLevel() == rootLevel() false is returned.
	 */
	bool resolveConflict();

	//! backtracks the last decision and sets the backtrack-level to the resulting decision level.
	/*!
	 * \return
	 *	- true if backtracking was possible
	 *  - false if decisionLevel() == rootLevel()
	 * \note
	 * If models should be enumerated this method must be called whenever
	 * a model was found.
	 */
	bool backtrack();

	//! undoes all assignments upto (but not including) decision level level.
	/*!
	 * \pre dl > 0 (assignments made on decision level 0 cannot be undone)
	 * \pre dl <= decisionLevel()
	 * \post decisionLevel == max(dl, max(rootLevel(), btLevel))
	 */
	void undoUntil(uint32 dl);
	//@}	

	
	//! searches for a model for at most maxConflict number of conflicts.
	/*!
	 * The search function implements the conflict-driven nogood-Learning DPLL-algorithm.
	 * It searches for a model for at most maxConflict number of conflicts.
	 * The number of learnt clauses is kept below maxLearnts. 
	 * \pre endAddConstraint() returned true and !hasConflict()
	 * \pre decisionLevel() == 0 || simplify was called on top-level.
	 * \param maxConflict stop search after maxConflict (-1 means: infinite)
	 * \param maxLearnts reduce learnt constraints after maxLearnts constraints have been learnt (-1 means: never reduce learnt constraints).
	 * \param randProp pick next decision variable randomly with a propability of randProp
	 * \param localR if true, stop after maxConflict in current branch
	 * \return
	 *  - value_true: if a model was found.
	 *  - value_false: if the problem is unsatisfiable (under assumptions, if any)
	 *  - value_free: if search was stopped because maxConflicts were found.
	 *	.
	 *
	 * \note search treats the root level as top-level, i.e. it
	 * will never backtrack below that level.
	 */
	ValueRep search(uint64 maxConflict, uint64 maxLearnts, double randProp = 0.0, bool localR = false);

	//! number of (unprocessed) literals in the propagation queue
	uint32 queueSize() const { return (uint32) trail_.size() - front_; }

	uint8& data(Var v) { return vars_[v].unused; }
private:
	typedef PodVector<uint8>::type TypeVec;
	typedef PodVector<Constraint*>::type	ConstraintDB;
	typedef std::pair<uint32, ConstraintDB> LevelInfo;
	typedef std::vector<LevelInfo> TrailLevels; 	
	typedef PodVector<Watch>::type GWL;
	typedef PodVector<ImpliedLiteral>::type ImpliedLits;
	// stub for storing ternary clauses.
	// A ternary clause [x y z] is stored using 3 stubs:
	// ~x, TernStub(y,z)
	// ~y, TernStub(x,z)
	// ~z, TernStub(x,y)
	typedef std::pair<Literal, Literal> TernStub;
	typedef LitVec BWL;
	typedef PodVector<TernStub>::type TWL;
	struct WL {
		BWL		bins;
		TWL		terns;
		GWL		gens;
		GWL::size_type size() const {
			return bins.size() + terns.size() + gens.size();
		}
	};
	typedef PodVector<WL>::type Watches;
	static GWL& gwl(WL& w) { return w.gens; }
	static const GWL& gwl(const WL& w) { return w.gens; }
	static BWL& bwl(WL& w) { return w.bins; }
	static TWL& twl(WL& w) { return w.terns; }
	inline bool validWatch(Literal p) const {
		return p.index() < (uint32)watches_.size();
	}
	
	void initRandomHeuristic(double randProp);
	void		freeMem();
	void		simplifySAT();
	void		simplifyShort(Literal p);
	void		simplifyDB(ConstraintDB& db);
	bool		unitPropagate();
	void		undoLevel(bool sp);
	void		minimizeConflictClause(ClauseCreator& outClause, uint32 abstr);
	bool		analyzeLitRedundant(Literal p, uint32 abstr);
	uint32	analyzeConflict(ClauseCreator& outClause);
	bool		localRestart(uint64& mCfl) {
		if (decisionLevel() > 0 && (stats.conflicts - levConflicts_[decisionLevel()-1]) > mCfl) {
			mCfl = 0;
			return true;
		}
		return false;
	}
	SolverStrategies	strategy_;		// Strategies used by this solver-object
	State							vars_;				// For Var v vars_[v] stores state of v
	ConstraintDB			constraints_;	// problem constraints
	ConstraintDB			learnts_;			// learnt constraints
	ImpliedLits				impliedLits_;	// Lits that were asserted on current dl but are logically implied earlier
	LitVec						conflict_;		// stores conflict-literals for later analysis
	LitVec						cc_;					// temporary: stores conflict clause within analyzeConflict
	LitVec						trail_;				// Assignment stack; stores assigments in the order they were made
	TrailLevels				levels_;			// Stores start-position in trail of every decision level
	VarVec						levConflicts_;// For a DL d, levConflicts_[d-1] stores num conflicts when d was started
	Watches						watches_;			// watches_[p.index()] is a list of constraints watching p
	LitVec::size_type	front_;				// "propagation queue"
	uint32						binCons_;			// number of binary constraints
	uint32						ternCons_;		// number of ternary constraints
	LitVec::size_type	lastSimplify_;// number of top-level assignments on last call to simplify
	uint32						rootLevel_;		// dl on which search started.
	uint32						btLevel_;			// When enumerating models: DL of the last unflipped decision from the current model. Can't backjump below this level.
	uint32						eliminated_;	// number of vars currently eliminated
	DecisionHeuristic*randHeuristic_;
	bool							shuffle_;			// shuffle program on next simplify?
};

//! returns true if p represents the special variable 0
inline bool isSentinel(Literal p) { return p.var() == sentVar; }

//! base class for all post propagators.
/*!
 * Post propagators are called after a solver finished unit-propagation. 
 * They may add more elaborate propagation mechanisms.
 * The typical asp example would be an unfounded set check.
 */
class PostPropagator {
public:
	PostPropagator();
	virtual ~PostPropagator();
	//! propagates the current assignment
	/*!
	 * \return
	 *	- false if propagation led to conflict
	 *  - true otherwise
	 * \pre the assignment is fully propagated.
	 * \post if the function returned true, the assignment is fully propagated.
	 *
	 * \note 
	 * In the context of a solver-object this method is called once for each decision level. 
	 *
	 */
	virtual bool propagate() = 0;

	//! resets the internal state of this object.
	/*!
	 * resets the internal state of this object and aborts any active propagation.
	 * \note the solver containing this object calls this method whenever a conflict is
	 * detected during unit-propagation.
	 */
	virtual void reset();
protected:
	// provides access to the solver's unit-propagation function.
	// A propagator typically runs a simple loop:
	// first one or more vars are asserted, then unitPropagate
	// is called to propagate the newly asserted vars.
	// This process is repeated until a fixpoint is reached.
	bool unitPropagate(Solver& s) const {
		return s.unitPropagate();
	}
private:
	PostPropagator(const PostPropagator&);
	PostPropagator& operator=(const PostPropagator&);
};

//! A "null-object" that implements the PostPropagator interface
class NoPostPropagator : public PostPropagator {
public:
	bool propagate() { return true; }
};

//@}


/**
 * \defgroup heuristic Decision Heuristics
 */
//@{

//! Base class for decision heuristics to be used in a Solver.
/*! 
 * During search the decision heuristic is used whenever the DPLL-procedure must pick 
 * a new variable to branch on. Each concrete decision heuristic can implement a
 * different algorithm for selecting the next decision variable.
 */
class DecisionHeuristic {
public:
	DecisionHeuristic() {}
	virtual ~DecisionHeuristic();	
	
	/*!
	 * Called once after all problem variables are known to the solver.
	 * The default-implementation is a noop.
	 * \param s The solver in which this heuristic is used.
	 */
	virtual void startInit(const Solver& /* s */) {}	

	/*!
	 * Called once after all problem constraints are known to the solver.
	 * The default-implementation is a noop.
	 * \param s The solver in which this heuristic is used.
	 */
	virtual void endInit(const Solver& /* s */) {}	

	/*!
	 * If the heuristic is used in an incremental setting, enable/disable
	 * reinitialization of existing variables.
	 * The default-implementation is a noop. Hence, heuristics will typically
	 * simply (re-)initialize all variables.
	 */
	virtual void reinit(bool /* b */) {}
	
	/*!
	 * Called for each var that changes its state from eliminated back to normal.
	 * The default-implementation is a noop.
	 * \param v The variable that is resurrected
	 */
	virtual void resurrect(Var /* v */) {}
	
	/*!
	 * Called on decision level 0. Variables that are assigned on this level
	 * may be removed from any decision heuristic.
	 * \note Whenever the solver returns to dl 0, simplify is only called again
	 * if the solver learnt new facts since the last call to simplify.
	 *
	 * \param s The solver that reached decision level 0.
	 * \param st The position in the trail of the first new learnt fact.
	 */
	virtual void simplify(Solver& /* s */, LitVec::size_type /* st */) {}
	
	/*!
	 * Called whenever the solver backracks.
	 * Literals in the range [s.trail()[st], s.trail().size()) are subject to backtracking.
	 * The default-implementation is a noop.
	 * \param s The solver that is about to backtrack
	 * \param st Position in the trail of the first literal that will be backtracked.
	 */
	virtual void undoUntil(const Solver& /* s */, LitVec::size_type /* st */) {}
	
	/*!
	 * Called whenever a new constraint is added to the solver s.
	 * The default-implementation is a noop.
	 * \param s The solver to which the constraint is added.
	 * \param first First literal of the new constraint.
	 * \param size Size of the new constraint.
	 * \param t Type of the new constraint.
	 * \note first points to an array of size size.
	 */
	virtual void newConstraint(const Solver&, const Literal* /* first */, LitVec::size_type /* size */, ConstraintType /* t */) {}
	
	/*!
	 * Called for each new reason-set that is traversed during conflict analysis.
	 * The default-implementation is a noop.
	 * \param s the solver in which the conflict is analyzed.
	 * \param lits The current reason-set under inspection.
	 * \param resolveLit The literal that is currently resolved.
	 * \note When a conflict is detected the solver passes the conflicting literals
	 * in lits during the first call to updateReason. On that first call resolveLit
	 * is the sentinel-literal.
	 */
	virtual void updateReason(const Solver& /* s */, const LitVec& /* lits */, Literal /* resolveLit */) {}
	
	/*! 
	 * Called whenever the solver must pick a new variable to branch on. 
	 * \param s The solver that needs a new decision variable.
	 * \return
	 *	- true	:	if the decision heuristic asserted a literal 
	 *  - false : if no appropriate decision literal could be found, because assignment is total
	 *  .
	 * \post
	 * if true is returned, the heuristic has asserted a literal.
	 */
	bool select(Solver& s) {
		if (s.numFreeVars() != 0) {
			Literal l = doSelect(s);
			return isSentinel(l) || s.assume(
				!s.strategies().saveProgress || s.preferredValue(l.var()) == value_free
				?	l 
				: Literal(l.var(), s.preferredValue(l.var()) == value_false)
			);
		}
		return false;
	}
private:
	DecisionHeuristic(const DecisionHeuristic&);
	DecisionHeuristic& operator=(const DecisionHeuristic&);
	
	//! implements the actual selection process
	/*!
	 * \pre s.numFreeVars() > 0, i.e. there is at least one variable to branch on.
	 * \return 
	 *	- a literal that is currently free or
	 *  - a sentinel literal. In that case, the heuristic shall have asserted a literal!
	 */ 
	virtual Literal doSelect(Solver& /* s */) = 0;
};

//! selects the first free literal w.r.t to the initial variable order.
class SelectFirst : public DecisionHeuristic {
private:
	Literal doSelect(Solver& s);
};

//@}
}
#endif
