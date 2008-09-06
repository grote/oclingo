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
#ifndef CLASP_SMODELS_CONSTRAINTS_H_INCLUDED
#define CLASP_SMODELS_CONSTRAINTS_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <clasp/include/constraint.h>

namespace Clasp {

//! Class implementing smodels-like cardinality- and weight constraints.
/*!
 * \ingroup constraint
 * This Constraint class represents the body of a smodels-like cardinality- 
 * resp. weight-constraint (i.e. only a lower bound is supported and weights must be
 * positive integers). A cardinality-constraint is handled like a weight-constraint where
 * all weights are equal to 1.
 *
 * The class implements the following four inference rules:
 * let sumWeights be the sum of the weights of all predecessors that are currently true.
 * let sumReach be the sum of the weights of all predecessors that are currently not false.
 * - FTB: If sumWeigts >= bound: set the body to true.
 * - BFB: If the body is false: set false all literals p for which sumWeights + weight(p) >= bound.
 * - FFB: If sumReach < bound: set the body to false.
 * - BTB: If the body is true: set true all literals p for which sumReach - weight(p) < bound.
 */
class BasicAggregate : public Constraint {
public:
	//! Creates a new BasicAggregate constraint from the given weight constraint
	/*!
	 * If the given weight constraint is initially true/false,
	 * its associated literal is assigned appropriately but no constraint is created. Otherwise
	 * the new BasicAggregate constraint is added to the solver.
	 * \param s solver in which the new constraint is to be used.
	 * \param con the literal that is associated with the constraint
	 * \param lits the literals of the weight constraint
	 * \param bound the lower bound of the weight constraint.
	 * \return false if the constraint is initially conflicting w.r.t the current assignment.
	 * \note Cardinality constraint are represented as weight constraints with all weights equal
	 * to 1.
	 */
	static bool newWeightConstraint(Solver& s, Literal con, WeightLitVec& lits, weight_t bound);
	bool simplify(Solver& s, bool = false);
	void destroy();
	PropResult propagate(const Literal& p, uint32& data, Solver& s);
	void reason(const Literal& p, LitVec& lits);
	void undoLevel(Solver& s);
	ConstraintType type() const { return Constraint_t::native_constraint; }
private:
	enum ActiveAggregate {
		ffb_btb = 1,  // watches con, ~l1, ..., ~ln
		ftb_bfb = 2   // watches ~con, l1, ..., ln
	};
	BasicAggregate(Solver& s, Literal con, const WeightLitVec& lits, uint32 bound, uint32 sumWeights, bool cardinality);
	~BasicAggregate();
	
	// returns the idx-th literal of sub-constraints c, i.e.
	// if c == ffb_btb: lits[idx]
	// if c == ftb_bfb: ~lits[idx]
	Literal lit(uint32 idx, ActiveAggregate c) const {
		return Literal::fromIndex(
			lits_[idx].index() ^ (c-1)
		);
	}
	void    addUndo(Solver& s, uint32 idx, ActiveAggregate a, bool markAsProcessed);
	uint32  lastUndoLevel(Solver&);
	uint32  undoStart() const { return wr_ == 0 ? size_ : size_<<1; }
	uint32& next()  { assert(wr_ == 1); return lits_[size_*3].asUint(); }
	uint32& bp()    { assert(wr_ == 1); return lits_[(size_*3)+1].asUint(); }
	
	// returns the weight of literal with the given index.
	// Note: if aggregate is a cardinality constraint the returned weight is always 1
	weight_t weight(uint32 index) const {
		return wr_ == 0
			? weight_t(1)
			: (weight_t)lits_[size_+index].asUint();
	}
	uint32  size_     : 30;   // number of lits in aggregate (incl. the literal associated with the constraint)
	uint32  active_   : 2;    // which of the two sub-constraints is currently relevant?
	uint32  undo_     : 31;   // undo position. Literals in the range [undoStart(), undo_) were processed in propagate
	uint32  wr_       : 1;    // 1 if this is a weight constraint, otherwise 0
	int32   bound_[2];        // bound_[0]: init: (sumW-bound)+1, if <= 0: FFB-BTB
														// bound_[1]: init: bound, if <= 0: FTB/BFB
	// if size is n and wr_ == 1, then
	// [0, n)       stores the literals of this aggregate, i.e. ~B, l1, ..., ln-1
	// [n, 2n)      stores the weights of the literals
	// [2n, 3n)     stores the assigned (and relevant) literals of this aggregate
	// lits_[3n]    position of next literal to look at during backpropagation
	// lits_[3n+1]  number of literals forced during backpropagation
	Literal   lits_[0]; 
};


//! Implements a (meta-)constraint for supporting Smodels-like minimize statements
/*!
 * \ingroup constraint
 * A solver contains at most one minimize constraint, but a minimize constraint
 * may represent several minimize statements. In that case, minimize statements added
 * earlier have higher priority. Note that this is inverse to the order used in lparse,
 * where the minimize statement written last has the highest priority. Otherwise, 
 * priority is handled like in smodels, i.e. statements
 * with lower priority become relevant only when all statements with higher priority
 * currently have an optimal assignment.
 *
 * MinimizeConstraint supports two notions of optimality: if mode is set to
 * compare_less solutions are considered optimal only if they are strictly smaller 
 * than previous solutions, otherwise, if mode is set to compare_less_equal a
 * solution is considered optimal if it is not greater than any previous solution.
 * Example: 
 *  m0: {a, b}
 *  m1: {c, d}
 *  Solutions: {a, c,...}, {a, d,...} {b, c,...}, {b, d,...} {a, b,...} 
 *  Optimal compare_less: {a, c, ...} (m0 = 1, m1 = 1}
 *  Optimal compare_less_equal: {a, c, ...}, {a, d,...}, {b, c,...}, {b, d,...} 
 * 
 */
class MinimizeConstraint : public Constraint {
public:
	//! Supported comparison operators
	/*! 
	 * Defines the possible comparison operators used when comparing
	 * (partial) assignments resp. solutions.
	 * If compare_less is used, an optimal solution is one that is
	 * strictly smaller than all previous solutions. Otherwise
	 * an optimal solution is one that is no greater than any previous
	 * solution.
	 */
	enum Mode {
		compare_less,       /**< optimize using < */
		compare_less_equal  /**< optimize using <= */
	};
	
	//! creates an empty minimize-constraint.
	/*!
	 * \note The default comparison mode is Mode::compare_less 
	 */
	MinimizeConstraint();
	~MinimizeConstraint();

	//! number of minimize statements contained in this constraint
	uint32 numRules() const { return (uint32)minRules_.size(); }

	//! Sets the comparison mode used in this constraint
	void setMode(Mode m, bool restartAfterModel = false) { mode_ = m; restart_ = (mode_ == compare_less && restartAfterModel); }

	//! Sets the optimum of a minimize-statement 
	/*!
	 * \param level priority level of the minimize statement for which the optimum should be set
	 * \param opt the new optimum
	 */
	void setOptimum(uint32 level, uint32 opt) {
		assert(level < minRules_.size());
		minRules_[level]->opt_ = opt;
	}
	
	//! Adds a minimize statement.
	/*!
	 * \pre simplify was not yet called
	 * \note If more than one minimize statement is added those added
	 * earlier have a higher priority.
	 */
	void minimize(Solver& s, const WeightLitVec& literals);
	
	//! Notifies the constraint about a model
	/*!
	 * setModel must be called whenever the solver finds a model.
	 * The found model is recorded as optimum and the decision level
	 * on which search should continue is returned.
	 * \return The decision level on which the search should continue or
	 *  uint32(-1) if search space is exhausted w.r.t. this minimize-constraint.
	 */
	uint32 setModel(Solver& s);

	//! Notifies the constraint about a model and backtracks the solver
	/*!
	 * Calls setModel and backtracks to the returned decision level if possible.
	 * \return
	 *  - true if backtracking was successful and search can continue
	 *  - false otherwise
	 */
	bool backtrackFromModel(Solver& s);
	
	//! returns the number of optimal models found so far
	uint64 models() const;
	void printOptimum(std::ostream& os) const;
	bool allowRestart() const { return restart_; }
	
	// constraint interface
	PropResult propagate(const Literal& p, uint32& data, Solver& s);
	void undoLevel(Solver& s);
	void reason(const Literal& p, LitVec& lits);
	bool simplify(Solver& s, bool = false);
	ConstraintType type() const {
		return Constraint_t::native_constraint;
	}
	
	bool select(Solver& s);
private:
	MinimizeConstraint(const MinimizeConstraint&);
	MinimizeConstraint& operator=(MinimizeConstraint&);
	
	struct MinRule {
		MinRule() : sum_(0), opt_(std::numeric_limits<weight_t>::max()) {}
		weight_t      sum_;     // current sum
		weight_t      opt_;     // optimial sum so far
		WeightLitVec  lits_;    // sorted by decreasing weights
	};
	// A literal occurrence in one MinRule
	struct LitRef {
		uint32 ruleIdx_;
		uint32 litIdx_;
	};
	// A literal that was assigned true by unit propagation or forced
	// to false by this constraint.
	struct UndoLit {
		UndoLit(Literal p, uint32 key, bool pos) : lit_(p), key_(key), pos_(pos) {}
		Literal lit_;       // assigned literal
		uint32  key_ : 31;  // pos_ == 1: index into occurList, else: true lits of rules [0, key] are the reason for lit
		uint32  pos_ : 1;   // 1 if assigned true
	};
	
	typedef PodVector<LitRef>::type LitOccurrence;
	typedef std::vector<LitOccurrence> OccurList;
	typedef PodVector<UndoLit>::type UndoList;
	typedef PodVector<MinRule*>::type Rules;
	typedef PodVector<uint32>::type Index;
	
	Literal   lit(const LitRef& r)    const { return minRules_[r.ruleIdx_]->lits_[r.litIdx_].first; }
	MinRule*  rule(const LitRef& r)   const { return minRules_[r.ruleIdx_]; }
	MinRule*  rule(uint32 pLevel)     const { return minRules_[pLevel]; }
	weight_t  weight(const LitRef& r) const { return minRules_[r.ruleIdx_]->lits_[r.litIdx_].second; }
	uint32    pLevel(const LitRef& r) const { return r.ruleIdx_; }
	void updateSum(uint32 key);
	void addUndo(Solver& s, Literal p, uint32 key, bool forced);
	bool conflict(uint32& level) const;
	bool backpropagate(Solver& s, MinRule* r);

	Index     index_;       // maps between Literal and Occur-List - only used during construction
	OccurList occurList_;   // occurList_[idx] sorted by increasing rule index
	Rules     minRules_;    // all minimize rules in priority-order
	UndoList  undoList_;    // stores indices into occurList
	uint64    models_;      // number of optimal models so far
	uint32    activePL_;    // priority level under consideration
	uint32    activeIdx_;   // index into rule of current priority level
	Mode      mode_;        // how to compare assignments?
	bool      restart_;     // Restart after each optimal model?
};

}

#endif
