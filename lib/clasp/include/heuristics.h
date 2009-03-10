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
#ifndef CLASP_HEURISTICS_H_INCLUDED
#define CLASP_HEURISTICS_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

/*!
 * \file 
 * Defines various decision heuristics to be used in clasp.
 * 
 */

#include <clasp/include/solver.h>
#include <clasp/include/pod_vector.h>
#include <clasp/include/util/indexed_priority_queue.h>
#include <list>
namespace Clasp { 

//! Lookahead is both a heuristic as well as a propagation-mechanism.
/*!
 * \ingroup heuristic
 * Lookahead extends propagation with failed-literal detection. If
 * used as heuristic it will select the literal with the highest score, 
 * where the score is determined by counting assignments made during
 * failed-literal detection. hybrid_lookahead simply selects the literal that
 * derived the most literals. uniform_lookahead behaves similar to the smodels
 * lookahead heuristic (the literal that maximizes the minimum is selected).
 *
 * Alternatively Lookahead can forward to a seperate decision heuristic. In that
 * case Lookahead only performs failed-literal detection. If no literal failed it
 * then calls the select-method of the installed decision heuristic. In this
 * mode Lookahead is a decorator of that other heuristic.
 * 
 * \see Patrik Simons: "Extending and Implementing the Stable Model Semantics" for a
 * detailed description of the lookahead heuristic.
 * 
 */
class Lookahead : public DecisionHeuristic {
public:
	//! Defines the supported lookahead-types
	enum Type { 
		auto_lookahead, /**< Let the class decide which lookahead-operation to use */
		atom_lookahead, /**< Test only atoms in both phases */
		body_lookahead, /**< Test only bodies in both phases */
		hybrid_lookahead/**< Test atoms and bodies but only their preferred decision literal */
	};
	/*!
	 * \param t Type of variables to consider during failed-literal detection.
	 * \param m Disable failed-literal detection after m choices (-1 always keep enabled)
	 * \param heuristic if 0 use the results of failed-literal-detection to determine
	 * next choice. Otherwise use the given heuristic.
	 */
	Lookahead(Type t, int m = -1, DecisionHeuristic* heuristic = 0);
	~Lookahead();
	void startInit(const Solver& /* s */);
	void endInit(const Solver& s) {
		if (select_) select_->endInit(s);
	}
	void simplify(Solver& s, LitVec::size_type st);
	void undoUntil(const Solver& s, LitVec::size_type st) {
		if (select_) select_->undoUntil(s, st);
	}
	void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t) {
		if (select_) select_->newConstraint(s, first, size, t);
	}
	void updateReason(const Solver& s, const LitVec& lits, Literal resolveLit) {
		if (select_) select_->updateReason(s, lits, resolveLit);
	}
	void reinit(bool b) {
		inc_ = true;
		if (select_) select_->reinit(b);
	}
private:
	Literal doSelect(Solver& s);
	Literal heuristic(Solver& s);
	void checkScore(uint32& min, uint32& max, Var v, const VarScore& vs, Literal& r);
	VarScores           scores_;
	VarVec              deps_;
	uint64              maxDecisions_;
	DecisionHeuristic*  select_;
	Type                type_;
	VarType             varTypes_;
	Var                 var_;
	Var                 startVar_;
	bool                inc_;
};


// Some lookback heuristics to be used together with learning.

//! computes a moms-like score for var v
uint32 momsScore(const Solver& s, Var v);

//! A variant of the BerkMin decision heuristic from the BerkMin Sat-Solver
/*!
 * \ingroup heuristic
 * \see E. Goldberg, Y. Navikov: "BerkMin: a Fast and Robust Sat-Solver"
 *
 * \note
 * This version of the BerkMin heuristic varies mostly in three points from the original BerkMin:
 *  -# it considers loop-nogoods if requested (this is the default)
 *  -# it uses a MOMS-like heuristic as long as there are no conflicts (and therefore no activities)
 *  -# it uses a MOMS-like score to break ties whenever multiple variables from an unsatisfied learnt
 * constraint have equal activities
 * 
 * The algorithm is as follows:
 * - For each variable a score s is maintained that is initially 0.
 * - During conflict analysis: For each variable v contained in a constraint CL that
 * participates in the derivation of the asserting clause, s(v) is incremented.
 * - Every 512 decisions, the score of each variable is divided by 4
 *
 * For selecting a literl DL the following algorithm is used:
 * - let CL be the most recently derived conflict clause that is currently not satisfied.
 * - if no such CL exists and look-back loops is enabled let CL be last learnt loop nogood that is currently not satisfied.
 * - if a CL was found:
 *    - let v be a free variable of CL with the highest activity. Break ties using MOMS
 *    - let p be the literal of v that occured more often in learnt nogoods. 
 *      - If both lits of v occurred equally often, set p to the preferred literal of v
 *    - set DL to p
 * - if no CL was found and #conflicts != 0
 *    - let v be the free variable with the highest activity. Break ties randomly.
 *    - let p and ~p be the two literals of v.
 *    - let s1 be the result of estimateBCP(p), let s2 be the result of estimateBCP(~p)
 *    - set DL to p iff s1 > s2, to ~p iff s2 > s1 and to the preferred decision literal of v otherwise.
 * - if #conflicts == 0
 *    - Select DL according to a MOMS-like heuristic
 * - return DL;
 * .
 */
class ClaspBerkmin : public DecisionHeuristic {
public:
	/*!
	 * \param maxBerk Check at most maxBerk candidates when searching for not yet satisfied learnt constraints
	 * \param considerLoops true if learnt loop-formulas should be considered during decision making
	 * \note maxBerk = 0 means check *all* candidates
	 */
	explicit ClaspBerkmin(uint32 maxBerk = 0, bool considerLoops = true);
	//! initializes the heuristic.
	void startInit(const Solver& s);
	void reinit(bool b) { reinit_ = b; }
	//! updates occurrence-counters if constraint is a learnt constraint.
	void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t);
	//! updates activity-counters
	void updateReason(const Solver& s, const LitVec& lits, Literal resolveLit);
	void undoUntil(const Solver&, LitVec::size_type);
	void resurrect(Var) {
		front_ = 1;
		cache_.clear();
		cacheFront_ = cache_.end();
	}
private:
	Literal doSelect(Solver& s);
	Literal selectVsids(Solver& s);
	Literal selectLiteral(Solver& s, Var v, bool vsids);
	Literal selectRange(Solver& s, const Literal* first, const Literal* last);
	bool hasTopUnsat(Solver& s);
	bool hasTopUnsat(Solver& s, uint32& maxIdx, uint32 minIdx, ConstraintType t);
	// Gathers heuristic information for one variable v.
	// *  activity_ stores number of times one of v's literals occurred in a constraint used during conflict analysis
	// *  occ_ is > 0 if posLit(v) occurred more often in learnt constraints. < if negLit(v) occurred more often 
	//    and = 0 if both lits occurred equally often.
	// *  decay_ is used for lazy activity-decaying. If decay_ is less than the global decay counter, var activity
	//    must be decayed. After decaying decay_ is set to the global decay counter. 
	struct HScore {
		HScore() : activity_(0), occ_(0), decay_(0) {}
		uint32  activity_;
		int32   occ_;
		uint32  decay_;
		uint32& activity(uint32 globalDecay) {
			if (uint32 x = (globalDecay - decay_)) {
				activity_ >>= (x<<1);
				decay_ = globalDecay;
			}
			return activity_;
		}
	};
	typedef PodVector<HScore>::type   Scores;
	typedef VarVec::iterator Pos;
	
	struct GreaterActivity {
		GreaterActivity(Scores& sc, uint32 decay) : sc_(sc), decay_(decay) {}
		bool operator()(Var v1, Var v2) const {
			return sc_[v1].activity(decay_) > sc_[v2].activity(decay_)
				|| (sc_[v1].activity_ == sc_[v2].activity_ && v1 < v2);
		}
	private:
		GreaterActivity& operator=(const GreaterActivity&);
		Scores& sc_;
		uint32  decay_;
	};
	Scores  score_;         // For each var v score_[v] stores heuristic score of v
	VarVec  cache_;         // Caches the most active variables
	LitVec  freeLits_;      // Stores free variables of the last learnt conflict clause that is not sat
	LitVec  freeLoopLits_;  // Stores free variables of the last learnt loop nogood that is not sat
	uint32  topConflict_;   // index into the array of learnt nogoods used when searching for conflict clauses that are not sat
	uint32  topLoop_;       // index into the array of learnt nogoods used when searching for loop nogoods that are not sat
	Var     front_;         // first variable whose truth-value is not already known - reset on backtracking
	Pos     cacheFront_;    // first unprocessed cache position - reset on backtracking
	uint32  cacheSize_;     // cache at most cacheSize_ variables
	uint32  decay_;         // "global" decay counter. Increased every 512 decisions
	uint32  numVsids_;      // number of consecutive vsids-based decisions
	uint32  maxBerkmin_;    // when searching for an open learnt constraint, check at most maxBerkmin_ candidates.
	bool    loops_;         // Consider loop nogoods when searching for learnt nogoods that are not sat
	bool    reinit_;
};

//! Variable Move To Front decision strategies inspired by Siege.
/*!
 * \ingroup heuristic
 * \see Lawrence Ryan: "Efficient Algorithms for Clause Learning SAT-Solvers"
 *
 * \note This implementation of VMTF differs from the original implementation in three points:
 *  - if considerLoops is true, it moves to the front a selection of variables from learnt loop nogoods
 *  - it measures variable activity by using a BerkMin-like score scheme
 *  - the initial order of the var list is determined using a MOMs-like score scheme
 */
class ClaspVmtf : public DecisionHeuristic {
public:
	/*!
	 * \param mtf The number of literals from constraints used during conflict resolution that are moved to the front
	 * \param considerLoops true if literals from learnt loop-formulas should be moved, too.
	 */
	explicit ClaspVmtf(LitVec::size_type mtf = 8, bool considerLoops = false);
	//! initializes the heuristic.
	void startInit(const Solver& s);
	void endInit(const Solver&);
	void reinit(bool b) { reinit_ = b; }
	/*!
	 * updates occurrence-counters if constraint is not a native constraint. Moves active
	 * vars to the front of the variable list the constraint is a conflict-clause
	 * or a loop-formula and consider loops was set to true.
	 */
	void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t);
	//! updates activity-counters
	void updateReason(const Solver& s, const LitVec& lits, Literal resolveLit);
	//! removes vars set were assigned on level 0 from the heuristic's var list
	void simplify(Solver&, LitVec::size_type);
	void undoUntil(const Solver&, LitVec::size_type);
	void resurrect(Var v) {
		if (score_[v].pos_ == vars_.end()) { score_[v].pos_ = vars_.insert(vars_.end(), v); }
		else { front_ = vars_.begin(); }
	}
private:
	Literal doSelect(Solver& s);
	Literal selectRange(Solver& s, const Literal* first, const Literal* last);
	Literal getLiteral(const Solver& s, Var v) const;
	typedef std::list<Var> VarList;
	typedef VarList::iterator VarPos;
	struct VarInfo {
		VarInfo(VarPos it) : pos_(it), activity_(0), occ_(0), decay_(0) { }
		VarPos  pos_;       // position of var in var list
		uint32  activity_;  // activity of var - initially 0
		int32   occ_;       // which literal of var occurred more often in learnt constraints?
		uint32  decay_;     // counter for lazy decaying activity
		uint32& activity(uint32 globalDecay) {
			if (uint32 x = (globalDecay - decay_)) {
				activity_ >>= (x<<1);
				decay_ = globalDecay;
			}
			return activity_;
		}
	};
	typedef PodVector<VarInfo>::type Score;
	
	struct LessLevel {
		LessLevel(const Solver& s, const Score& sc) : s_(s), sc_(sc) {}
		bool operator()(Var v1, Var v2) const {
			return s_.level(v1) < s_.level(v2) 
				|| (s_.level(v1) == s_.level(v2) && sc_[v1].activity_ > sc_[v2].activity_);
		}
		bool operator()(Literal l1, Literal l2) const {
			return (*this)(l1.var(), l2.var());
		}
	private:
		LessLevel& operator=(const LessLevel&);
		const Solver& s_;
		const Score&  sc_;
		
	};
	Score       score_;     // For each var v score_[v] stores heuristic score of v
	VarList     vars_;      // List of possible choices, initially ordered by MOMs-like score
	VarVec      mtf_;       // Vars to be moved to the front of vars_
	VarPos      front_;     // Current front-position in var list - reset on backtracking 
	uint32      decay_;     // "global" decay counter. Increased every 512 decisions
	const LitVec::size_type MOVE_TO_FRONT;
	bool        loops_;     // Move MOVE_TO_FRONT/2 vars from loop nogoods to the front of vars 
	bool        reinit_;
};

//! A variable state independent decision heuristic favoring variables that were active in recent conflicts.
/*!
 * \ingroup heuristic
 * This heuristic combines ideas from VSIDS and BerkMin. Literal Selection works as
 * in VSIDS but var activities are increased as in BerkMin.
 *
 * \see M. W. Moskewicz, C. F. Madigan, Y. Zhao, L. Zhang, and S. Malik: 
 * "Chaff: Engineering an Efficient SAT Solver"
 * \see E. Goldberg, Y. Navikov: "BerkMin: a Fast and Robust Sat-Solver"
 * 
 * The heuristic works as follows:
 * - For each variable v an activity a(v) is maintained that is initially 0.
 * - For each literal p an occurrence counter c(p) is maintained that is initially 0.
 * - When a new constraint is learnt (conflict or loop), c(p) is incremented 
 * for each literal p in the newly learnt constraint.
 * - During conflict analysis: For each variable v contained in a constraint CL that
 * participates in the derivation of the asserting clause, a(v) is incremented.
 * - Periodically the activity of each variable is divided by a constant that is a power
 * of two (currently every 512 decision variable activity is dived by 4).
 * - When a decision is necessary a free variable with the highest activity is taken
 * and from this variable the literal that occured in more learnt clauses is picked.
 * If c(v) == c(~v) the preferred choice literal of v is selected (v for bodies, ~v for heads).
 */
class ClaspVsids : public DecisionHeuristic {
public:
	/*!
	 * \param considerLoops If true, activity counters are increased for variables that occure in loop-formulas
	 */
	explicit ClaspVsids(bool considerLoops = false);
	void startInit(const Solver& s);
	void endInit(const Solver&);
	void reinit(bool b) { reinit_ = b; }
	void newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t);
	void updateReason(const Solver& s, const LitVec& lits, Literal resolveLit);
	void undoUntil(const Solver&, LitVec::size_type);
	void simplify(Solver&, LitVec::size_type);
	void resurrect(Var v) {
		vars_.update(v);  
	}
private:
	Literal doSelect(Solver& s);
	Literal selectRange(Solver& s, const Literal* first, const Literal* last);
	void updateVarActivity(Var v) {
		if ( (score_[v].first += inc_) > 1e100 ) {
			for (LitVec::size_type i = 0; i != score_.size(); ++i) {
				score_[i].first *= 1e-100;
			}
			inc_ *= 1e-100;
		}
		if (vars_.is_in_queue(v)) {
			vars_.increase(v);
		}
	}
	// HScore.first: activity of a variable
	// HScore.second: occurrence counter for a variable's literal
	//  > 0: positive literal occurred more often
	//  < 0: negative literal occurred more often
	//  = 0: literals occurred equally often
	typedef std::pair<double, int32>  HScore;
	typedef PodVector<HScore>::type   Scores;
	struct GreaterActivity {
		explicit GreaterActivity(const Scores& s) : sc_(s) {}
		bool operator()(Var v1, Var v2) const {
			return sc_[v1].first > sc_[v2].first;
		}
	private:
		GreaterActivity& operator=(const GreaterActivity&);
		const Scores& sc_;
	};
	typedef bk_lib::indexed_priority_queue<GreaterActivity> VarOrder;
	Scores    score_;
	VarOrder  vars_;
	double    inc_;
	bool      scoreLoops_;
	bool      reinit_;
};

}
#endif
