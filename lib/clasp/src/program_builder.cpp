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

#include <clasp/include/program_builder.h>
#include <clasp/include/preprocessor.h>
#include <clasp/include/solver.h>
#include <clasp/include/clause.h>
#include <clasp/include/smodels_constraints.h>
#include <clasp/include/unfounded_check.h>
#include <sstream>
#include <numeric>


namespace Clasp { 
/////////////////////////////////////////////////////////////////////////////////////////
// class PrgAtomNode 
//
// Objects of this class represent atoms in the atom-body-dependency graph.
/////////////////////////////////////////////////////////////////////////////////////////
// Creates the atom-oriented nogoods for this atom in form of clauses.
// Adds clauses for the tableau-rules FTA and BFA, i.e.
// - a binary clause [~b h] for every body b of h
// Adds a clause for the tableau-rules BTA and FFA, i.e.
// - [~h B1...Bi] where each Bi is a defining body of h
// Note: If b is a body of a choice rule, no binary clause [~b h] is created, since
// FTA and BFA do not apply to choice rules.
bool PrgAtomNode::toConstraint(Solver& s, ClauseCreator& gc, ProgramBuilder& prg) {
	if (value() != value_free && !s.force(trueLit(),0)) { 
		return false; 
	}
	if (!hasVar()) { return true; }
	ClauseCreator bc(&s);
	Literal a = literal();
	gc.start().add(~a);
	prg.vars_.mark( ~a );
	bool sat = false;
	// consider only bodies which are part of the simplified program, i.e.
	// are associated with a variable in the solver.
	VarVec::iterator j = preds.begin();
	for (VarVec::iterator it = preds.begin(); it != preds.end(); ++it) {
		PrgBodyNode* bn = prg.bodies_[*it];
		Literal B = bn->literal();
		sat |= prg.vars_.marked( ~B );
		if (bn->hasVar()) {
			*j++ = *it;
			if (!prg.vars_.marked(B)) {
				prg.vars_.mark( B );
				gc.add(B);
			}
			if (bn->type() != CHOICERULE && a != B) {
				bc.start().add(a);
				if ( (a != ~B && !bc.add(~B).end()) || (a == ~B && !bc.end()) ) {
					return false;
				}
			}
		}   
	}
	preds.erase(j, preds.end());
	prg.vars_.unmark( var() );
	for (VarVec::const_iterator it = preds.begin(); it != preds.end(); ++it) {
		prg.vars_.unmark( prg.bodies_[*it]->var() );
	}
	return sat || gc.end();
}

// Some bodies of this node have changed during preprocessing,
// e.g. they are false or were replaced with different but equivalent bodies.
// Update all back-links.
PrgAtomNode::SimpRes PrgAtomNode::simplifyBodies(Var atomId, ProgramBuilder& prg, const Preprocessor& pre, bool strong) {
	VarVec::iterator j = preds.begin();
	Var eq;
	uint32 diffLits = 0;
	for (VarVec::iterator it = preds.begin(), end = preds.end(); it != end; ++it) {
		if ((eq = pre.getEqBody(*it)) != *it) { *it = eq; }
		PrgBodyNode* x = prg.bodies_[*it];
		if (x->visited() == 0 && !x->ignore() && x->value() != value_false && (!strong || x->hasHead(atomId))) {
			x->setVisited(true);
			*j++ = *it;
			if (strong && (!prg.vars_.marked(x->literal()) || x->type() == CHOICERULE)) {
				++diffLits;
				if (x->type() != CHOICERULE) { prg.vars_.mark(x->literal()); }
			}
		}
	}
	preds.erase(j, preds.end());
	for (VarVec::iterator it = preds.begin(), end = preds.end(); it != end; ++it) {
		prg.bodies_[*it]->setVisited(false);
		if (strong) { prg.vars_.unmark(prg.bodies_[*it]->var()); }
	}
	if (!strong) diffLits = (uint32)preds.size();
	if (preds.empty()) {
		setIgnore(true);
		return SimpRes(setValue(value_false), 0);
	}
	return SimpRes(true, diffLits);
}

/////////////////////////////////////////////////////////////////////////////////////////
// class PrgBodyNode 
//
// Objects of this class represent bodies in the atom-body-dependency graph.
/////////////////////////////////////////////////////////////////////////////////////////
PrgBodyNode::PrgBodyNode(uint32 id, const PrgRule& rule, const PrgRule::RData& rInfo, ProgramBuilder& prg) {
	size_     = (uint32)rule.body.size();
	posSize_  = rInfo.posSize;
	goals_    = new Literal[size_];
	choice_   = rule.type() == CHOICERULE;
	extended_ = rule.type() == BASICRULE || rule.type() == CHOICERULE ? 0 : new Extended(this, rule.bound(), rule.type() == WEIGHTRULE);
	uint32 spw= 0;  // sum of positive weights
	uint32 snw= 0;  // sum of negative weights
	uint32 p = 0, n = 0;
	// store B+ followed by B-
	for (LitVec::size_type i = 0, end = rule.body.size(); i != end; ++i) {
		WeightLiteral g = rule.body[i];
		PrgAtomNode* a  = prg.resize(g.first.var());
		prg.ruleState_.popFromRule(g.first.var()); // clear flags that were set in PrgRule during rule simplification
		if (!g.first.sign()) {  // B+ atom
			goals_[p] =   g.first;
			spw       +=  g.second;
			a->posDep.push_back(id);
			if (extended_ && extended_->weights_) { extended_->weights_[p] = g.second; }
			++p;
		}
		else {                  // B- atom
			goals_[posSize_+n]  = g.first;
			snw += g.second;
			if (prg.eqIters_ != 0) { a->negDep.push_back(id); }
			if (extended_ && extended_->weights_) { extended_->weights_[posSize_+n] = g.second; }
			++n;
		}
	}
	if (extended_) {
		extended_->sumWeights_  = spw + snw;
		extended_->bound_       = rule.bound();
	}
	unsupp_ = static_cast<weight_t>(this->bound() - snw);
}

PrgBodyNode::~PrgBodyNode() {
	delete [] goals_;
	delete extended_;
}

PrgBodyNode::Extended::Extended(PrgBodyNode* self, uint32 b, bool w) 
	: bound_(b) 
	, weights_( w ? new weight_t[self->size()] : 0 ) {  
}

PrgBodyNode::Extended::~Extended() {
	delete [] weights_;
}

// Normalize head-list, e.g. replace [1, 2, 1, 3] with [1,2,3]
void PrgBodyNode::buildHeadSet() {
	if (heads.size() > 1) {
		std::sort(heads.begin(), heads.end());
		heads.erase(std::unique(heads.begin(), heads.end()), heads.end());
	}
}

// Type of the body
RuleType PrgBodyNode::type() const {
	return extended_ == 0 
		? (choice_ == 0 ? BASICRULE : CHOICERULE)
		: extended_->type();
}

// Lower bound of this body, i.e. number of literals that must be true
// in order to make the body true.
weight_t PrgBodyNode::bound() const {
	return extended_ == 0
		? (weight_t)size()
		: (weight_t)std::max(extended_->bound_, weight_t(0));
}

// Sum of weights of the literals in the body.
// Note: if type != WEIGHTRULE, the size of the body is returned
weight_t PrgBodyNode::sumWeights() const {
	return extended_ == 0
		? (weight_t)size()
		: (uint32)std::max(extended_->sumWeights_, weight_t(0));
}

// Returns the weight of the idx'th subgoal in B+/B-
// Note: if type != WEIGHTRULE, the returned weight is always 1
weight_t PrgBodyNode::weight(uint32 idx, bool pos) const {
	return extended_ == 0 || extended_->weights_ == 0
		? 1
		: extended_->weights_[ (!pos * posSize()) + idx ];
}

// Returns true if *this and other are equivalent w.r.t their subgoals
// Note: For weight rules false is always returned.
bool PrgBodyNode::equal(const PrgBodyNode& other) const {
	if (type() == other.type() && type() != WEIGHTRULE && posSize() == other.posSize() && negSize() == other.negSize() && bound() == other.bound()) {
		LitVec  temp(goals_, goals_+size_);
		std::sort(temp.begin(), temp.end());
		for (uint32 i = 0, end = other.size_; i != end; ++i) {
			if (!std::binary_search(temp.begin(), temp.end(), other.goals_[i])) {
				return false;
			}
		}
		return true;
	}
	return false;
}

// The atom v, which must be a positive subgoal of this body, is supported, 
// check if this body is now also supported.
bool PrgBodyNode::onPosPredSupported(Var v) {
	if (!extended_ || extended_->weights_ == 0) {
		return --unsupp_ <= 0;
	}
	unsupp_ -= extended_->weights_[ std::distance(goals_, std::find(goals_, goals_+posSize_, posLit(v))) ];
	return unsupp_ <= 0;
}

// Creates the body-oriented nogoods for this body
bool PrgBodyNode::toConstraint(Solver& s, ClauseCreator& c, const ProgramBuilder& prg) {
	if (value() != value_free && !s.force(trueLit(), 0))  { return false; }
	if (!hasVar() || ignore())                            { return true; } // body is not relevant
	const AtomList& atoms = prg.atoms_;
	if (extended_ == 0) { return addPredecessorClauses(s, c, atoms); }
	WeightLitVec lits;
	for (uint32 i = 0, end = size_; i != end; ++i) {
		assert(goals_[i].var() != 0);
		Literal eq = goals_[i].sign() ? ~atoms[goals_[i].var()]->literal() : atoms[goals_[i].var()]->literal();
		lits.push_back( WeightLiteral(eq, weight(i)) );
	}
	return BasicAggregate::newWeightConstraint(s, literal(), lits, bound());
}

// Adds clauses for the tableau-rules FFB and BTB as well as FTB, BFB.
// FFB and BTB:
// - a binary clause [~b s] for every positive subgoal of b
// - a binary clause [~b ~n] for every negative subgoal of b
// FTB and BFB:
// - a clause [b ~s1...~sn n1..nn] where si is a positive and ni a negative subgoal
bool PrgBodyNode::addPredecessorClauses(Solver& s, ClauseCreator& gc, const AtomList& prgAtoms) {
	const Literal negBody = ~literal();
	ClauseCreator bc(&s);
	gc.start().add(literal()); 
	bool sat = false;
	for (Literal* it = goals_, *end = goals_+size_; it != end; ++it) {
		assert(it->var() != 0);
		Literal aEq = it->sign() ? ~prgAtoms[it->var()]->literal() : prgAtoms[it->var()]->literal();
		if (negBody != ~aEq) {
			bc.start().add(negBody);
			if ( (negBody != aEq && !bc.add(aEq).end()) || (negBody == aEq && !bc.end()) ) {
				return false;
			}
		} // else: SAT-clause - ~b b
		sat |= aEq == literal();
		if (~aEq != literal()) {
			gc.add( ~aEq );
		}
	}
	return sat || gc.end();
}

// Remove/merge duplicate literals.
// If body contains p and ~p and both are needed, set body to false.
// Remove false/true literals
bool PrgBodyNode::simplifyBody(ProgramBuilder& prg, uint32 bodyId, std::pair<uint32, uint32>& hashes, Preprocessor& pre, bool strong) {
	hashes.first = hashes.second = 0;
	bool ok = sumWeights() >= bound();
	uint32 j = 0;
	Var eq;
	bool rem;
	for (uint32 i = 0, end = size_; i != end && ok; ++i) {
		rem = true;
		Var a = goals_[i].var();
		hashes.first += hashId(goals_[i].sign()?-a:a);
		if ((eq = prg.getEqAtom(a)) != a) {
			a         = eq;
			goals_[i] = Literal(a, goals_[i].sign());
		}
		Literal p; ValueRep v = prg.atoms_[a]->value();
		bool mark = false;
		if (prg.atoms_[a]->hasVar() || strong) {
			p = goals_[i].sign() ? ~prg.atoms_[a]->literal() : prg.atoms_[a]->literal();
			v = prg.atoms_[a]->hasVar() ? v : value_false; 
			mark = true;
		}
		if (v != value_free) {                // truth value is known
			if (v == falseValue(goals_[i])) { 
				// drop rule if normal/choice
				// or if we can no longer reach the lower bound of card/weight rule
				ok = extended_ != 0 && (extended_->sumWeights_ -= weight(i)) >= bound();
				if (ok) {
					pre.setSimplifyHeads(bodyId);
				}
			}
			else if (extended_) { 
				// subgoal is true: remove from rule and decrease necessary lower bound
				weight_t w = weight(i);
				extended_->sumWeights_  -= w;
				extended_->bound_       -= w; 
			}
		}
		else if (!mark || !prg.vars_.marked(p)) {
			if (mark) prg.vars_.mark(p);
			goals_[j] = goals_[i];
			rem       = false;
			if (extended_ && extended_->weights_) {
				extended_->weights_[j] = extended_->weights_[i];
			}
			++j;
		}
		else if (extended_) { // body contains p more than once
			// ignore lit if normal/choice, merge if card/weight
			uint32 x = findLit(p, prg.atoms_);
			assert(x != static_cast<uint32>(-1) && "WeightBody - Literal is missing!");
			if (extended_->weights_ == 0) {
				extended_->weights_ = new weight_t[size()];
				std::fill_n(extended_->weights_, (LitVec::size_type)size(), 1);
			}
			extended_->weights_[x] += extended_->weights_[i];
		}
		if (mark && prg.vars_.marked(~p)) {     // body contains p and ~p
			ok = extended_ != 0 && (extended_->sumWeights_ - std::min(weight(i), findWeight(~p, prg.atoms_))) >= extended_->bound_;
		}
		if (rem) {
			VarVec& deps = goals_[i].sign() ? prg.atoms_[a]->negDep : prg.atoms_[a]->posDep;
			VarVec::iterator it = std::find(deps.begin(), deps.end(), bodyId);
			if (it != deps.end()) {
				*it = deps.back();
				deps.pop_back();
			}
		}
	}
	size_ = j;
	uint32 posS = 0;
	for (uint32 i = 0, end = (uint32)size_; i != end; ++i) {
		prg.vars_.unmark( prg.atoms_[goals_[i].var()]->var() );
		hashes.second += hashId(goals_[i].sign()?-goals_[i].var():goals_[i].var()); 
		posS          += goals_[i].sign() == false;
	}
	posSize_ = posS;
	assert(sumWeights() >= bound() || !ok);
	if (!ok) {          // body is false...
		setIgnore(true);  // ...and therefore can be ignored
		for (VarVec::size_type i = 0; i != heads.size(); ++i) {
			pre.setSimplifyBodies(heads[i]);
		}
		heads.clear();
		return setValue(value_false);
	}
	else if (bound() == 0) { // body is satisfied
		size_ = posSize_ = hashes.second = 0;
		delete extended_; extended_ = 0;
		return setValue(value_true);
	} 
	else if (bound() == sumWeights() || size_ == 1) { // body is normal
		delete extended_; extended_ = 0;
	}
	return true;
}

// remove duplicate, equivalent and superfluous atoms from the head
bool PrgBodyNode::simplifyHeads(ProgramBuilder& prg, Preprocessor& pre, bool strong) {
	// 1. mark the body literals so that we can easily detect superfluous atoms
	// and selfblocking situations.
	RuleState& rs = prg.ruleState_;
	for (uint32 i = 0; i != size_; ++i) { rs.addToBody( goals_[i] ); }
	// 2. Now check for duplicate/superfluous heads and selfblocking situations
	bool ok = true;
	Weights w(*this);
	VarVec::iterator j = heads.begin();
	for (VarVec::iterator it = heads.begin(), end = heads.end();  it != end; ++it) {
		if ((!strong || prg.atoms_[*it]->hasVar()) && !rs.inHead(*it)) {
			// Note: equivalent atoms don't have vars.
			if (!rs.superfluousHead(type(), sumWeights(), bound(), *it, w)) {
				*j++ = *it;
				rs.addToHead(*it);
				if (ok && rs.selfblocker(type(), sumWeights(), bound(), *it, w)) {
					ok = false;
				}
			}
			else { pre.setSimplifyBodies(*it); }
		}
	}
	heads.erase(j, heads.end());
	for (uint32 i = 0; i != size_; ++i) { rs.popFromRule(goals_[i].var()); }
	if (!ok) {
		for (VarVec::size_type i = 0; i != heads.size(); ++i) {
			pre.setSimplifyBodies(heads[i]);
			rs.popFromRule(heads[i]);
		}
		heads.clear();
		return setValue(value_false);
	}
	// head-set changed, reestablish ordering
	std::sort(heads.begin(), heads.end());
	for (VarVec::iterator it = heads.begin(), end = heads.end();  it != end; ++it) {
		rs.popFromRule(*it);
	}
	return true;
}

uint32 PrgBodyNode::findLit(Literal p, const AtomList& prgAtoms) const {
	for (uint32 i = 0; i != size(); ++i) {
		Literal x = prgAtoms[goals_[i].var()]->literal();
		if (goals_[i].sign()) x = ~x;
		if (x == p) return i;
	}
	return static_cast<uint32>(-1);
}

weight_t PrgBodyNode::findWeight(Literal p, const AtomList& prgAtoms) const {
	if (extended_ == 0 || extended_->weights_ == 0) return 1;
	uint32 i = findLit(p, prgAtoms);
	assert(i != static_cast<uint32>(-1) && "WeightBody - Literal is missing!");
	return extended_->weights_[i];
}
/////////////////////////////////////////////////////////////////////////////////////////
// SCC/cycle checking
/////////////////////////////////////////////////////////////////////////////////////////
class ProgramBuilder::CycleChecker {
public:
	CycleChecker(const AtomList& prgAtoms, const BodyList& prgBodies, bool check) 
		: atoms_(prgAtoms)
		, bodies_(prgBodies)
		, count_(0)
		, sccs_(0)
		, check_(check) {
	} 
	void visit(PrgBodyNode* b) { if (check_) visitDfs(b, true); }
	void visit(PrgAtomNode* a) { if (check_) visitDfs(a, false); }
	bool hasCycles() const {
		return sccs_ > 0;
	}
	uint32 sccs() const {
		return sccs_;
	}
private:
	CycleChecker& operator=(const CycleChecker&);
	void visitDfs(PrgNode* n, bool body);
	typedef VarVec::iterator        VarVecIter;
	struct Call {
		Call(PrgNode* n, bool body, Var next, uint32 min = 0)
			: node_( n )
			, min_(min)
			, next_(next)
			, body_(body) {}
		PrgNode*  node()  const { return node_; }
		bool      body()  const { return body_ != 0; }
		uint32    min()   const { return min_; }
		uint32    next()  const { return next_; }
		void      setMin(uint32 m)  { min_ = m; }
	private:
		PrgNode*  node_;    // node that is visited
		uint32    min_;     // min "discovering time"
		uint32    next_:31; // next successor
		uint32    body_:1;  // is node_ a body?
	};
	typedef PodVector<Call>::type CallStack;
	NodeList        nodeStack_; // Nodes in the order they are visited
	CallStack       callStack_; // explict "runtime" stack - avoid recursion and stack overflows (mainly a problem on Windows)
	const AtomList& atoms_;     // atoms of the program
	const BodyList& bodies_;    // bodies of the program
	uint32          count_;     // dfs counter
	uint32          sccs_;      // current scc number
	bool            check_;     // scc-check enabled?
};
// Tarjan's scc algorithm
// Uses callStack instead of native runtime stack in order to avoid stack overflows on
// large sccs.
void ProgramBuilder::CycleChecker::visitDfs(PrgNode* node, bool body) {
	if (!node->hasVar() || node->ignore() || node->visited()) return;
	callStack_.push_back( Call(node, body, 0) );
START:
	while (!callStack_.empty()) {
		Call c = callStack_.back();
		callStack_.pop_back();
		PrgNode*    n = c.node();
		bool body = c.body();
		if (n->visited() == 0) {
			nodeStack_.push_back(n);
			c.setMin(count_++);
			n->setRoot(c.min());
			n->setVisited(1);
		}
		// visit successors
		if (body) {
			PrgBodyNode* b = static_cast<PrgBodyNode*>(n);  
			for (VarVec::const_iterator it = b->heads.begin() + c.next(), end = b->heads.end(); it != end; ++it) {
				PrgAtomNode* a = atoms_[*it];
				if (a->hasVar() && !a->ignore()) {
					if (a->visited() == 0) {
						callStack_.push_back(Call(b, true, static_cast<uint32>(it-b->heads.begin()), c.min()));
						callStack_.push_back(Call(a, false, 0));
            goto START;
					} 
					if (a->root() < c.min()) {
						c.setMin(a->root());
					}
				}
			}
		}
		else if (!body) {
			PrgAtomNode* a = static_cast<PrgAtomNode*>(n);  
			VarVec::size_type end = a->posDep.size();
			assert(c.next() <= end);
			for (VarVec::size_type it = c.next(); it != end; ++it) {
				PrgBodyNode* bn = bodies_[a->posDep[it]];
				if (bn->hasVar() && !bn->ignore()) {
					if (bn->visited() == 0) {
						callStack_.push_back(Call(a, false, (uint32)it, c.min()));
						callStack_.push_back(Call(bn, true, 0));
						goto START;
					}
					if (bn->root() < c.min()) {
						c.setMin(bn->root());
					}
				}
			}
		}
		if (c.min() < n->root()) {
			n->setRoot( c.min() );
		}
		else {
			uint32 maxVertex = (1U<<30)-1;
			PrgNode* succVertex = 0;
			NodeList inCC;
			do {
				succVertex = nodeStack_.back();
				nodeStack_.pop_back();
				inCC.push_back(succVertex);
				succVertex->setRoot(maxVertex);
			} while(succVertex != n);
			if (inCC.size() == 1) {
				succVertex->setScc(PrgNode::noScc);
			}
			else {
				for (NodeList::size_type i = 0; i != inCC.size(); ++i) {
					inCC[i]->setScc(sccs_);
				}
				++sccs_;
			} 
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// class VarList
/////////////////////////////////////////////////////////////////////////////////////////
void VarList::addTo(Solver& s, Var startVar) {
	s.reserveVars((uint32)vars_.size());
	for (Var i = startVar; i != (Var)vars_.size(); ++i) {
		Var x = s.addVar( type(i) );
		assert(x == i);
		if (hasFlag(x, eq_f)) {
			s.changeVarType(x, Var_t::atom_body_var);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// class ProgramBuilder
/////////////////////////////////////////////////////////////////////////////////////////
ProgramBuilder::ProgramBuilder() : minimize_(0), incData_(0), ufs_(0) { }
ProgramBuilder::~ProgramBuilder() { disposeProgram(true); }
ProgramBuilder::Incremental::Incremental() : startAtom_(0), startVar_(1) {}
void ProgramBuilder::disposeProgram(bool force) {
	std::for_each( bodies_.begin(), bodies_.end(), DeleteObject() );
	BodyList().swap(bodies_);
	bodyIndex_.clear();
	MinimizeRule* r = minimize_;
	while (r) {
		MinimizeRule* t = r;
		r = r->next_;
		delete t;
	}
	minimize_ = 0;
	VarVec().swap(initialSupp_);
	rule_.clear();
	if (force) {
		std::for_each( atoms_.begin(), atoms_.end(), DeleteObject() );
		AtomList().swap(atoms_);
		eqAtoms_.clear();
		ufs_ = 0;
		delete incData_;  
		VarVec().swap(computeFalse_);
		vars_.clear();
		ruleState_.clear();
	}
}

ProgramBuilder& ProgramBuilder::setAtomName(Var atomId, const char* name) {
	resize(atomId);
	if (stats.index.size() <= (AtomIndex::size_type)atomId) {
		stats.index.resize( (AtomIndex::size_type)atomId + 1 );
	}
	stats.index[atomId].name = name;  
	return *this;
}
ProgramBuilder& ProgramBuilder::setCompute(Var atomId, bool pos) {  
	resize(atomId);
	ValueRep v = pos ? value_true : value_false;
	if (v == value_false) {
		computeFalse_.push_back(atomId);
	}
	if (!atoms_[atomId]->setValue(v)) {
		setConflict();
	}
	return *this;
}

ProgramBuilder& ProgramBuilder::freeze(Var atomId) {
	resize(atomId);
	if (!incData_) incData_ = new Incremental();
	incData_->freeze_.push_back(atomId);
	return *this;
}

ProgramBuilder& ProgramBuilder::unfreeze(Var atomId) {
	resize(atomId);
	if (!incData_) incData_ = new Incremental();
	incData_->unfreeze_.push_back(atomId);
	return *this;
}

ProgramBuilder& ProgramBuilder::startProgram(DefaultUnfoundedCheck* ufs, uint32 numEqIters) {
	disposeProgram(true);
	// atom 0 is always false
	atoms_.push_back( new PrgAtomNode() );
	incData_  = 0;
	eqIters_  = numEqIters;
	stats     = PreproStats();
	ufs_      = ufs;
	frozen_   = false;
	return *this;
}

ProgramBuilder& ProgramBuilder::updateProgram() {
	if (!incData_)  { incData_ = new Incremental(); }
	else            { 
		incData_->startAtom_  = (uint32)atoms_.size(); 
		incData_->startVar_   = (uint32)vars_.size();
		incData_->unfreeze_.clear();
		for (VarVec::iterator it = incData_->freeze_.begin(), end = incData_->freeze_.end(); it != end; ++it) {
			atoms_[*it]->setIgnore(false);
		}
	}
	// delete bodies...
	disposeProgram(false);
	// clean up atoms
	for (VarVec::size_type i = 0; i != atoms_.size(); ++i) {
		VarVec().swap(atoms_[i]->posDep);
		VarVec().swap(atoms_[i]->negDep);
		VarVec().swap(atoms_[i]->preds);
	}
	frozen_   = false;
	return *this;
}


ProgramBuilder& ProgramBuilder::addRule(PrgRule& r) {
	// simplify rule, mark literals
	PrgRule::RData rd = r.simplify(ruleState_); 
	if (r.type() != ENDRULE) {  // rule is relevant, add it
		addRuleImpl(r, rd);
	}
	else {                      // rule is not relevant, ignore it
		clearRuleState(r);        // but don't forget to clear the rule state
	}
	return *this;
}

uint32 ProgramBuilder::addAsNormalRules(PrgRule& r, PrgRule::TransformationMode m) {
	PrgRule::RData rd = r.simplify(ruleState_); 
	if (r.type() == BASICRULE || r.type() == OPTIMIZERULE) {
		addRuleImpl(r, rd);
		return 1;
	}
	clearRuleState(r);
	if (r.type() != ENDRULE) {
		return r.transform(*this, m);
	}
	return 0;
}

void ProgramBuilder::addRuleImpl(const PrgRule& r, const PrgRule::RData& rd) {
	assert(!frozen_ && "Can't add rules to frozen program!");
	assert(r.type() != ENDRULE);
	if (r.type() != OPTIMIZERULE) {
		Body b = findOrCreateBody(r, rd);
		if (b.first->value() == value_false || rd.value_ == value_false) {
			// a false body can't define any atoms
			for (VarVec::iterator it = b.first->heads.begin(), end = b.first->heads.end(); it != end; ++it) {
				PrgAtomNode* a = atoms_[*it];
				VarVec::iterator p = std::find(a->preds.begin(), a->preds.end(), b.second);
				if (p != a->preds.end()) { *p = a->preds.back(); a->preds.pop_back(); }
			}
			b.first->heads.clear();
			b.first->setValue(value_false);
		}
		for (VarVec::const_iterator it = r.heads.begin(), end = r.heads.end(); it != end; ++it) {
			if (b.first->value() != value_false) {
				PrgAtomNode* a = resize(*it);
				if (r.body.empty()) a->setIgnore(true);
				// Note: b->heads may now contain duplicates. They are removed in PrgBodyNode::buildHeadSet.
				b.first->heads.push_back((*it));
				// Similarly, duplicates in atoms_[*it]->preds_ are removed in PrgAtomNode::toConstraint
				a->preds.push_back(b.second);
			}
			ruleState_.popFromRule(*it);  // clear flag of head atoms
		}
		if (rd.value_ != value_free) { b.first->setValue(rd.value_); }
	}
	else {
		assert(r.heads.empty());
		ProgramBuilder::MinimizeRule* mr = new ProgramBuilder::MinimizeRule;
		for (WeightLitVec::const_iterator it = r.body.begin(), bEnd = r.body.end(); it != bEnd; ++it) { 
			resize(it->first.var());
			ruleState_.popFromRule(it->first.var());
		} 
		mr->lits_ = r.body;
		mr->next_ = minimize_;
		minimize_ = mr;
	}
}

ProgramBuilder::Body ProgramBuilder::findOrCreateBody(const PrgRule& r, const PrgRule::RData& rd) {
	// Note: We don't search for an existing body node for weight rules because
	// for those checking for equivalence can't be done using the "mark-and-test"-approach alone.
	if (r.type() != WEIGHTRULE) {
		ProgramBuilder::BodyRange eqRange = bodyIndex_.equal_range(rd.hash);
		for (; eqRange.first != eqRange.second; ++eqRange.first) {
			PrgBodyNode& o = *bodies_[eqRange.first->second];
			if (o.type() == r.type() && r.body.size() == o.size() && rd.posSize == o.posSize() && r.bound() == o.bound()) {
				// bodies are structurally equivalent - check if they contain the same literals
				// Note: at this point all literals of this rule are marked, thus we can check for
				// equivalence by simply walking over the literals of o and checking if there exists
				// a literal l that is not marked. If no such l exists, the bodies are equivalent.
				uint32 i = 0, end = std::max(o.posSize(), o.negSize());
				for (; i != end; ++i) {
					if (i < o.posSize() && !ruleState_.inBody(posLit(o.pos(i)))) { break; }
					if (i < o.negSize() && !ruleState_.inBody(negLit(o.neg(i)))) { break; }
				}
				if (i == end) { 
					// found an equivalent body - clear flags, i.e. unmark all body literals of this rule
					for (WeightLitVec::const_iterator it = r.body.begin(), bEnd = r.body.end(); it != bEnd; ++it) { 
						ruleState_.popFromRule(it->first.var());
					}
					return Body(&o, eqRange.first->second); 
				}
			}
		}
	}
	// no corresponding body exists, create a new object
	// Note: the flags are cleared in the ctor of PrgBodyNode
	uint32 bodyId   = (uint32)bodies_.size();
	PrgBodyNode* b  = new PrgBodyNode(bodyId, r, rd, *this);
	bodyIndex_.insert(BodyIndex::value_type(rd.hash, bodyId));
	bodies_.push_back(b);
	if (b->isSupported()) {
		initialSupp_.push_back(bodyId);
	}
	return Body(b, bodyId);
}


void ProgramBuilder::clearRuleState(const PrgRule& r) {
	for (VarVec::const_iterator it = r.heads.begin(), end = r.heads.end();  it != end; ++it) {
		// clear flag only if a node was added for the head!
		if ((*it) < ruleState_.size()) {    
			ruleState_.popFromRule(*it);
		}
	}
	for (WeightLitVec::const_iterator it = r.body.begin(), bEnd = r.body.end(); it != bEnd; ++it) { 
		ruleState_.popFromRule(it->first.var());
	} 
}

bool ProgramBuilder::endProgram(Solver& solver, bool initialLookahead, bool finalizeSolver) {
	if (frozen_ == false) {
		stats.bodies += (uint32)bodies_.size();
		updateFrozenAtoms(solver);
		frozen_ = true;
		if (atoms_[0]->value() == value_true || !applyCompute()) { return false; }
		Preprocessor p;
		if (!p.preprocess(*this, eqIters_ != 0 ? Preprocessor::full_eq : Preprocessor::no_eq, eqIters_, solver.strategies().satPrePro.get() == 0)) {
			return false;
		}
		vars_.addTo(solver, incData_ ? incData_->startVar_ : 1);
	}
	else {
		if (ufs_.get()) {
			ufs_ = new DefaultUnfoundedCheck(ufs_->reasonStrategy());
		}
		cloneVars(solver);
	}
	solver.startAddConstraints();
	CycleChecker c(atoms_, bodies_, ufs_.get() != 0);
	uint32 startAtom = 0;
	if (incData_) {
		startAtom = incData_->startAtom_;
		for (VarVec::const_iterator it = incData_->unfreeze_.begin(), end = incData_->unfreeze_.end(); it != end; ++it) {
			atoms_.push_back(atoms_[*it]);
		}
	}
	bool ret = addConstraints(solver, c, startAtom);
	stats.sccs += c.sccs();
	if (ret && c.hasCycles()) {
		// Transfer ownership of ufs to solver...
		solver.strategies().postProp.reset(ufs_.release());
		// and init the unfounded set checker with the non-tight program.
		ret = ufs_->init(solver, atoms_, bodies_, startAtom);
		stats.ufsNodes = (uint32)ufs_->nodes();
	}
	if (incData_) atoms_.resize( atoms_.size() - incData_->unfreeze_.size() );
	return ret && (!finalizeSolver || solver.endAddConstraints(initialLookahead));
}

void ProgramBuilder::updateFrozenAtoms(const Solver& solver) {
	if (incData_ != 0) {
		// update truth values of atoms from previous iterations
		for (uint32 i = 0; i != incData_->startAtom_; ++i) {
			ValueRep v = solver.value( atoms_[i]->var() );
			if (v != value_free) {
				atoms_[i]->setValue(v == trueValue(atoms_[i]->literal()) ? value_true : value_false);
			}
		}
		for (VarVec::iterator it = incData_->unfreeze_.begin(), end = incData_->unfreeze_.end(); it != end; ++it) {
			PrgAtomNode* a = atoms_[*it]; 
			assert(a->hasVar() && "Error: Can't unfreeze atom that is not frozen");
			// Mark literal of a so that we can remove a from freeze-list
			Literal x = a->literal(); 
			x.watch();
			a->setLiteral( x );
		}
		VarVec::iterator j = incData_->freeze_.begin();
		for (VarVec::iterator it = j, end = incData_->freeze_.end(); it != end; ++it) {
			PrgAtomNode* a = atoms_[*it]; 
			if (a->literal().watched()) { // atom is no longer frozen
				Literal x = a->literal();
				x.clearWatch();
				a->setLiteral(x);
				a->resetSccFlags();
			}
			else {
				assert(a->preds.empty() && "Can't freeze defined atom!");
				// Make atom a choice.
				// This way, no special handling during preprocessing/nogood creation is necessary
				// Note: The choice is removed on next call to updateProgram!
				startRule(CHOICERULE).addHead(*it).endRule();
				*j++ = *it;
			}
		}
		incData_->freeze_.erase(j, incData_->freeze_.end());
	}
}

bool ProgramBuilder::applyCompute() {
	for (VarVec::size_type i = 0; i != computeFalse_.size(); ++i) {
		PrgAtomNode* falseAtom = atoms_[computeFalse_[i]];
		// falseAtom is false, thus all its non-choice-bodies must be false
		for (VarVec::iterator it = falseAtom->preds.begin(), end = falseAtom->preds.end(); it != end; ++it) {
			PrgBodyNode* b = bodies_[*it];
			if (b->visited() == 0) { 
				b->setVisited(true);
				b->buildHeadSet();
				if (b->type() != CHOICERULE) {
					if (!b->setValue(value_false)) return false;
					// since the body is false, it can no longer define its heads
					for (VarVec::size_type x = 0; x != b->heads.size(); ++x) {
						if (b->heads[x] != computeFalse_[i]) {
							PrgAtomNode* head = atoms_[b->heads[x]];
							head->preds.erase(std::find(head->preds.begin(), head->preds.end(), *it));
						}
					}
					b->heads.clear();
				}
				else {
					b->heads.erase(std::find(b->heads.begin(), b->heads.end(), computeFalse_[i]));
					if (b->heads.empty() && b->value() != value_false) b->setIgnore(true);
				}
			}
		}
		for (VarVec::iterator it = falseAtom->preds.begin(), end = falseAtom->preds.end(); it != end; ++it) {
			bodies_[*it]->setVisited(false);
		}
		falseAtom->preds.clear();
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// program creation - clark's completion
//
// Adds (completion) nogoods and initiates scc checking.
/////////////////////////////////////////////////////////////////////////////////////////
bool ProgramBuilder::addConstraints(Solver& s, CycleChecker& c, uint32 startAtom) {
	ClauseCreator gc(&s);
	for (BodyList::const_iterator it = bodies_.begin(); it != bodies_.end(); ++it) {
		if ( !(*it)->toConstraint(s, gc, *this) ) { return false; }
		c.visit(*it);
	}
	for (AtomList::const_iterator it = atoms_.begin()+startAtom; it != atoms_.end(); ++it) {
		if ( !(*it)->toConstraint(s, gc, *this) ) { return false; }
		c.visit(*it);
	}
	freezeMinimize(s);
	return true;
}

MinimizeConstraint* ProgramBuilder::createMinimize(Solver& solver) {
	if (!minimize_) { return 0; }
	MinimizeConstraint* m = new MinimizeConstraint();
	WeightLitVec lits;
	for (MinimizeRule* r = minimize_; r; r = r->next_) {
		for (WeightLitVec::iterator it = r->lits_.begin(); it != r->lits_.end(); ++it) {
			PrgAtomNode* h    = atoms_[it->first.var()];
			if (h->hasVar()) {
				lits.push_back(WeightLiteral(it->first.sign() ? ~h->literal() : h->literal(), it->second));
			}
		}
		m->minimize(solver, lits);
		lits.clear();
	}
	m->simplify(solver,false);
	return m;
}

// exclude vars contained in minimize statements from var elimination
void ProgramBuilder::freezeMinimize(Solver& solver) {
	if (!minimize_) return;
	for (MinimizeRule* r = minimize_; r; r = r->next_) {
		for (WeightLitVec::iterator it = r->lits_.begin(); it != r->lits_.end(); ++it) {
			PrgAtomNode* h    = atoms_[it->first.var()];
			if (h->hasVar()) {
				solver.setFrozen(h->var(), true);
			}
		}
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// misc/helper functions
/////////////////////////////////////////////////////////////////////////////////////////
void ProgramBuilder::setConflict() {
	atoms_[0]->setValue(value_true);
}

Var ProgramBuilder::newAtom() {
	assert(!frozen_ && "Can't add atom to frozen program!");
	atoms_.push_back( new PrgAtomNode() );
	return (Var) atoms_.size() - 1;
}

PrgAtomNode* ProgramBuilder::resize(Var atomId) {
	assert(atomId != varMax && atomId > 0);
	while (atoms_.size() <= (AtomIndex::size_type)atomId) {
		atoms_.push_back( new PrgAtomNode() );
	}
	return atoms_[atomId];
}

// for each var in vars_, adds a corresponding variable to the solver solver and
// clears all flags used during scc-checking.  
void ProgramBuilder::cloneVars(Solver& solver) {
	vars_.addTo(solver, incData_?incData_->startVar_:1);
	for (VarVec::size_type i = 0; i < bodies_.size(); ++i) {
		bodies_[i]->resetSccFlags();
	}
	for (VarVec::size_type i = incData_?incData_->startVar_:0; i < atoms_.size(); ++i) {
		atoms_[i]->resetSccFlags();
	}
	if (incData_) {
		for (VarVec::size_type i = 0; i != incData_->unfreeze_.size(); ++i) {
			atoms_[incData_->unfreeze_[i]]->resetSccFlags();
		}
	}
	
}

void ProgramBuilder::writeProgram(std::ostream& os) {
	const char* const delimiter = "0";
	// first write all minimize rules - revert order!
	PodVector<MinimizeRule*>::type mr;
	for (MinimizeRule* r = minimize_; r; r = r->next_) {
		mr.push_back(r);
	}
	for (PodVector<MinimizeRule*>::type::reverse_iterator rit = mr.rbegin(); rit != mr.rend(); ++rit) {
		os << OPTIMIZERULE << " " << 0 << " ";
		std::stringstream pBody, nBody, weights;
		VarVec::size_type nbs = 0, pbs =0;
		MinimizeRule* r = *rit;
		for (WeightLitVec::iterator it = r->lits_.begin(); it != r->lits_.end(); ++it) {
			if (atoms_[it->first.var()]->hasVar()) {
				it->first.sign() ? ++nbs : ++pbs;
				std::stringstream& body = it->first.sign() ? nBody : pBody;
				body << it->first.var() << " ";
				weights << it->second << " ";
			}
		}
		os << pbs+nbs << " " << nbs << " " 
			 << nBody.str() << pBody.str() << weights.str() << "\n";
	}
	// write all bodies together with their heads
	for (BodyList::iterator it = bodies_.begin(); it != bodies_.end(); ++it) {
		if ( (*it)->hasVar() ) {
			writeRule(*it, 0, os);
		}
	}
	// write the equivalent atoms
	for (EqNodes::const_iterator it = eqAtoms_.begin(); it != eqAtoms_.end(); ++it) {
		os << "1 " << it->first << " 1 0 " << it->second << " \n";
	}
	// write symbol-table and compute statement
	std::stringstream bp, bm, symTab;
	Literal comp;
	for (AtomList::size_type i = 1; i < atoms_.size(); ++i) {
		if ( atoms_[i]->value() != value_free ) {
			std::stringstream& str = atoms_[i]->value() == value_true ? bp : bm;
			str << i << "\n";
		}
		if (i < stats.index.size() && stats.index[i].lit != negLit(sentVar) && !stats.index[i].name.empty()) {
			symTab << i << " " << stats.index[i].name << "\n";
		}
	}
	os << delimiter << "\n";
	os << symTab.str();
	os << delimiter << "\n";
	os << "B+\n" << bp.str() << "0\n"
		 << "B-\n" << bm.str() << "0\n1\n";
}

void ProgramBuilder::writeRule(PrgBodyNode* b, Var h, std::ostream& os) {
	VarVec::size_type nbs = 0, pbs = 0;
	std::stringstream body;
	std::stringstream extended;
	RuleType rt = b->type();
	for (uint32 p = 0; p < b->negSize(); ++p) {
		if (atoms_[b->neg(p)]->hasVar() ) {
			body << b->neg(p) << " ";
			++nbs;
			if (rt == WEIGHTRULE) {
				extended << b->weight(p, false) << " ";
			}
		}
	}
	for (uint32 p = 0; p < b->posSize(); ++p) {
		if (atoms_[b->pos(p)]->hasVar() ) {
			body << b->pos(p) << " ";
			++pbs;
			if (rt == WEIGHTRULE) {
				extended << b->weight(p, true) << " ";
			}
		}
	}
	body << extended.str();
	if (rt != CHOICERULE) {
		for (VarVec::const_iterator it = b->heads.begin(); it != b->heads.end(); ++it) {
			Var x = h == 0 ? *it : h;
			if (h != 0 || atoms_[x]->hasVar()) {
				os << rt << " " << x << " ";
				if (rt == WEIGHTRULE) {
					os << b->bound() << " ";
				}
				os << pbs + nbs << " " << nbs << " ";
				if (rt == CONSTRAINTRULE) {
					os << b->bound() << " ";
				}
				os << body.str() << "\n";
			}
			if (h != 0) break;
		}
	}
	else {
		os << rt << " ";
		extended.str("");
		int heads = 0;
		for (VarVec::const_iterator it = b->heads.begin(); it != b->heads.end(); ++it) {
			if (atoms_[*it]->hasVar()) {
				++heads;
				extended << *it << " ";
			}
		}
		os << heads << " " << extended.str()
			 << pbs + nbs << " " << nbs << " "
			 << body.str() << "\n";
	}
}
}
