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
#include <clasp/include/solver.h>
#include <clasp/include/clause.h>
#include <clasp/include/util/misc_types.h>
#include <functional>
#include <stdexcept>

#ifdef _MSC_VER
#pragma warning (disable : 4355) // 'this' used in base member initializer list - intended & safe
#pragma warning (disable : 4702) // unreachable code - intended
#endif

namespace Clasp { 

uint32 randSeed_g = 1;

SatPreprocessor::~SatPreprocessor() {}
DecisionHeuristic::~DecisionHeuristic() {}
PostPropagator::PostPropagator() {}
PostPropagator::~PostPropagator() {}
void PostPropagator::reset() {}

/////////////////////////////////////////////////////////////////////////////////////////
// SelectFirst selection strategy
/////////////////////////////////////////////////////////////////////////////////////////
// selects the first free literal
Literal SelectFirst::doSelect(Solver& s) {
	for (Var i = 1; i <= s.numVars(); ++i) {
		if (s.value(i) == value_free) {
			return s.preferredLiteralByType(i);
		}
	}
	assert(!"SelectFirst::doSelect() - precondition violated!\n");
	return Literal();
}
/////////////////////////////////////////////////////////////////////////////////////////
// SelectRandom selection strategy
/////////////////////////////////////////////////////////////////////////////////////////
// Selects a random literal from all free literals.
class SelectRandom : public DecisionHeuristic {
public:
	SelectRandom() : randProp_(1.0), pos_(0) {}
	void shuffle() {
		std::random_shuffle(vars_.begin(), vars_.end(), irand);
		pos_ = 0;
	}
	void randProp(double d) {
		randProp_ = d;
	}
	double randProp() {
		return randProp_;
	}
	void endInit(const Solver& s) {
		vars_.clear();
		for (Var i = 1; i <= s.numVars(); ++i) {
			if (s.value( i ) == value_free) {
				vars_.push_back(i);
			}
		}
		pos_ = 0;
	}
private:
	Literal doSelect(Solver& s) {
		LitVec::size_type old = pos_;
		do {
			if (s.value(vars_[pos_]) == value_free) {
				return s.preferredLiteralByType(vars_[pos_]);
			}
			if (++pos_ == vars_.size()) pos_ = 0;
		} while (old != pos_);
		assert(!"SelectRandom::doSelect() - precondition violated!\n");
		return Literal();
	}
	VarVec            vars_;
	double            randProp_;
	VarVec::size_type pos_;
};
/////////////////////////////////////////////////////////////////////////////////////////
// SolverStrategies
/////////////////////////////////////////////////////////////////////////////////////////
SolverStrategies::SolverStrategies()
	: satPrePro(0)
	, heuristic(new SelectFirst) 
	, postProp(new NoPostPropagator)
	, search(use_learning)
	, cflMin(beame_minimization)
	, cflMinAntes(all_antes)
	, randomWatches(false)
	, saveProgress(false) 
	, compress_(250) {
}
PostPropagator* SolverStrategies::releasePostProp() {
	PostPropagator* p = postProp.release();
	postProp.reset(new NoPostPropagator);
	return p;
}
/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Construction/Destruction/Setup
////////////////////////////////////////////////////////////////////////////////////////
Solver::Solver() 
	: strategy_()
	, watches_()
	, front_(0)
	, binCons_(0)
	, ternCons_(0)
	, lastSimplify_(0)
	, rootLevel_(0)
	, btLevel_(0)
	, eliminated_(0)
	, randHeuristic_(0)
	, shuffle_(false) {
	addVar( Var_t::atom_body_var );
	vars_[0].value  = value_true;
	vars_[0].seen   = 1;
}

Solver::~Solver() {
	freeMem();
}


void Solver::freeMem() {
	std::for_each( constraints_.begin(), constraints_.end(), DestroyObject());
	std::for_each( learnts_.begin(), learnts_.end(), DestroyObject() );
	constraints_.clear();
	learnts_.clear();
	PodVector<WL>::destruct(watches_);
	delete randHeuristic_;
}

void Solver::reset() {
	// hopefully, no one derived from this class...
	this->~Solver();
	new (this) Solver();
}
/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Problem specification
////////////////////////////////////////////////////////////////////////////////////////
void Solver::startAddConstraints() {
	watches_.resize(vars_.size()<<1);
	trail_.reserve(numVars());
	strategy_.heuristic->startInit(*this);
}

bool Solver::endAddConstraints(bool look) {
	Antecedent::checkPlatformAssumptions();
	if (strategy_.satPrePro.get() != 0) {
		SolverStrategies::SatPrePro temp(strategy_.satPrePro.release());
		bool r = temp->preprocess();
		strategy_.satPrePro.reset(temp.release());
		if (!r) return false;
	}
	VarVec deps;
	Var v = 0;
	VarScores scores;
	while (simplify() && look && failedLiteral(v, scores,Var_t::atom_var, true, deps)) {
		deps.clear();
	}
	stats.native[0] = numConstraints();
	stats.native[1] = numBinaryConstraints();
	stats.native[2] = numTernaryConstraints();
	if (!hasConflict()) {
		strategy_.heuristic->endInit(*this);  
		if (randHeuristic_) randHeuristic_->endInit(*this);
	}
	return !hasConflict();
}

Var Solver::addVar(VarType t) {
	Var v = (Var)vars_.size();
	vars_.push_back(VarState());
	vars_.back().body = (t == Var_t::body_var);
	return v;
}

void Solver::eliminate(Var v, bool elim) {
	assert(validVar(v)); 
	if (elim && vars_[v].elim == 0) {
		assert(vars_[v].value == value_free && "Can not eliminate assigned var!\n");
		vars_[v].elim = 1;
		vars_[v].seen = 1;
		vars_[v].value= value_true; // so that the var is ignored by heuristics
		++eliminated_;
	}
	else if (!elim && vars_[v].elim == 1) {
		vars_[v].elim = 0;
		vars_[v].seen = 0;
		vars_[v].value= value_free;
		--eliminated_;
		strategy_.heuristic->resurrect(v);
	}
}

bool Solver::addUnary(Literal p) {
	if (decisionLevel() != 0) {
		impliedLits_.push_back(ImpliedLiteral(p, 0, 0));
	}
	else {
		vars_[p.var()].seen = 1;
	}
	return force(p, 0);
}

bool Solver::addBinary(Literal p, Literal q, bool asserting) {
	assert(validWatch(~p) && validWatch(~q) && "ERROR: startAddConstraints not called!");
	++binCons_;
	bwl(watches_[(~p).index()]).push_back(q);
	bwl(watches_[(~q).index()]).push_back(p);
	return !asserting || force(p, ~q);
}

bool Solver::addTernary(Literal p, Literal q, Literal r, bool asserting) {
	assert(validWatch(~p) && validWatch(~q) && validWatch(~r) && "ERROR: startAddConstraints not called!");
	assert(p != q && q != r && "ERROR: ternary clause contains duplicate literal");
	++ternCons_;
	twl(watches_[(~p).index()]).push_back(TernStub(q, r));
	twl(watches_[(~q).index()]).push_back(TernStub(p, r));
	twl(watches_[(~r).index()]).push_back(TernStub(p, q));
	return !asserting || force(p, Antecedent(~q, ~r));
}
/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Watch management
////////////////////////////////////////////////////////////////////////////////////////
uint32 Solver::numWatches(Literal p) const {
	assert( validVar(p.var()) );
	if (!validWatch(p)) return 0;
	return (uint32)watches_[p.index()].size();
}
	
bool Solver::hasWatch(Literal p, Constraint* c) const {
	if (!validWatch(p)) return false;
	const GWL& pList = gwl(watches_[p.index()]);
	return std::find(pList.begin(), pList.end(), c) != pList.end();
}

Watch* Solver::getWatch(Literal p, Constraint* c) const {
	if (!validWatch(p)) return 0;
	const GWL& pList = gwl(watches_[p.index()]);
	GWL::const_iterator it = std::find(pList.begin(), pList.end(), c);
	return it != pList.end()
		? &const_cast<Watch&>(*it)
		: 0;
}

void Solver::removeWatch(const Literal& p, Constraint* c) {
	assert(validWatch(p));
	GWL& pList = gwl(watches_[p.index()]);
	GWL::iterator it = std::find(pList.begin(), pList.end(), c);
	if (it != pList.end()) {
		pList.erase(it);
	}
}

void Solver::removeUndoWatch(uint32 dl, Constraint* c) {
	assert(dl != 0 && dl <= decisionLevel() );
	ConstraintDB& uList = levels_[dl-1].second;
	ConstraintDB::iterator it = std::find(uList.begin(), uList.end(), c);
	if (it != uList.end()) {
		*it = uList.back();
		uList.pop_back();
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Basic DPLL-functions
////////////////////////////////////////////////////////////////////////////////////////
void Solver::initRandomHeuristic(double randProp) {
	randProp = std::min(1.0, std::max(0.0, randProp));
	if (randProp == 0.0) {
		delete randHeuristic_;
		randHeuristic_ = 0;
		return;
	}
	if (!randHeuristic_) {
		randHeuristic_ = new SelectRandom();
		randHeuristic_->endInit(*this);
	}
	static_cast<SelectRandom*>(randHeuristic_)->shuffle();
	static_cast<SelectRandom*>(randHeuristic_)->randProp(randProp);
}


// removes all satisfied binary and ternary clauses as well
// as all constraints for which Constraint::simplify returned true.
bool Solver::simplify() {
	if (decisionLevel() != 0) return true;
	if (trail_.empty()) strategy_.heuristic->simplify(*this, 0);
	
	// Loop because heuristic may apply failed-literal-detection and
	// therefore force some literals.
	while (lastSimplify_ < trail_.size() || shuffle_) {
		LitVec::size_type old = lastSimplify_;
		if (!propagate()) return false;   // Top-Level-conflict
		simplifySAT();                    // removes SAT-Constraints
		assert(lastSimplify_ == trail_.size());
		strategy_.heuristic->simplify(*this, old);    
	}
	return true;
}

void Solver::simplifySAT() {
	for (; lastSimplify_ < trail_.size(); ++lastSimplify_) {
		Literal p = trail_[lastSimplify_];
		vars_[p.var()].seen = 1;            // ignore level 0 literals during conflict analysis
		simplifyShort(p);                   // remove satisfied binary- and ternary clauses
		gwl(watches_[p.index()]).clear();   // updated during propagation. 
		gwl(watches_[(~p).index()]).clear();// ~p will never be true. List is no longer relevant.
	}
	if (shuffle_) {
		std::random_shuffle(constraints_.begin(), constraints_.end(), irand);
		std::random_shuffle(learnts_.begin(), learnts_.end(), irand);
	}
	simplifyDB(constraints_);
	simplifyDB(learnts_);
	shuffle_ = false;
}

void Solver::simplifyDB(ConstraintDB& db) {
	ConstraintDB::size_type i, j, end = db.size();
	for (i = j = 0; i != end; ++i) {
		Constraint* c = db[i];
		if (c->simplify(*this, shuffle_)) { c->destroy(); }
		else                              { db[j++] = c;  }
	}
	db.erase(db.begin()+j, db.end());
}

// removes all binary clauses containing p - those are now SAT
// binary clauses containing ~p are unit and therefore likewise SAT. Those
// are removed when their second literal is processed.
// Note: Binary clauses containing p are those that watch ~p.
//
// Simplifies ternary clauses.
// Ternary clauses containing p are SAT and therefore removed.
// Ternary clauses containing ~p are now either binary or SAT. Those that
// are SAT are removed when the satisfied literal is processed. 
// All conditional binary-clauses are replaced with a real binary clause.
// Note: Ternary clauses containing p watch ~p. Those containing ~p watch p.
// Note: Those clauses are now either binary or satisfied.
void Solver::simplifyShort(Literal p) {
	bwl(watches_[p.index()]).clear(); // this list was already propagated
	LitVec& pbList = watches_[(~p).index()].bins;
	binCons_    -= (uint32)pbList.size();
	for (LitVec::size_type i = 0; i < pbList.size(); ++i) {
		remove_first_if(watches_[(~pbList[i]).index()].bins, std::bind2nd(std::equal_to<Literal>(), p));
	}
	pbList.clear();

	// remove every ternary clause containing p -> clause is satisfied
	TWL& ptList = watches_[(~p).index()].terns;
	ternCons_   -= (uint32)ptList.size();
	for (LitVec::size_type i = 0; i < ptList.size(); ++i) {
		remove_first_if(watches_[(~ptList[i].first).index()].terns, PairContains<Literal>(p));
		remove_first_if(watches_[(~ptList[i].second).index()].terns, PairContains<Literal>(p));
	}
	ptList.clear();
	// transform ternary clauses containing ~p to binary clause
	TWL& npList = watches_[p.index()].terns;
	for (LitVec::size_type i = 0; i < npList.size(); ++i) {
		const Literal& q = npList[i].first;
		const Literal& r = npList[i].second;
		if (value(q.var()) == value_free && value(r.var()) == value_free) {
			// clause is binary on dl 0
			--ternCons_;
			remove_first_if(watches_[(~q).index()].terns, PairContains<Literal>(~p));
			remove_first_if(watches_[(~r).index()].terns, PairContains<Literal>(~p));
			addBinary(q, r, false);
		}
		// else: clause is SAT and removed when the satisfied literal is processed
	}
	npList.clear();
}
bool Solver::force(const Literal& p, const Antecedent& c) {
	assert((!hasConflict() || isTrue(p)) && !eliminated(p.var()));
	const Var var = p.var();
	if (value(var) == value_free) {
		vars_[var].assign(trueValue(p), c, decisionLevel());
		trail_.push_back(p);
	}
	else if (value(var) == falseValue(p)) {   // conflict
		if (strategy_.search != SolverStrategies::no_learning && !c.isNull()) {
			c.reason(p, conflict_);
		}
		conflict_.push_back(~p);
		return false;
	}
	return true;
}

bool Solver::assume(const Literal& p) {
	assert( value(p.var()) == value_free );
	++stats.choices;
	levels_.push_back(LevelInfo((uint32)trail_.size(), ConstraintDB()));
	levConflicts_.push_back( (uint32)stats.conflicts );
	return force(p, Antecedent());  // always true
}

bool Solver::propagate() {
	if (!unitPropagate() || !strategy_.postProp->propagate()) {
		front_ = trail_.size();
		strategy_.postProp->reset();
		return false;
	}
	assert(queueSize() == 0);
	return true;
}

bool Solver::unitPropagate() {
	assert(!hasConflict());
	while (front_ < trail_.size()) {
		const Literal& p = trail_[front_++];
		uint32 idx = p.index();
		LitVec::size_type i, end;
		WL& wl = watches_[idx];
		// first, do binary BCP...    
		for (i = 0, end = wl.bins.size(); i != end; ++i) {
			if (!isTrue(wl.bins[i]) && !force(wl.bins[i], p)) {
				return false;
			}
		}
		// then, do ternary BCP...
		for (i = 0, end = wl.terns.size(); i != end; ++i) {
			Literal q = wl.terns[i].first;
			Literal r = wl.terns[i].second;
			if (isTrue(r) || isTrue(q)) continue;
			if (isFalse(r) && !force(q, Antecedent(p, ~r))) {
				return false;
			}
			else if (isFalse(q) && !force(r, Antecedent(p, ~q))) {
				return false;
			}
		}
		// and finally do general BCP
		if (!wl.gens.empty()) {
			GWL& gWL = gwl(wl);
			Constraint::PropResult r;
			LitVec::size_type j;
			for (j = 0, i = 0, end = gWL.size(); i != gWL.size(); ) {
				Watch& w = gWL[i++];
				r = w.propagate(*this, p);
				if (r.second) { // keep watch
					gWL[j++] = w;
				}
				if (!r.first) {
					while (i != end) {
						gWL[j++] = gWL[i++];
					}
					gWL.erase(gWL.begin()+j, gWL.end());
					return false;
				}
			}
			gWL.erase(gWL.begin()+j, gWL.end());
		}
	}
	return true;
}

bool Solver::failedLiteral(Var& var, VarScores& scores, VarType types, bool uniform, VarVec& deps) {
	assert(validVar(var));
	scores.resize( vars_.size() );
	deps.clear();
	Var oldVar = var;
	bool cfl = false;
	do {
		if ( (type(var) & types) != 0 && value(var) == value_free ) {
			Literal p = preferredLiteralByType(var);
			bool testBoth = uniform || type(var) == Var_t::atom_body_var;
			if (!scores[var].seen(p) && !lookahead(p, scores,deps, types, true)) {
				cfl = true;
				break;
			}
			if (testBoth && !scores[var].seen(~p) && !lookahead(~p, scores,deps, types, true)) {
				cfl = true;
				break;
			}
		}
		if (++var > numVars()) var = 0;
	} while (!cfl && var != oldVar);
	if (cfl) {
		scores[var].clear();
		for (VarVec::size_type i = 0; i < deps.size(); ++i) {
			scores[deps[i]].clear();
		}
		deps.clear();
	}   
	return cfl;
}

bool Solver::resolveConflict() {
	assert(hasConflict());
	++stats.conflicts;
	if (decisionLevel() > rootLevel_) {
		if (decisionLevel() != btLevel_ && strategy_.search != SolverStrategies::no_learning) {
			ClauseCreator uip(this);
			uint32 uipLevel = analyzeConflict(uip);
			updateJumps(stats, decisionLevel(), uipLevel, btLevel_);
			undoUntil( uipLevel );
			bool ret = uip.end();
			if (uipLevel < btLevel_) {
				assert(decisionLevel() == btLevel_);
				// logically the asserting clause is unit on uipLevel but the backjumping
				// is bounded by btLevel_ thus the uip is asserted on that level. 
				// We store enough information so that the uip can be re-asserted once we backtrack below btLevel.
				impliedLits_.push_back( ImpliedLiteral( trail_.back(), uipLevel, reason(trail_.back().var()) ));
			}
			assert(ret);
			return ret;
		}
		else {
			return backtrack();
		}
	}
	return false;
}

bool Solver::backtrack() {
	if (decisionLevel() == rootLevel_) return false;
	Literal lastChoiceInverted = ~decision(decisionLevel());
	btLevel_ = decisionLevel() - 1;
	undoUntil(btLevel_);
	bool r = force(lastChoiceInverted, 0);
	ImpliedLits::size_type j = 0;
	for (ImpliedLits::size_type i = 0; i < impliedLits_.size(); ++i) {
		r = r && force(impliedLits_[i].lit, impliedLits_[i].ante);
		if (impliedLits_[i].level != btLevel_) {
			impliedLits_[j++] = impliedLits_[i];
		}
	}
	impliedLits_.erase(impliedLits_.begin()+j, impliedLits_.end());
	return r || backtrack();
}

void Solver::undoUntil(uint32 level) {
	assert(btLevel_ >= rootLevel_);
	level = std::max( level, btLevel_ );
	if (level >= decisionLevel()) return;
	conflict_.clear();
	strategy_.heuristic->undoUntil( *this, levels_[level].first);
	bool sp = false;
	do {
		undoLevel(sp);
		sp = strategy_.saveProgress;
	} while (decisionLevel() > level);
}

bool Solver::lookahead(Literal p, VarScores& scores, VarVec& deps, VarType types, bool addDeps) {
	LitVec::size_type old = trail_.size();
	bool ok = assume(p) && propagate();
	if (ok) {
		scores[p.var()].setScore(p, trail_.size() - old);
		if (addDeps) {
			for (; old < trail_.size(); ++old) {
				Var v = trail_[old].var();
				if ( (type(v) & types) != 0) {
					if (!scores[v].seen()) { deps.push_back(v); }
					scores[v].setSeen(trail_[old]);
				}
			}
		}
		undoUntil(decisionLevel()-1);
	}
	else {
		resolveConflict();
	}
	--stats.choices;
	return ok;
}

uint32 Solver::estimateBCP(const Literal& p, int rd) const {
	assert(front_ == trail_.size());
	if (value(p.var()) != value_free) return 0;
	LitVec::size_type i = front_;
	Solver& self = const_cast<Solver&>(*this);
	self.vars_[p.var()].value = trueValue(p);
	self.trail_.push_back(p);
	do {
		Literal x = trail_[i++];  
		const LitVec& xList = watches_[x.index()].bins;
		for (LitVec::size_type k = 0; k < xList.size(); ++k) {
			Literal y = xList[k];
			if (value(y.var()) == value_free) {
				self.vars_[y.var()].value = trueValue(y);
				self.trail_.push_back(y);
			}
		}
	} while (i < trail_.size() && rd-- != 0);
	i = trail_.size() - front_;
	while (trail_.size() != front_) {
		self.vars_[self.trail_.back().var()].value = value_free;
		self.trail_.pop_back();
	}
	return (uint32)i;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Private helper functions
////////////////////////////////////////////////////////////////////////////////////////
// removes the current decision level
void Solver::undoLevel(bool sp) {
	assert(decisionLevel() != 0);
	for (LitVec::size_type count = trail_.size() - levels_.back().first; count != 0; --count) {
		Literal& p = trail_.back();
		vars_[p.var()].undo(sp);
		trail_.pop_back();
	}
	const ConstraintDB& undoList = levels_.back().second;
	for (ConstraintDB::size_type i = 0, end = undoList.size(); i != end; ++i) {
		undoList[i]->undoLevel(*this);
	}
	levels_.pop_back();
	levConflicts_.pop_back();
	front_ = trail_.size();
}

// Computes the First-UIP clause and stores it in outClause.
// outClause[0] is the asserting literal (inverted UIP)
// Returns the dl on which the conflict clause is asserting.
uint32 Solver::analyzeConflict(ClauseCreator& outClause) {
	// must be called here, because we unassign vars during analyzeConflict
	strategy_.heuristic->undoUntil( *this, levels_.back().first );
	uint32 onLevel  = 0;        // number of literals from the current DL in resolvent
	uint32 abstr    = 0;        // abstraction of DLs in cc_
	Literal p;
	strategy_.heuristic->updateReason(*this, conflict_, p);
	cc_.clear();
	for (;;) {
		for (LitVec::size_type i = 0; i != conflict_.size(); ++i) {
			Literal& q = conflict_[i];
			if (vars_[q.var()].seen == 0) {
				assert(isTrue(q) && "Invalid literal in reason set!");
				uint32 cl = level(q.var());
				assert(cl > 0 && "Simplify not called on Top-Level - seen-flag not set!");
				vars_[q.var()].seen = 1;
				if (cl == decisionLevel()) {
					++onLevel;
				}
				else {
					cc_.push_back(~q);
					abstr |= (1 << (cl & 31));
				}
			}
		}
		// search for the last assigned literal that needs to be analyzed...
		while (vars_[trail_.back().var()].seen == 0) {
			Literal& p = trail_.back();
			vars_[p.var()].undo(false);
			trail_.pop_back();
		}
		p = trail_.back();
		vars_[p.var()].seen = 0;
		conflict_.clear();
		if (--onLevel == 0) {
			break;
		}
		reason(p.var()).reason(p, conflict_);
		strategy_.heuristic->updateReason(*this, conflict_, p);
	}
	outClause.reserve( cc_.size()+1 );
	outClause.startAsserting(Constraint_t::learnt_conflict, ~p);  // store the 1-UIP
	minimizeConflictClause(outClause, abstr);
	// clear seen-flag of all literals that are not from the current dl.
	for (LitVec::size_type i = 0; i != cc_.size(); ++i) {
		vars_[cc_[i].var()].seen = 0;
	}
	cc_.clear();
	assert( decisionLevel() == level(p.var()));
	return outClause.size() != 1 
		? level(outClause[outClause.secondWatch()].var()) 
		: 0;
}

void Solver::minimizeConflictClause(ClauseCreator& outClause, uint32 abstr) {
	uint32 m = strategy_.cflMinAntes;
	LitVec::size_type t = trail_.size();
	for (LitVec::size_type i = 0; i != cc_.size(); ++i) { 
		if (reason(cc_[i].var()).isNull() || ((reason(cc_[i].var()).type()+1) & m) == 0  || !analyzeLitRedundant(cc_[i], abstr)) {
			outClause.add(cc_[i]);
		}
	}
	while (trail_.size() != t) {
		vars_[trail_.back().var()].seen = 0;
		trail_.pop_back();
	}
}

bool Solver::analyzeLitRedundant(Literal p, uint32 abstr) {
	if (strategy_.cflMin == SolverStrategies::beame_minimization) {
		conflict_.clear(); reason(p.var()).reason(~p, conflict_);
		for (LitVec::size_type i = 0; i != conflict_.size(); ++i) {
			if (vars_[conflict_[i].var()].seen == 0) return false;
		}
		return true;
	}
	// else: een_minimization
	LitVec::size_type start = trail_.size();
	trail_.push_back(~p);
	for (LitVec::size_type f = start; f != trail_.size(); ) {
		p = trail_[f++];
		conflict_.clear();
		reason(p.var()).reason(p, conflict_);
		for (LitVec::size_type i = 0; i != conflict_.size(); ++i) {
			p = conflict_[i];
			if (vars_[p.var()].seen == 0) {
				if (!reason(p.var()).isNull() && ((1<<(level(p.var())&31)) & abstr) != 0) {
					vars_[p.var()].seen = 1;
					trail_.push_back(p);
				}
				else {
					while (trail_.size() != start) {
						vars_[trail_.back().var()].seen = 0;
						trail_.pop_back();
					}
					return false;
				}
			}
		}
	}
	return true;
}

// Selects next branching literal. Use user-supplied heuristic if rand() < randProp.
// Otherwise makes a random choice.
// Returns false if assignment is total.
bool Solver::decideNextBranch() {
	DecisionHeuristic* heu = strategy_.heuristic.get();
	if (randHeuristic_ && drand() < static_cast<SelectRandom*>(randHeuristic_)->randProp()) {
		heu = randHeuristic_;
	}
	return heu->select(*this);
}

// Remove upto maxRem% of the learnt nogoods.
// Keep those that are locked or have a high activity.
void Solver::reduceLearnts(float maxRem) {
	uint32 oldS = numLearntConstraints();
	ConstraintDB::size_type i, j = 0;
	if (maxRem < 1.0f) {    
		LitVec::size_type remMax = static_cast<LitVec::size_type>(numLearntConstraints() * std::min(1.0f, std::max(0.0f, maxRem)));
		uint64 actSum = 0;
		for (i = 0; i != learnts_.size(); ++i) {
			actSum += static_cast<LearntConstraint*>(learnts_[i])->activity();
		}
		double actThresh = (actSum / (double) numLearntConstraints()) * 1.5;
		for (i = 0; i != learnts_.size(); ++i) {
			LearntConstraint* c = static_cast<LearntConstraint*>(learnts_[i]);
			if (remMax == 0 || c->locked(*this) || c->activity() > actThresh) {
				c->decreaseActivity();
				learnts_[j++] = c;
			}
			else {
				--remMax;
				c->removeWatches(*this);
				c->destroy();
			}
		}
	}
	else {
		// remove all nogoods that are not locked
		for (i = 0; i != learnts_.size(); ++i) {
			LearntConstraint* c = static_cast<LearntConstraint*>(learnts_[i]);
			if (c->locked(*this)) {
				c->decreaseActivity();
				learnts_[j++] = c;
			}
			else {
				c->removeWatches(*this);
				c->destroy();
			}
		}
	}
	learnts_.erase(learnts_.begin()+j, learnts_.end());
	stats.deleted += oldS - numLearntConstraints();
}
/////////////////////////////////////////////////////////////////////////////////////////
// The basic DPLL-like search-function
/////////////////////////////////////////////////////////////////////////////////////////
ValueRep Solver::search(uint64 maxConflicts, uint64 maxLearnts, double randProp, bool localR) {
	initRandomHeuristic(randProp);
	if (!simplify()) { return value_false; }
	do {
		while (!propagate()) {
			if (!resolveConflict() || (decisionLevel() == 0 && !simplify())) {
				return value_false;
			}
			if ((!localR && --maxConflicts == 0) || (localR && localRestart(maxConflicts))) {
				undoUntil(0);
				return value_free;  
			}
		}
		if (numLearntConstraints()>(uint32)maxLearnts) { reduceLearnts(.75f); }
	} while (decideNextBranch());
	assert(numFreeVars() == 0);
	++stats.models;
	updateModels(stats, decisionLevel());
	if (strategy_.satPrePro.get()) {
		strategy_.satPrePro->extendModel(vars_);
	}
	return value_true;
}
}
