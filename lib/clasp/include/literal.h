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
#ifndef CLASP_LITERAL_H_INCLUDED
#define CLASP_LITERAL_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif
#include <clasp/include/util/platform.h>
#include <clasp/include/pod_vector.h>
#include <algorithm>  // std::swap
#include <limits>


/*!
 * \file 
 * Contains the definition of the class Literal.
 */
namespace Clasp {
/*!
 * \addtogroup constraint
 */
//@{

//! A variable is currently nothing more but an integer in the range [0..varMax)
typedef uint32 Var;

//! varMax is not a valid variale, i.e. currently Clasp supports at most 2^30 variables.
const Var varMax = (1U << 30);

//! The variable 0 has a special meaning in the solver.
const Var sentVar = 0;

//! A signed integer type used to represent weights
typedef int32 weight_t;

inline weight_t toWeight(int w) {
	assert(w < std::numeric_limits<weight_t>::max());
	return static_cast<weight_t>(w);
}

//! A literal is a variable or its negation.
/*!
 * A literal is determined by two things: a sign and a variable index. 
 *
 * \par Implementation: 
 * A literal's state is stored in a single 32-bit integer as follows:
 *  - 30-bits   : var-index
 *  - 1-bit     : sign, 1 if negative, 0 if positive
 *  - 1-bit     : watch-flag 
 */
class Literal {
public:
	//! The default constructor creates the positive literal of the special sentinel var.
	Literal() : rep_(0) { }
	
	//! creates a literal of the variable var with sign s
	/*!
	 * \param var The literal's variable
	 * \param s true if new literal should be negative.
	 * \pre var < varMax
	 */
	Literal(Var var, bool sign) : rep_( (var<<2) + (uint32(sign)<<1) ) {
		assert( var < varMax );
	}

	//! returns the unique index of this literal
	/*!
	 * \note The watch-flag is ignored and thus the index of a literal can be stored in 31-bits. 
	 */
	uint32 index() const { return rep_ >> 1; }
	
	//! creates a literal from an index.
	/*!
	 * \pre idx < 2^31
	 */
	static Literal fromIndex(uint32 idx) {
		assert( idx < (1U<<31) );
		return Literal(idx<<1);
	}

	//! creates a literal from an unsigned integer.
	static Literal fromRep(uint32 rep) { return Literal(rep); }
	
	uint32& asUint()        { return rep_; }
	uint32  asUint() const  { return rep_; }

	//! returns the variable of the literal.
	Var var() const { return rep_ >> 2; }
	
	//! returns the sign of the literal.
	/*!
	 * \return true if the literal is negative. Otherwise false.
	 */
	bool sign() const { return (rep_ & 2) != 0; }

	void swap(Literal& other) { std::swap(rep_, other.rep_); }
	
	//! sets the watched-flag of this literal
	void watch() { rep_ |= 1; }
	
	//! clears the watched-flag of this literal
	void clearWatch() { rep_ &= ~static_cast<uint32>(1u); }
	
	//! returns true if the watched-flag of this literal is set
	bool watched() const { return (rep_ & 1) != 0; }

	//! returns the complimentary literal of this literal.
	/*!
	 *  The complementary Literal of a Literal is a Literal referring to the
	 *  same variable but with inverted sign.
	 */
	inline Literal operator~() const {
		return Literal( (rep_ ^ 2) & ~static_cast<uint32>(1u) );
	}

	//! Equality-Comparison for literals.
	/*!
	 * Two Literals p and q are equal, iff
	 * - they both refer to the same variable
	 * - they have the same sign
	 * .
	 */
	inline bool operator==(const Literal& rhs) const {
		return index() == rhs.index();
	}
private:
	Literal(uint32 rep) : rep_(rep) {}
	uint32 rep_;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Common interface
/////////////////////////////////////////////////////////////////////////////////////////

//! creates the negative literal of variable v.
inline Literal negLit(Var v) { return Literal(v, true);}
//! creates the positive literal of variable v.
inline Literal posLit(Var v) { return Literal(v, false);}
//! returns the index of variable v.
/*!
 * \note same as posLit(v).index()
 */
inline uint32 index(Var v) { return v << 1; }

//! defines a strict-weak-ordering for Literals.
inline bool operator<(const Literal& lhs, const Literal& rhs) {
	return lhs.index() < rhs.index();
}

//! returns !(lhs == rhs)
inline bool operator!=(const Literal& lhs, const Literal& rhs) {
	return ! (lhs == rhs);
}

inline void swap(Literal& l, Literal& r) {
	l.swap(r);
}
typedef PodVector<Var>::type VarVec;          /**< a vector of variables  */
typedef PodVector<Literal>::type LitVec;      /**< a vector of literals   */
typedef PodVector<weight_t>::type WeightVec;  /**< a vector of weights    */

typedef std::pair<Literal, weight_t> WeightLiteral;  /**< a weight-literal */
typedef PodVector<WeightLiteral>::type WeightLitVec; /**< a vector of weight-literals */

//@}


}
#endif
