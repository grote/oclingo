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

#ifdef _MSC_VER
#pragma warning (disable : 4146) // unary minus operator applied to unsigned type, result still unsigned
#pragma warning (disable : 4996) // 'std::_Fill_n' was declared deprecated
#endif

#include <clasp/include/program_rule.h>
#include <clasp/include/program_builder.h>
#include <clasp/include/util/misc_types.h>
#include <deque>
#include <numeric>

namespace Clasp {

/////////////////////////////////////////////////////////////////////////////////////////
// class PrgRule
//
// Code for handling a rule of a logic program
/////////////////////////////////////////////////////////////////////////////////////////
void PrgRule::clear() {
	heads.clear();
	body.clear();
	bound_  = 0;
	type_   = ENDRULE;
}

// Adds atomId as head to this rule.
PrgRule& PrgRule::addHead(Var atomId) {
	assert(type_ != ENDRULE && "Invalid operation - Start rule not called");
	heads.push_back(atomId);
	return *this;
}

// Adds atomId to the body of this rule. If pos is true, atomId is added to B+
// otherwise to B-. The weight is ignored (set to 1) unless the rule is a weight/optimize rule
PrgRule& PrgRule::addToBody(Var atomId, bool pos, weight_t weight) {
	assert(type_ != ENDRULE && "Invalid operation - Start rule not called");
	assert( weight > 0 );
	if (weight != 1 && !(type_ == WEIGHTRULE || type_ == OPTIMIZERULE)) {
		weight = 1;
	}
	body.push_back(WeightLiteral(Literal(atomId, pos == false), weight));
	return *this;
}

// Simplifies the rule:
// - NORMALIZE: make sure that each literal occurs at most once in the head/body, i.e.
//  - drop duplicate head atoms
//  - drop duplicate body-literals from normal/choice rules
//    - h :- b, b -> h :- b.
//  - merge duplicate body-literals in extended rules
//    - h :- 2 {a, b, a} -> h :- 2 [a=2, b].
//  - if the lower bound of a cardinality/weight constraint equals the maximal
//  achievable weight, replace rule with a normal rule
//    - h :- 5 [a=2, b=3] -> h :- a, b.
// - CONTRA: drop contradictory rules
//  - if rule contains p and not p and both are strictly needed to satisfy the body, drop rule
//  - if lower bound can't be reached, drop rule
// - TAUT: check for tautologies
//  - atoms that appear in the head and the positive part of the body are removed
//  from the head if they are strictly needed to satisfy the body.
//  - SIMP-CR: atoms that appear both in the head and the body of a choice rule are removed
//  from the head.
PrgRule::RData PrgRule::simplify(RuleState& rs) {
	RData r = { 0, 0, 0, 0 };
	// 1. Check body bounds
	// maximal achievable weight
	r.sumWeight = (type_ != WEIGHTRULE && type_ != OPTIMIZERULE)
		? (weight_t)body.size()
		: std::accumulate(body.begin(), body.end(), weight_t(0), compose22(
				std::plus<weight_t>(),
				identity<weight_t>(),
				select2nd<WeightLiteral>()));
	// minimal weight needed to satisfy body
	// for constraint/weight rules this is the lower bound that was set. For
	// normal/choice rules all literals must be true to make the body true, thus
	// set lower bound to body.size().
	if (type_ != CONSTRAINTRULE && type_ != WEIGHTRULE) bound_ = (weight_t)body.size();
	if (bound_ <= 0) {
		body.clear(); // body is satisfied - ignore contained lits
		if (type_ == CONSTRAINTRULE || type_ == WEIGHTRULE) {
			type_ = BASICRULE;
		}
		r.value_ = value_true;
	}
	if (bound_ > r.sumWeight) {
		type_ = ENDRULE;  // CONTRA - body can never be satisfied
		body.clear();     // drop the rule
		r.value_ = value_false;
		return r;
	}
	else if (bound_ == r.sumWeight && (type_ == CONSTRAINTRULE || type_ == WEIGHTRULE)) {
		// all subgoals must be true in order to satisfy body
		// - just like in a normal rule, thus handle this rule as such.
		type_       = BASICRULE;
		r.sumWeight = (weight_t)body.size();
	}

	// 2. NORMALIZE body literals, check for CONTRA
	WeightLitVec::iterator j = body.begin();
	for (WeightLitVec::iterator it = body.begin(), bEnd = body.end(); it != bEnd; ++it) {
		if (!rs.inBody(it->first)) {  // a not seen yet
			*j++  = *it;                // keep it and 
			rs.addToBody(it->first);    // mark as seen
			if (it->first.sign()) { r.hash += hashId(-it->first.var()); }
			else                  { ++r.posSize; r.hash += hashId(it->first.var()); }
		}
		else if (type_ != BASICRULE && type_ != CHOICERULE) {
			// a is already in the rule. Ignore the duplicate if rule is a normal or choice rule,
			// otherwise merge the occurrences by merging the weights of the literals.
			if (type_ == CONSTRAINTRULE) { type_ = WEIGHTRULE; }
			WeightLitVec::iterator lit = std::find_if(body.begin(), j, compose1(
				std::bind2nd(std::equal_to<Literal>(), it->first),
				select1st<WeightLiteral>()));
			assert(lit != j);
			lit->second += it->second;
		}
		if (rs.inBody(~it->first)) {
			// rule contains p and not p - check if both are needed to satisfy body
			if ( type_ == BASICRULE || type_ == CHOICERULE || (type_ == WEIGHTRULE 
				&& r.sumWeight - std::min(it->second, weight(it->first.var(), it->first.sign())) < bound_)) {
				type_ = ENDRULE;  // CONTRA
				break;
			}
		}
	}
	body.erase(j, body.end());
	if (type_ != CONSTRAINTRULE && type_ != WEIGHTRULE) {
		// just in case we removed duplicate atoms
		bound_ = static_cast<weight_t>(body.size());
	}
	Weights w(*this);
	// 3. NORMALIZE head atoms, check for TAUT and SIMP-CR
	if (type_ != ENDRULE) {
		VarVec::iterator j = heads.begin();
		for (VarVec::iterator it = heads.begin(), end = heads.end();  it != end; ++it) {
			if ( !rs.inHead(*it) && !rs.superfluousHead(type(), r.sumWeight, bound(), *it, w)) {
				rs.addToHead(*it);  // mark as seen
				if (rs.selfblocker(type(), r.sumWeight, bound(), *it, w)) {
					r.value_= value_false;
				}
				*j++ = *it;
			}
		}
		heads.erase(j, heads.end());
		if (heads.empty() && type_ != OPTIMIZERULE) type_ = ENDRULE;
	}
	return r;
}

weight_t PrgRule::weight(Var id, bool pos) const {
	if (type_ != WEIGHTRULE && type_ != OPTIMIZERULE) {
		return 1;
	}
	return std::find_if(body.begin(), body.end(), compose1(
		std::bind2nd(std::equal_to<Literal>(), Literal(id, pos==false)),
		select1st<WeightLiteral>()
		))->second;
}

// Transforms this rule into an equivalent set of normal rules
uint32 PrgRule::transform(ProgramBuilder& prg, TransformationMode m) {
	if (type() == ENDRULE || type() == BASICRULE || type() == OPTIMIZERULE) {
		return 0;
	}
	if (type_ == CHOICERULE) {
		return transformChoice(prg);
	}
	else if (type_ == CONSTRAINTRULE || type_ == WEIGHTRULE) {
		return transformAggregate(prg, m);
	}
	assert(!"unknown rule type");
	return 0;
}

// A choice rule {h1,...hn} :- BODY
// is replaced with:
// h1   :- BODY, not aux1.
// aux1 :- not h1.
// ...
// hn   :- BODY, not auxN.
// auxN :- not hn.
// If n is large or BODY contains many literals BODY is replaced with auxB and
// auxB :- BODY.
uint32 PrgRule::transformChoice(ProgramBuilder& prg) {
	uint32 newRules = 0;
	Var extraHead = ((heads.size() * (body.size()+1)) + heads.size()) > (heads.size()*3)+body.size()
			? prg.newAtom()
			: varMax;
	PrgRule r1, r2;
	r1.setType(BASICRULE); r2.setType(BASICRULE);
	if (extraHead != varMax) { r1.addToBody( extraHead, true, 1 ); }
	else { r1.body.swap(body); }
	for (VarVec::iterator it = heads.begin(), end = heads.end(); it != end; ++it) {
		r1.heads.clear(); r2.heads.clear();
		Var aux = prg.newAtom();
		r1.heads.push_back(*it);  r1.addToBody(aux, false, 1);
		r2.heads.push_back(aux);  r2.addToBody(*it, false, 1);
		prg.addRule(r1);  // h    :- body, not aux
		prg.addRule(r2);  // aux  :- not h
		r1.body.pop_back();
		r2.body.pop_back();
		newRules += 2;
	}
	if (extraHead != varMax) {
		r1.heads.clear();
		r1.body.clear();
		r1.body.swap(body);
		r1.heads.push_back(extraHead);
		prg.addRule(r1);
		++newRules;
	}
	body.swap(r1.body);
	return newRules;
}

// Transforms cardinality and weight constraint.
// Depending on the rule's size, its lower bound and the weights contained
// in the rule, two different transformation algorithms are applied.
// The first algorithm introduces new atoms and is quadratically bounded.
// The second algorithm does not introduce new atoms but creates an exponential number
// of rules in the worst case. It is applied only for small rules and simple bounds.
uint32 PrgRule::transformAggregate(ProgramBuilder& prg, TransformationMode m) {
	if (type_ == WEIGHTRULE) {
		std::stable_sort(body.begin(), body.end(), compose22(
			std::greater<weight_t>(),
			select2nd<WeightLiteral>(),
			select2nd<WeightLiteral>()));
	}
	WeightVec sumWeights(body.size() + 1, 0);
	LitVec::size_type i = (uint32)body.size();
	weight_t sum = 0;
	while (i != 0) {
		--i;
		sum += body[i].second;
		sumWeights[i] = sum;
	}
	return m == quadratic_transform || (m == dynamic_transform && body.size() >= 6 && choose(sumWeights[0], bound_) >= (body.size()*bound_*2))
		? transformAggregateQuad(prg, sumWeights)
		: transformAggregateExp(prg, sumWeights);
}

// quadratic transformation - introduces aux atoms
uint32 PrgRule::transformAggregateQuad(ProgramBuilder& prg, const WeightVec& sumWeights) {
	typedef std::pair<uint32, weight_t> Key;
	typedef std::deque<Key> KeyQueue;
	if (bound_ == 0) {
		PrgRule r(BASICRULE);
		r.addHead(heads[0]);
		prg.addRule(r);
		return 1;
	}
	VarVec atoms(body.size() * bound_, varMax);
	atoms[bound_-1] = heads[0];
	KeyQueue todo;
	todo.push_back( Key(0, bound_) );
	PrgRule r(BASICRULE);
	r.heads.resize(1);
	uint32 newRules = 0;
	while (!todo.empty()) {
		r.body.clear();
		uint32    n = todo.front().first;
		weight_t  w = todo.front().second;
		todo.pop_front();
		Var a = atoms[(n * bound_) + (w-1)];
		assert(a != varMax);
		r.heads[0] = a;
		if (sumWeights[n+1] >= w) {
			++newRules;
			assert(n+1 < (uint32)body.size());
			uint32 nextKey = ((n+1) * bound_) + (w-1);
			if (atoms[nextKey] == varMax) {
				atoms[nextKey] = prg.newAtom();
				todo.push_back( Key(n+1,w) );
			}
			r.addToBody(atoms[nextKey], true);
			prg.addRule(r);
			r.body.clear();
		} 
		weight_t nk = w - body[n].second;
		if (nk <= 0 || sumWeights[n+1] >= nk) {
			++newRules;
			r.addToBody(body[n].first.var(), body[n].first.sign() == false);
			if (nk > 0) {
				assert((n+1) < (uint32)body.size());
				uint32 nextKey = ((n+1) * bound_) + (nk-1);
				if (atoms[nextKey] == varMax) {
					atoms[nextKey] = prg.newAtom();
					todo.push_back( Key(n+1,nk) );
				}
				r.addToBody(atoms[nextKey], true);
			}
			prg.addRule(r);
			r.body.clear();
		}
	}
	return newRules;
}

// exponential transformation - creates minimal subsets, no aux atoms
uint32 PrgRule::transformAggregateExp(ProgramBuilder& prg, const WeightVec& sumWeights) {
	uint32 newRules = 0;
	VarVec    nextStack;
	WeightVec weights;
	PrgRule r(BASICRULE);
	r.addHead(heads[0]);
	uint32    end   = (uint32)body.size();
	weight_t  cw    = 0;
	uint32    next  = 0;
	if (next == end) { prg.addRule(r); return 1; }
	while (next != end) {
		r.addToBody(body[next].first.var(), body[next].first.sign() == false);
		weights.push_back( body[next].second );
		cw += weights.back();
		++next;
		nextStack.push_back(next);
		if (cw >= bound_) {
			prg.addRule(r);
			r.setType(BASICRULE);
			++newRules;
			r.body.pop_back();
			cw -= weights.back();
			nextStack.pop_back();
			weights.pop_back();
		}
		while (next == end && !nextStack.empty()) {
			r.body.pop_back();
			cw -= weights.back();
			weights.pop_back();
			next = nextStack.back();
			nextStack.pop_back();
			if (next != end && (cw + sumWeights[next]) < bound_) {
				next = end;
			}
		}
	}
	return newRules;
}

}
