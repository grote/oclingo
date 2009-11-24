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
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/util/misc_types.h>
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
	SelectRandom() : randFreq_(1.0), pos_(0) {}
	void shuffle() {
		std::random_shuffle(vars_.begin(), vars_.end(), irand);
		pos_ = 0;
	}
	void   randFreq(double d) { randFreq_ = d; }
	double randFreq() const   { return randFreq_; }
	void   endInit(Solver& s) {
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
				Literal l = preferredLiteral(s, vars_[pos_]);
				return l != posLit(0)
					? l
					: s.preferredLiteralByType(vars_[pos_]);
			}
			if (++pos_ == vars_.size()) pos_ = 0;
		} while (old != pos_);
		assert(!"SelectRandom::doSelect() - precondition violated!\n");
		return Literal();
	}
	VarVec            vars_;
	double            randFreq_;
	VarVec::size_type pos_;
};
/////////////////////////////////////////////////////////////////////////////////////////
// Post propagator list
/////////////////////////////////////////////////////////////////////////////////////////
Solver::PPList::PPList() : head(0), saved(0) { }
Solver::PPList::~PPList() {
	for (PostPropagator* r = head; r;) {
		PostPropagator* t = r;
		r = r->next;
		t->destroy();
	}
}
void Solver::PPList::add(PostPropagator* p) {
	assert(p && p->next == 0);
	uint32 prio = p->priority();
	if (!head || prio == PostPropagator::priority_highest || prio < head->priority()) {
		p->next = head;
		head    = p;
	}
	else {
		for (PostPropagator* r = head; ; r = r->next) {
			if (r->next == 0) {
				r->next = p; break;
			}
			else if (prio < r->next->priority()) {
				p->next = r->next;
				r->next = p;
				break;
			}				
		}
	}
	saved = head;
}
void Solver::PPList::remove(PostPropagator* p) {
	assert(p);
	if (!head) return;
	if (p == head) {
		head = head->next;
		p->next = 0;
	}
	else {
		for (PostPropagator* r = head; ; r = r->next) {
			if (r->next == p) {
				r->next = r->next->next;
				p->next = 0;
				break;
			}
		}
	}
	saved = head;
}
bool Solver::PPList::propagate(Solver& s, PostPropagator* x) {
	if (x == head) return true;
	PostPropagator* p = head, *t;
	do {
		// just in case t removes itself from the list
		// during propagateFixpoint
		t = p;
		p = p->next;
		if (!t->propagateFixpoint(s)) { return false; }
	}
	while (p != x);
	return true;
}
bool PostPropagator::propagateFixpoint(Solver& s) {
	bool ok = propagate(s);
	while (ok && s.queueSize() > 0) {
		ok = s.propagateUntil(this) && propagate(s);
	}
	return ok;
}
void Solver::PPList::reset()            { for (PostPropagator* r = head; r; r = r->next) { r->reset(); } }
bool Solver::PPList::isModel(Solver& s) {
	for (PostPropagator* r = head; r; r = r->next) {
		if (!r->isModel(s)) return false;
	}
	return true;
}
bool Solver::PPList::nextSymModel(Solver& s, bool expand) {
	for (; saved; saved = saved->next) {
		if (saved->nextSymModel(s, expand)) return true;
	}
	return false;
}
/////////////////////////////////////////////////////////////////////////////////////////
// SolverStrategies
/////////////////////////////////////////////////////////////////////////////////////////
SolverStrategies::SolverStrategies()
	: satPrePro(0)
	, heuristic(new SelectFirst) 
	, symTab(0)
	, search(use_learning)
	, saveProgress(0) 
	, cflMinAntes(all_antes)
	, strengthenRecursive(false)
	, randomWatches(false)
	, compress_(250) {
}
/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Construction/Destruction/Setup
////////////////////////////////////////////////////////////////////////////////////////
Solver::Solver() 
	: strategy_()
	, randHeuristic_(0)
	, levConflicts_(0)
	, undoHead_(0)
	, front_(0)
	, units_(0)
	, binCons_(0)
	, ternCons_(0)
	, lastSimplify_(0)
	, rootLevel_(0)
	, btLevel_(0)
	, eliminated_(0)
	, shuffle_(false) {
	// every solver contains a special sentinel var that is always true
	Var sentVar = addVar( Var_t::atom_body_var );
	vars_.assign(sentVar, value_true);
	markSeen(sentVar);
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
	delete levConflicts_;
	delete randHeuristic_;
	// free undo lists
	// first those still in use
	for (TrailLevels::size_type i = 0; i != levels_.size(); ++i) {
		delete levels_[i].second;
	}
	// then those in the free list
	for (ConstraintDB* x = undoHead_; x; ) {
		ConstraintDB* t = x;
		x = (ConstraintDB*)x->front();
		delete t;
	}
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
	watches_.resize(vars_.numVars()<<1);
	// pre-allocate some memory
	trail_.reserve(numVars());
	levels_.reserve(25);
	for (uint32 i = 0; i != 25; ++i) { undoFree(new ConstraintDB(10)); }
	strategy_.heuristic->startInit(*this);
	if (strategies().satPrePro.get()) {
		strategies().satPrePro->setSolver(*this);
	}

	stats.problem.reset();
	stats.problem.vars = numVars();
}

bool Solver::endAddConstraints() {
	Antecedent::checkPlatformAssumptions();
	if (strategy_.satPrePro.get() != 0) {
		SolverStrategies::SatPrePro temp(strategy_.satPrePro.release());
		bool r = temp->preprocess();
		strategy_.satPrePro.reset(temp.release());
		stats.problem.eliminated = numEliminatedVars();
		if (!r) return false;
	}
	if (lastSimplify_ == trail_.size()) {
		// since there may be new constraints (e.g. added by the SAT preprocessor)
		// we force simplification (e.g. failed-literal detection if enabled) even
		// if there are no new unit literals
		strategy_.heuristic->simplify(*this, 0);
	}
	if (!simplify()) {
		return false;
	}
	strategy_.heuristic->endInit(*this);
	stats.problem.constraints[0] = numConstraints();
	stats.problem.constraints[1] = numBinaryConstraints();
	stats.problem.constraints[2] = numTernaryConstraints();
	if (randHeuristic_) randHeuristic_->endInit(*this);
	return true;
}

uint32 Solver::problemComplexity() const {
	uint32 r = binCons_ + ternCons_;
	for (uint32 i = 0; i != constraints_.size(); ++i) {
		r += constraints_[i]->estimateComplexity(*this);
	}
	return r;
}

void Solver::reserveVars(uint32 numVars) {
	vars_.reserve(numVars);
	levelSeen_.reserve(numVars);
	reason_.reserve(numVars);
}

Var Solver::addVar(VarType t, bool eq) {
	Var v = vars_.numVars();
	vars_.add(t == Var_t::body_var);
	if (eq) vars_.setEq(v, true);
	levelSeen_.push_back(0);
	reason_.push_back(Antecedent());
	return v;
}

void Solver::eliminate(Var v, bool elim) {
	assert(validVar(v)); 
	if (elim && !eliminated(v)) {
		assert(value(v) == value_free && "Can not eliminate assigned var!\n");
		vars_.setEliminated(v, true);
		markSeen(v);
		// so that the var is ignored by heuristics
		vars_.assign(v, value_true);
		++eliminated_;
	}
	else if (!elim && eliminated(v)) {
		vars_.setEliminated(v, false);
		clearSeen(v);
		vars_.undo(v);
		--eliminated_;
		strategy_.heuristic->resurrect(v);
	}
}

bool Solver::addUnary(Literal p) {
	if (decisionLevel() != 0) {
		impliedLits_.push_back(ImpliedLiteral(p, 0, posLit(0)));
	}
	return force(p, posLit(0));
}

bool Solver::addBinary(Literal p, Literal q, bool asserting) {
	assert(validWatch(~p) && validWatch(~q) && "ERROR: startAddConstraints not called!");
	++binCons_;
	watches_[(~p).index()].push_back(q);
	watches_[(~q).index()].push_back(p);
	return !asserting || force(p, ~q);
}

bool Solver::addTernary(Literal p, Literal q, Literal r, bool asserting) {
	assert(validWatch(~p) && validWatch(~q) && validWatch(~r) && "ERROR: startAddConstraints not called!");
	assert(p != q && q != r && "ERROR: ternary clause contains duplicate literal");
	++ternCons_;
	watches_[(~p).index()].push_back(TernStub(q, r));
	watches_[(~q).index()].push_back(TernStub(p, r));
	watches_[(~r).index()].push_back(TernStub(p, q));
	return !asserting || force(p, Antecedent(~q, ~r));
}
bool Solver::clearAssumptions()  {
	rootLevel_ = btLevel_ = 0;
	undoUntil(0);
	assert(decisionLevel() == 0);
	if (!hasConflict()) {
		for (ImpliedLits::size_type i = 0; i != impliedLits_.size(); ++i) {
			if (impliedLits_[i].level == 0 && !force(impliedLits_[i].lit, impliedLits_[i].ante)) {
				return false;
			}
		}
	}
	impliedLits_.clear();
	return simplify();
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
	const GWL& pList = watches_[p.index()].genW;
	return std::find(pList.begin(), pList.end(), c) != pList.end();
}

Watch* Solver::getWatch(Literal p, Constraint* c) const {
	if (!validWatch(p)) return 0;
	const GWL& pList = watches_[p.index()].genW;
	GWL::const_iterator it = std::find(pList.begin(), pList.end(), c);
	return it != pList.end()
		? &const_cast<Watch&>(*it)
		: 0;
}

void Solver::removeWatch(const Literal& p, Constraint* c) {
	assert(validWatch(p));
	GWL& pList = watches_[p.index()].genW;
	GWL::iterator it = std::find(pList.begin(), pList.end(), c);
	if (it != pList.end()) {
		pList.erase(it);
	}
}

void Solver::removeUndoWatch(uint32 dl, Constraint* c) {
	assert(dl != 0 && dl <= decisionLevel() );
	if (levels_[dl-1].second) {
		ConstraintDB& uList = *levels_[dl-1].second;
		ConstraintDB::iterator it = std::find(uList.begin(), uList.end(), c);
		if (it != uList.end()) {
			*it = uList.back();
			uList.pop_back();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Basic DPLL-functions
////////////////////////////////////////////////////////////////////////////////////////
void Solver::initRandomHeuristic(double randFreq) {
	randFreq = std::min(1.0, std::max(0.0, randFreq));
	if (randFreq == 0.0) {
		delete randHeuristic_;
		randHeuristic_ = 0;
		return;
	}
	if (!randHeuristic_) {
		randHeuristic_ = new SelectRandom();
		randHeuristic_->endInit(*this);
	}
	static_cast<SelectRandom*>(randHeuristic_)->shuffle();
	static_cast<SelectRandom*>(randHeuristic_)->randFreq(randFreq);
}


// removes all satisfied binary and ternary clauses as well
// as all constraints for which Constraint::simplify returned true.
bool Solver::simplify() {
	if (decisionLevel() != 0) return true;
	if (hasConflict())        return false;
	// loop because DecisionHeuristic::simplify() may apply failed-literal
	// detection and hence may derive new unit literals
	while (lastSimplify_ != trail_.size()) {
		LitVec::size_type old = lastSimplify_;
		if (!propagate()) return false;   // Top-Level-conflict
		simplifySAT();                    // removes SAT-Constraints
		assert(lastSimplify_ == trail_.size());
		strategy_.heuristic->simplify(*this, old);
	}
	if (shuffle_) {
		simplifySAT();
	}
	return true;
}

void Solver::simplifySAT() {
	for (; lastSimplify_ < trail_.size(); ++lastSimplify_) {
		simplifyShort(trail_[lastSimplify_] );// remove satisfied binary- and ternary clauses
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
	WL& pList     = watches_[p.index()];
	WL& negPList  = watches_[(~p).index()];
	releaseVec( pList.binW ); // this list was already propagated
	binCons_    -= (uint32)negPList.binW.size();
	for (LitVec::size_type i = 0; i < negPList.binW.size(); ++i) {
		remove_first_if(watches_[(~negPList.binW[i]).index()].binW, std::bind2nd(std::equal_to<Literal>(), p));
	}
	releaseVec(negPList.binW);
	
	// remove every ternary clause containing p -> clause is satisfied
	TWL& ptList = negPList.ternW;
	ternCons_   -= (uint32)ptList.size();
	for (LitVec::size_type i = 0; i < ptList.size(); ++i) {
		remove_first_if(watches_[(~ptList[i].first).index()].ternW, PairContains<Literal>(p));
		remove_first_if(watches_[(~ptList[i].second).index()].ternW, PairContains<Literal>(p));
	}
	releaseVec(ptList);
	// transform ternary clauses containing ~p to binary clause
	TWL& npList = pList.ternW;
	for (LitVec::size_type i = 0; i < npList.size(); ++i) {
		const Literal& q = npList[i].first;
		const Literal& r = npList[i].second;
		if (value(q.var()) == value_free && value(r.var()) == value_free) {
			// clause is binary on dl 0
			--ternCons_;
			remove_first_if(watches_[(~q).index()].ternW, PairContains<Literal>(~p));
			remove_first_if(watches_[(~r).index()].ternW, PairContains<Literal>(~p));
			addBinary(q, r, false);
		}
		// else: clause is SAT and removed when the satisfied literal is processed
	}
	releaseVec(npList);
	releaseVec(pList.genW);     // updated during propagation. 
	releaseVec(negPList.genW);  // ~p will never be true. List is no longer relevant.
}
bool Solver::force(const Literal& p, const Antecedent& c) {
	assert((!hasConflict() || isTrue(p)) && !eliminated(p.var()));
	const Var var = p.var();
	if (value(var) == value_free) {
		vars_.assign(var, trueValue(p));
		levelSeen_[var] = decisionLevel() << 2;
		reason_[var]    = c;
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
	++stats.solve.choices;
	levels_.push_back(LevelInfo((uint32)trail_.size(), 0));
	if (levConflicts_) levConflicts_->push_back( (uint32)stats.solve.conflicts );
	return force(p, Antecedent());  // always true
}

bool Solver::propagate() {
	if (unitPropagate() && post_.propagate(*this, 0)) {
		assert(queueSize() == 0);
		return true;
	}
	front_ = trail_.size();
	post_.reset();
	return false;
}


uint32 Solver::mark(uint32 s, uint32 e) {
	while (s != e) { markSeen(trail_[s++].var()); }
	return e;
}

bool Solver::unitPropagate() {
	assert(!hasConflict());
	while (front_ < trail_.size()) {
		const Literal& p = trail_[front_++];
		uint32 idx = p.index();
		WL& wl = watches_[idx];
		LitVec::size_type i, bEnd = wl.binW.size(), tEnd = wl.ternW.size(), gEnd = wl.genW.size();
		// first, do binary BCP...    
		for (i = 0; i != bEnd; ++i) {
			if (!isTrue(wl.binW[i]) && !force(wl.binW[i], p)) {
				return false;
			}
		}
		// then, do ternary BCP...
		for (i = 0; i != tEnd; ++i) {
			Literal q = wl.ternW[i].first;
			Literal r = wl.ternW[i].second;
			if (isTrue(r) || isTrue(q)) continue;
			if (isFalse(r) && !force(q, Antecedent(p, ~r))) {
				return false;
			}
			else if (isFalse(q) && !force(r, Antecedent(p, ~q))) {
				return false;
			}
		}
		// and finally do general BCP
		if (gEnd != 0) {
			GWL& gWL = wl.genW;
			Constraint::PropResult r;
			LitVec::size_type j;
			for (j = 0, i = 0; i != gEnd; ) {
				Watch& w = gWL[i++];
				r = w.propagate(*this, p);
				if (r.second) { // keep watch
					gWL[j++] = w;
				}
				if (!r.first) {
					while (i != gEnd) {
						gWL[j++] = gWL[i++];
					}
					shrinkVecTo(gWL, j);
					return false;
				}
			}
			shrinkVecTo(gWL, j);
		}
	}
	return decisionLevel() > 0 || (units_=mark(units_, uint32(front_))) == front_;
}

bool Solver::failedLiteral(Var& var, VarScores& scores, VarType types, VarVec& deps) {
	assert(validVar(var));
	scores.resize( vars_.numVars() );
	deps.clear();
	Var oldVar   = var;
	bool uniform = (types !=  Var_t::atom_body_var);
	bool cfl     = false;
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
	++stats.solve.conflicts;
	if (decisionLevel() > rootLevel_) {
		if (decisionLevel() != btLevel_ && strategy_.search != SolverStrategies::no_learning) {
			uint32 sw;
			uint32 uipLevel = analyzeConflict(sw);
			stats.solve.updateJumps(decisionLevel(), uipLevel, btLevel_);
			undoUntil( uipLevel );
			bool ret = ClauseCreator::createClause(*this, Constraint_t::learnt_conflict, cc_, sw);
			if (uipLevel < btLevel_) {
				assert(decisionLevel() == btLevel_);
				// logically the asserting clause is unit on uipLevel but the backjumping
				// is bounded by btLevel_ thus the uip is asserted on that level. 
				// We store enough information so that the uip can be re-asserted once we backtrack below jLevel.
				impliedLits_.push_back( ImpliedLiteral( trail_.back(), uipLevel, reason(trail_.back()) ));
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
	bool sp = strategy_.saveProgress > 0 && (decisionLevel() - level) > (uint32)strategy_.saveProgress;
	undoLevel(false);
	while (decisionLevel() != level) {
		undoLevel(sp);
	}
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
	--stats.solve.choices;
	return ok;
}

uint32 Solver::estimateBCP(const Literal& p, int rd) const {
	assert(front_ == trail_.size());
	if (value(p.var()) != value_free) return 0;
	LitVec::size_type i = front_;
	Solver& self = const_cast<Solver&>(*this);
	self.vars_.assign(p.var(), trueValue(p));
	self.trail_.push_back(p);
	do {
		Literal x = trail_[i++];  
		const BWL& xList = watches_[x.index()].binW;
		for (LitVec::size_type k = 0; k < xList.size(); ++k) {
			Literal y = xList[k];
			if (value(y.var()) == value_free) {
				self.vars_.assign(y.var(), trueValue(y));
				self.trail_.push_back(y);
			}
		}
	} while (i < trail_.size() && rd-- != 0);
	i = trail_.size() - front_;
	while (trail_.size() != front_) {
		self.vars_.undo(self.trail_.back().var());
		self.trail_.pop_back();
	}
	return (uint32)i;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Private helper functions
////////////////////////////////////////////////////////////////////////////////////////
Solver::ConstraintDB* Solver::allocUndo(Constraint* c) {
	if (undoHead_ == 0) {
		return new ConstraintDB(1, c);
	}
	assert(undoHead_->size() == 1);
	ConstraintDB* r = undoHead_;
	undoHead_ = (ConstraintDB*)undoHead_->front();
	r->clear();
	r->push_back(c);
	return r;
}
void Solver::undoFree(ConstraintDB* x) {
	// maintain a single-linked list of undo lists
	x->clear();
	x->push_back((Constraint*)undoHead_);
	undoHead_ = x;
}
void Solver::saveValueAndUndo(Literal p) {
	vars_.setPrefValue(p.var(), trueValue(p));
	vars_.undo(p.var());
}
// removes the current decision level
void Solver::undoLevel(bool sp) {
	assert(decisionLevel() != 0);
	LitVec::size_type numUndo = trail_.size() - levels_.back().first;
	assert(numUndo > 0 && "Decision level must not be empty!");
	if (!sp) {
		while (numUndo-- != 0) {
			vars_.undo(trail_.back().var());
			trail_.pop_back();
		}
	}
	else {
		while (numUndo-- != 0) {
			saveValueAndUndo(trail_.back());
			trail_.pop_back();
		}
	}
	if (levels_.back().second) {
		const ConstraintDB& undoList = *levels_.back().second;
		for (ConstraintDB::size_type i = 0, end = undoList.size(); i != end; ++i) {
			undoList[i]->undoLevel(*this);
		}
		undoFree(levels_.back().second);
	}
	levels_.pop_back();
	if (levConflicts_) levConflicts_->pop_back();
	front_ = trail_.size();
}

// Computes the First-UIP clause and stores it in cc_, where cc_[0] is the asserting literal (inverted UIP).
// Returns the dl on which cc_ is asserting and stores the position of a literal 
// from this level in secondWatch.
uint32 Solver::analyzeConflict(uint32& secondWatch) {
	// must be called here, because we unassign vars during analyzeConflict
	strategy_.heuristic->undoUntil( *this, levels_.back().first );
	uint32 onLevel  = 0;        // number of literals from the current DL in resolvent
	uint32 abstr    = 0;        // abstraction of DLs in cc_
	Literal p;                  // literal to be resolved out next
	cc_.assign(1, p);           // will later be replaced with asserting literal
	strategy_.heuristic->updateReason(*this, conflict_, p);
	for (;;) {
		for (LitVec::size_type i = 0; i != conflict_.size(); ++i) {
			Literal& q = conflict_[i];
			if (!seen(q.var())) {
				assert(isTrue(q) && "Invalid literal in reason set!");
				uint32 cl = level(q.var());
				assert(cl > 0 && "Unit literal not marked!");
				markSeen(q.var());
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
		while (!seen(trail_.back().var())) {
			vars_.undo(trail_.back().var());
			trail_.pop_back();
		}
		p = trail_.back();
		clearSeen(p.var());
		conflict_.clear();
		if (--onLevel == 0) {
			break;
		}
		reason(p).reason(p, conflict_);
		strategy_.heuristic->updateReason(*this, conflict_, p);
	}
	cc_[0] = ~p; // store the 1-UIP
	assert( decisionLevel() == level(p.var()));
	minimizeConflictClause(abstr);
	// clear seen-flag of all literals that are not from the current dl
	// and determine position of literal from second highest DL, which is
	// the asserting level of the newly derived conflict clause.
	secondWatch        = 1;
	uint32 assertLevel = 0;
	for (LitVec::size_type i = 1; i != cc_.size(); ++i) {
		clearSeen(cc_[i].var());
		if (level(cc_[i].var()) > assertLevel) {
			assertLevel = level(cc_[i].var());
			secondWatch = (uint32)i;
		}
	}
	return assertLevel;
}

void Solver::minimizeConflictClause(uint32 abstr) {
	uint32 m = strategy_.cflMinAntes;
	LitVec::size_type t = trail_.size();
	// skip the asserting literal
	LitVec::size_type j = 1;
	for (LitVec::size_type i = 1; i != cc_.size(); ++i) { 
		Literal p = ~cc_[i];
		if (reason(p).isNull() || ((reason(p).type()+1) & m) == 0  || !minimizeLitRedundant(p, abstr)) {
			cc_[j++] = cc_[i];
		}
		// else: p is redundant and can be removed from cc_
		// it was added to trail_ so that we can clear its seen flag
	}
	cc_.erase(cc_.begin()+j, cc_.end());
	while (trail_.size() != t) {
		clearSeen(trail_.back().var());
		trail_.pop_back();
	}
}

bool Solver::minimizeLitRedundant(Literal p, uint32 abstr) {
	if (!strategy_.strengthenRecursive) {
		conflict_.clear(); reason(p).reason(p, conflict_);
		for (LitVec::size_type i = 0; i != conflict_.size(); ++i) {
			if (!seen(conflict_[i].var())) {
				return false;
			}
		}
		trail_.push_back(p);
		return true;
	}
	// else: een_minimization
	trail_.push_back(p);  // assume p is redundant
	LitVec::size_type start = trail_.size();
	for (LitVec::size_type f = start;;) {
		conflict_.clear();
		reason(p).reason(p, conflict_);
		for (LitVec::size_type i = 0; i != conflict_.size(); ++i) {
			p = conflict_[i];
			if (!seen(p.var())) {
				if (!reason(p).isNull() && ((1<<(level(p.var())&31)) & abstr) != 0) {
					markSeen(p.var());
					trail_.push_back(p);
				}
				else {
					while (trail_.size() != start) {
						clearSeen(trail_.back().var());
						trail_.pop_back();
					}
					trail_.pop_back(); // undo initial assumption
					return false;
				}
			}
		}
		if (f == trail_.size()) break;
		p = trail_[f++];
	}
	return true;
}

// Selects next branching literal. Use user-supplied heuristic if rand() < randProp.
// Otherwise makes a random choice.
// Returns false if assignment is total.
bool Solver::decideNextBranch() {
	DecisionHeuristic* heu = strategy_.heuristic.get();
	if (randHeuristic_ && drand() < static_cast<SelectRandom*>(randHeuristic_)->randFreq()) {
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
	stats.solve.learnt[3] += oldS - numLearntConstraints();
}

/////////////////////////////////////////////////////////////////////////////////////////
// The basic DPLL-like search-function
/////////////////////////////////////////////////////////////////////////////////////////
ValueRep Solver::search(uint64 maxConflicts, uint32 maxLearnts, double randProp, bool localR) {
	initRandomHeuristic(randProp);
	maxConflicts = std::max(uint64(1), maxConflicts);
	if (localR) {
		if (!levConflicts_) levConflicts_ = new VarVec();
		levConflicts_->assign(decisionLevel()+1, (uint32)stats.solve.conflicts);
	}
	do {
		if ((hasConflict()&&!resolveConflict()) || !simplify()) { return value_false; }
		do {
			while (!propagate()) {
				if (!resolveConflict() || (decisionLevel() == 0 && !simplify())) {
					return value_false;
				}
				if ((!localR && --maxConflicts == 0) ||
					(localR && (stats.solve.conflicts - (*levConflicts_)[decisionLevel()]) > maxConflicts)) {
					undoUntil(0);
					return value_free;  
				}
			}
			if (numLearntConstraints()>maxLearnts) { 
				reduceLearnts(.75f); 
			}
		} while (decideNextBranch());
		// found a model candidate
		assert(numFreeVars() == 0);
	} while (!post_.isModel(*this));
	++stats.solve.models;
	stats.solve.updateModels(decisionLevel());
	if (strategy_.satPrePro.get()) {
		strategy_.satPrePro->extendModel(vars_);
	}
	return value_true;
}

bool Solver::nextSymModel(bool expand) {
	assert(numFreeVars() == 0);
	if (expand) {
		if (strategy_.satPrePro.get() != 0 && strategy_.satPrePro->hasSymModel()) {
			++stats.solve.models;
			stats.solve.updateModels(decisionLevel());
			strategy_.satPrePro->extendModel(vars_);
			return true;
		}
		else if (post_.nextSymModel(*this, true)) {
			++stats.solve.models;
			stats.solve.updateModels(decisionLevel());
			return true;
		}
	}
	else {
		post_.nextSymModel(*this, false);
		if (strategy_.satPrePro.get()) {
			strategy_.satPrePro->clearModel();
		}
	}
	return false;
}
}
