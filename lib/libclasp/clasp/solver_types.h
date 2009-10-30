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
#ifndef CLASP_SOLVER_TYPES_H_INCLUDED
#define CLASP_SOLVER_TYPES_H_INCLUDED
#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/literal.h>
#include <clasp/constraint.h>

#if !defined(MAINTAIN_JUMP_STATS)
#define MAINTAIN_JUMP_STATS 0           // alternative: 1
#endif

/*!
 * \file 
 * Contains some types used by a Solver
 */
namespace Clasp {

/*!
 * \addtogroup solver
 */
//@{

//! Possible types of a variable.
struct Var_t {
	enum Value { atom_var = 1, body_var = 2, atom_body_var = atom_var | body_var};
};
typedef Var_t::Value VarType;
inline bool isBody(VarType t) { return (static_cast<uint32>(t) & static_cast<uint32>(Var_t::body_var)) != 0; }
inline bool isAtom(VarType t) { return (static_cast<uint32>(t) & static_cast<uint32>(Var_t::atom_var)) != 0; }

///////////////////////////////////////////////////////////////////////////////
// Count a thing or two...
///////////////////////////////////////////////////////////////////////////////
//! A struct for aggregating basic problem statistics
struct ProblemStats {
	ProblemStats() { reset(); }
	uint32  vars;
	uint32  eliminated;
	uint32  constraints[3]; /**< 0: all, 1: binary, 2: ternary */
	void    reset() { std::memset(this, 0, sizeof(*this)); }
	void diff(const ProblemStats& o) {
		vars           = std::max(vars, o.vars)-std::min(vars, o.vars);
		eliminated     = std::max(eliminated, o.eliminated)-std::min(eliminated, o.eliminated);
		for (int i = 0; i != 3; ++i) {
			constraints[i] = std::max(constraints[i], o.constraints[i])-std::min(constraints[i], o.constraints[i]);
		}		
	} 
};


//! A struct for aggregating statistics for one solve operation
struct SolveStats {
	SolveStats() { std::memset(this, 0, sizeof(*this)); }
	uint64 models;    /**< Number of models found */
	uint64 conflicts; /**< Number of conflicts found */
	uint64 loops;     /**< Number of learnt loop nogoods */
	uint64 choices;   /**< Number of choices performed */
	uint64 restarts;  /**< Number of restarts */ 
	uint64 lits[2];   /**< 0: conflict, 1: loop */
	uint32 learnt[4]; /**< 0: all, 1: binary, 2: ternary, 3: deleted */
	void updateLearnt(LitVec::size_type n, ConstraintType t) {
		assert(t != Constraint_t::native_constraint);
		++learnt[0];  
		lits[t-1] += n;
		loops += t == Constraint_t::learnt_loop;
		if (n > 1 && n < 4) {
			++learnt[n-1];
		}
	}
	void   reset() { *this = SolveStats(); }
	void   accu(const SolveStats& o) {
		models   += o.models;
		conflicts+= o.conflicts;
		loops    += o.loops;
		choices  += o.choices;
		restarts += o.restarts;
		for (int i = 0; i != 2; ++i) lits[i]   += o.lits[i];
		for (int i = 0; i != 4; ++i) learnt[i] += o.learnt[i];
	}
};

//! A struct for aggregating jump statistics relative to one solve operation
struct JumpStats {
	JumpStats() { std::memset(this, 0, sizeof(*this)); }
#if MAINTAIN_JUMP_STATS == 1
	uint64  modLits;  /**< Number of decision literals in models */
	uint64  jumps;    /**< Number of backjumps (i.e. number of analyzed conflicts) */
	uint64  bJumps;   /**< Number of backjumps that were bounded */
	uint64  jumpSum;  /**< Number of levels that could be skipped w.r.t first-uip */
	uint64  boundSum; /**< Number of levels that could not be skipped because of backtrack-level*/
	uint32  maxJump;  /**< Longest possible backjump */
	uint32  maxJumpEx;/**< Longest executed backjump (< maxJump if longest jump was bounded) */
	uint32  maxBound; /**< Max difference between uip- and backtrack-level */
	void updateModels(uint32 n) { modLits += n; }
	void updateJumps(uint32 dl, uint32 uipLevel, uint32 bLevel) {
		++jumps;
		jumpSum += dl - uipLevel; 
		maxJump = std::max(maxJump, dl - uipLevel);
		if (uipLevel < bLevel) {
			++bJumps;
			boundSum += bLevel - uipLevel;
			maxJumpEx = std::max(maxJumpEx, dl - bLevel);
			maxBound  = std::max(maxBound, bLevel - uipLevel);
		}
		else {
			maxJumpEx = maxJump;
		}
	}
	double avgJumpLen() const {
		return jumpSum/std::max(1.0,(double)jumps);
	}
	double avgJumpLenEx() const {
		return (jumpSum-boundSum)/std::max(1.0,(double)jumps);
	}
#else
	void updateModels(uint32) {}
  void updateJumps(uint32, uint32, uint32) {}
#endif
	void   reset() { *this = JumpStats(); }
};

//! A struct for aggregating some solver statistics
struct SolverStatistics{
	SolverStatistics() { }
	SolveStats   solve;
	JumpStats    jump;
	ProblemStats problem;
	void resetSolve() {
		solve.reset();
		jump.reset();
	}
};
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// A watch is a constraint and a "data-blob"
///////////////////////////////////////////////////////////////////////////////
//! Represents a watch in a Solver.
struct Watch {
	Watch(Constraint* a_con, uint32 a_data = 0) : con(a_con), data(a_data) {}
	//! calls propagate on the stored constraint and passes the stored data to that constraint.
	Constraint::PropResult propagate(Solver& s, Literal p) { return con->propagate(p, data, s); }
	//! returns true if other contains the same constraint as *this
	bool operator==(const Watch& other) const { return con == other.con; }
	Constraint* con;    /**< The constraint watching a certain literal */
	uint32      data;   /**< Additional data associated with this watch - passed to constraint on update */
};

typedef uint8 ValueRep;           /**< Type of the three value-literals */
const ValueRep value_true   = 1;  /**< Value used for variables that are true */
const ValueRep value_false  = 2;  /**< Value used for variables that are false */
const ValueRep value_free   = 0;  /**< Value used for variables that are unassigned */

//! returns the value that makes the literal lit true.
/*!
 * \param lit the literal for which the true-value should be determined.
 * \return
 *   - value_true     iff lit is a positive literal
 *   - value_false    iff lit is a negative literal.
 *   .
 */
inline ValueRep trueValue(const Literal& lit) { return 1 + lit.sign(); }

//! returns the value that makes the literal lit false.
/*!
 * \param lit the literal for which the false-value should be determined.
 * \return
 *   - value_false      iff lit is a positive literal
 *   - value_true       iff lit is a negative literal.
 *   .
 */
inline ValueRep falseValue(const Literal& lit) { return 1 + !lit.sign(); }

//! Stores information about variables
class VarInfo {
public:
	VarInfo() {}
	void  reserve(uint32 maxSize) { info_.reserve(maxSize); }
	void  add(bool body) {
		info_.push_back( body ? BODY_FLAG : 0);
	}
	uint32    numVars() const { return (uint32)info_.size(); }
	
	// getters
	ValueRep  value(Var v)      const { return info_[v] & VAL_MASK; }
	ValueRep  prefValue(Var v)  const { return ValueRep((info_[v]>>2)&3u); }
	bool      frozen(Var v)     const { return (info_[v] & FROZEN_FLAG) != 0; }
	bool      eliminated(Var v) const { return (info_[v] & ELIM_FLAG) != 0;   }
	bool      body(Var v)       const { return (info_[v] & BODY_FLAG) != 0; }
	bool      eq(Var v)         const { return (info_[v] & EQ_FLAG) != 0; }
	
	// setters
	void assign(Var v, ValueRep x)    { assert(x != value_free); info_[v] |= x; }
	void undo(Var v)                  { info_[v] &= ~VAL_MASK; }
	void setPrefValue(Var v, ValueRep val) {
		info_[v] &= ~PFVAL_MASK;
		info_[v] |= (val<<2);
	}
	void setFrozen(Var v, bool b)     { b ? info_[v] |= FROZEN_FLAG : info_[v] &= ~FROZEN_FLAG; }
	void setEliminated(Var v, bool b) { b ? info_[v] |= ELIM_FLAG   : info_[v] &= ~ELIM_FLAG;   }
	void setBody(Var v, bool b)       { b ? info_[v] |= BODY_FLAG   : info_[v] &= ~BODY_FLAG;   }
	void setEq(Var v, bool b)         { b ? info_[v] |= EQ_FLAG     : info_[v] &= ~EQ_FLAG;     }
private:
	// Bit:   7     6   5   4   3   2  1  0
	//      frozen elim eq body pfVal  value
	typedef PodVector<uint8>::type InfoVec;
	static const uint8 VAL_MASK   = 3u;
	static const uint8 PFVAL_MASK = 12u;
	static const uint8 BODY_FLAG  = 1u<<4;
	static const uint8 EQ_FLAG    = 1u<<5;
	static const uint8 ELIM_FLAG  = 1u<<6;
	static const uint8 FROZEN_FLAG= 1u<<7;
	VarInfo(const VarInfo&);
	VarInfo& operator=(const VarInfo&);
	InfoVec info_;
};

//! Type used to store lookahead-information for one variable.
struct VarScore {
	VarScore() { clear(); }
	void clear() { std::memset(this, 0, sizeof(VarScore)); }
	//! Mark literal p as dependent
	void setSeen( Literal p ) {
		seen_ |= uint32(p.sign()) + 1;
	}
	//! Is literal p dependent?
	bool seen(Literal p) const {
		return (seen_ & (uint32(p.sign())+1)) != 0;
	}
	//! Is this var dependent?
	bool seen() const { return seen_ != 0; }
	//! Mark literal p as tested during lookahead.
	void setTested( Literal p ) {
		tested_ |= uint32(p.sign()) + 1;
	}
	//! Was literal p tested during lookahead?
	bool tested(Literal p) const {
		return (tested_ & (uint32(p.sign())+1)) != 0;
	}
	//! Was some literal of this var tested?
	bool tested() const { return tested_ != 0; }
	//! Were both literals of this var tested?
	bool testedBoth() const { return tested_  == 3; }

	//! Sets the score for literal p to value
	void setScore(Literal p, LitVec::size_type value) {
		if (value > (1U<<14)-1) value = (1U<<14)-1;
		if (p.sign()) nVal_ = uint32(value);
		else          pVal_ = uint32(value);
		setTested(p);
	}
	
	//! Returns the score for literal p.
	uint32 score(Literal p) const {
		return p.sign() ? nVal_ : pVal_;
	}
	
	//! Returns the scores of the two literals of a variable.
	/*!
	 * \param[out] mx the maximum score
	 * \param[out] mn the minimum score
	 */
	void score(uint32& mx, uint32& mn) const {
		if (nVal_ > pVal_) {
			mx = nVal_;
			mn = pVal_;
		}
		else {
			mx = pVal_;
			mn = nVal_;
		}
	}
	//! returns the sign of the literal that has the higher score.
	bool prefSign() const {
		return nVal_ > pVal_;
	}
private:
	uint32 pVal_  : 14;
	uint32 nVal_  : 14;
	uint32 seen_  : 2;
	uint32 tested_: 2;
};

typedef PodVector<VarScore>::type VarScores; /**< A vector of variable-scores */

//! Stores information about a literal that is implied on an earlier level than the current decision level.
struct ImpliedLiteral {
	ImpliedLiteral(Literal a_lit, uint32 a_level, const Antecedent& a_ante) 
		: lit(a_lit)
		, level(a_level)
		, ante(a_ante) {
	}
	Literal     lit;    /**< The implied literal */
	uint32      level;  /**< The earliest decision level on which lit is implied */
	Antecedent  ante;   /**< The reason why lit is implied on decision-level level */
};

//@}
}
#endif
