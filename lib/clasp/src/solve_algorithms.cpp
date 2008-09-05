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
#include <clasp/include/solve_algorithms.h>
#include <clasp/include/solver.h>
#include <clasp/include/smodels_constraints.h>
#include <cmath>
using std::log;
namespace Clasp { 

ModelPrinter::~ModelPrinter() {}
Enumerator::Enumerator() : printer_(0) {}
Enumerator::~Enumerator() {}
void Enumerator::updateModel(Solver& s)							{ if (printer_) printer_->printModel(s); }
bool Enumerator::backtrackFromModel(Solver& s)			{	return s.backtrack();		}
bool Enumerator::allowRestarts() const							{ return false;						}
uint64 Enumerator::numModels(const Solver& s)	const	{ return s.stats.models;	}
void Enumerator::report(const Solver&)				const { }
static Enumerator defaultEnum_s;

/////////////////////////////////////////////////////////////////////////////////////////
// SolveParams
/////////////////////////////////////////////////////////////////////////////////////////
SolveParams::SolveParams() 
	: reduceBase_(3.0), reduceInc_(1.1), reduceMaxF_(3.0)
	, restartInc_(1.5)
	, randProp_(0.0)
	, enumerator_(&defaultEnum_s)
	, restartBase_(100)
	, restartOuter_(0)
	, randRuns_(0), randConflicts_(0)
	, shuffleFirst_(0), shuffleNext_(0)
	, restartBounded_(false)
	, restartLocal_(false)
	, reduceOnRestart_(false) {
}

/////////////////////////////////////////////////////////////////////////////////////////
// Restarts
/////////////////////////////////////////////////////////////////////////////////////////
namespace {
// Implements clasp's configurable restart-strategies.
// Note: Currently all restart-strategies can be easily implemented using one class.
// In the future, as new restart strategies emerge, the class should be replaced with a strategy hierarchy
struct RestartStrategy {
public:
	RestartStrategy(uint32 base, double incA = 0.0, uint32 outer = 0) : incA_(incA), outer_(outer?outer:uint64(-1)), base_(base), idx_(0) {}
	uint64 next() {
		uint64 x;
		if			(base_ == 0)			x = uint64(-1);
		else if	(incA_ == 0)			x = lubyR();
		else											x = innerOuterR();
		++idx_;
		return x;
	}
private:
	uint64 arithR() const { return static_cast<uint64>(base_ * pow(incA_, (double)idx_)); }
	uint64 lubyR() const {
		uint32 k = idx_+1;
		while (k) {
			uint32 nk = static_cast<uint32>(log((double)k) / log(2.0)) + 1;
			if (k == ((1u << nk) - 1)) {
				return base_ * (1u << (nk-1));
			}
			k -= 1u << (nk-1);
			++k;
		}
		return base_;
	}
	uint64 innerOuterR() {
		uint64 x = arithR();
		if (x > outer_) {
			idx_		= 0;
			outer_	= static_cast<uint64>(outer_*1.5);
			return arithR();
		}
		return x;
	}
	double	incA_;
	uint64	outer_;
	uint32	base_;
	uint32	idx_;
};

}
/////////////////////////////////////////////////////////////////////////////////////////
// solve
/////////////////////////////////////////////////////////////////////////////////////////
namespace {
bool backtrackFromModel(Solver& s, const SolveParams& p) {
	p.enumerator()->updateModel(s);
	if (s.strategies().satPrePro.get() && s.strategies().satPrePro->hasSymModel()) {
		return true;
	}
	return p.enumerator()->backtrackFromModel(s);
}
}

//#define PRINT_SEARCH_PROGRESS

bool solve(Solver& s, uint32 maxAs, const SolveParams& p) {
	if (s.hasConflict()) return false;
	// ASP-specific. A SAT-Solver typically uses a fraction of the initial number
	// of clauses. For ASP this does not work as good, because the number of
	// clauses resulting from Clark's completion is far greater than the number
	// of rules contained in the logic program and thus maxLearnts tends to be
	// to large if based on number of clauses.
	// Of course, one could increase maxLearntDiv (e.g. by a factor 4) to compensate
	// for this fact. Clasp uses a different approach: it uses the number of heads
	// plus the number of bodies as basis for maxLearnt. 
	// Translated into clauses: From Clark's completion Clasp counts only the 
	// clauses that are not necessarily binary.
	double lBase				= std::min(s.numVars(), s.numConstraints());
	double maxLearnts		= p.reduceBase() != 0 ? (uint32)std::max(10.0, lBase / p.reduceBase()) : -1.0;
	double boundLearnts	= p.reduceMax() != 0 ? lBase * p.reduceMax() : std::numeric_limits<double>::max();
	if (maxLearnts < s.numLearntConstraints()) maxLearnts = s.numLearntConstraints();
	RestartStrategy rs(p.restartBase(), p.restartInc(), p.restartOuter());
	uint32 asFound	= 0;
	ValueRep result = value_free;
	uint32 randRuns	= p.randRuns();
	double randProp	= randRuns == 0 ? p.randomPropability() : 1.0;
	uint64 maxCfl		= randRuns == 0 ? rs.next() : p.randConflicts();
	uint32 shuffle	= p.shuffleBase();
	bool	noRestartAfterModel = !p.enumerator()->allowRestarts() && !p.restartBounded();
	while (result == value_free) {
#ifdef PRINT_SEARCH_PROGRESS
		printf("c V: %7u, C: %8u, L: %8u, ML: %8u (%6.2f%%), IL: %8u\n"
			, s.numFreeVars()
			, s.numConstraints()
			, s.numLearntConstraints()
			, (uint32)maxLearnts
			, (maxLearnts/s.numConstraints())*100.0
			, (uint32)maxCfl
		);
#endif
		result = s.search(maxCfl, (uint64)maxLearnts, randProp, p.restartLocal());
		if (result == value_true) {
			if ( !backtrackFromModel(s, p) ) {
				result = value_false;
			}
			else if (maxAs == 0 || ++asFound != maxAs) {
				result = value_free;
				randRuns = 0;
				randProp = p.randomPropability();
				// Afert the first solution was found we can either disable restarts completely
				// or limit them to the current backtrack level. Full restarts are
				// no longer possible - the solver does not record found models and
				// therefore could recompute them after a complete restart.
				if (noRestartAfterModel) maxCfl = static_cast<uint64>(-1);
			}
		}
		else if (result == value_free){
			if (randRuns == 0) {
				maxCfl = rs.next();
				if (p.reduceRestart()) { s.reduceLearnts(.33f); }
				if (maxLearnts != -1.0 && maxLearnts < boundLearnts && (s.numLearntConstraints()+maxCfl) > maxLearnts) {
					maxLearnts = std::min(maxLearnts*p.reduceInc(), (double)std::numeric_limits<LitVec::size_type>::max());
				}
				if (++s.stats.restarts == shuffle) {
					shuffle += p.shuffleNext();
					s.shuffleOnNextSimplify();
				}
				
			}
			else if (--randRuns == 0) {
				maxCfl = rs.next();
				randProp = p.randomPropability();
			}	
		}
	}
	s.undoUntil(0);
	return result == value_true;
}

bool solve(Solver& s, const LitVec& assumptions, uint32 maxAs, const SolveParams& p) {
	if (s.hasConflict()) return false;
	if (!assumptions.empty() && !s.simplify()) {
		return false;
	}
	for (LitVec::size_type i = 0; i != assumptions.size(); ++i) {
		Literal p = assumptions[i];
		if (!s.isTrue(p) && (s.isFalse(p) || !(s.assume(p)&&s.stats.choices--) || !s.propagate())) {
			s.undoUntil(0);
			return false;
		}
	}
	s.setRootLevel(s.decisionLevel());
	bool ret = solve(s, maxAs, p);
	s.clearAssumptions();
	return ret;
}

}
