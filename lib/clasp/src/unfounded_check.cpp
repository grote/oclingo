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
#pragma warning(disable : 4996) // std::copy was declared deprecated
#endif

#include <clasp/include/unfounded_check.h>
#include <clasp/include/clause.h>
#include <algorithm>

namespace Clasp { 

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Body Nodes
/////////////////////////////////////////////////////////////////////////////////////////
// Body	object for handling normal/choice rules
// Choice rules are handled like normal rules with one exception:
//	Since BFA does not apply to choice rules, we manually trigger the removal of source pointers
//	whenever an atom source by a choice rule becomes false.
//
class UfsNormal : public DefaultUnfoundedCheck::UfsBodyNode {
public:
	typedef DefaultUnfoundedCheck::UfsBodyNode UfsBodyNode;
	typedef DefaultUnfoundedCheck::UfsAtomNode UfsAtomNode;
	UfsNormal(PrgBodyNode* b, bool choice) : UfsBodyNode(b->literal(), b->scc()), circWeight_(b->posSize()), choice_(choice) {}
	bool init(const PrgBodyNode&, const AtomList&, DefaultUnfoundedCheck& ufs, UfsAtomNode** initList, uint32 len) {
		if (initList == preds) { circWeight_ = len; }
		else if (choice_ == 1) {
			// If a head of a choice rule becomes false, force removal of its source pointer.
			for (uint32 i = 0; succs[i] != 0; ++i) {
				ufs.addWatch(~succs[i]->lit, this, i);
			}
		}
		return circWeight_ == 0;
	}
	void onWatch(DefaultUnfoundedCheck& ufs, Literal, uint32 data) {
		assert(choice_ == 1);
		if (succs[data]->hasSource() && succs[data]->watch() == this) {
			assert(ufs.solver().isFalse(succs[data]->lit));
			ufs.enqueuePropagateUnsourced(succs[data]);
		}
	}
	bool canSource(const UfsAtomNode&)		{ return circWeight_ == 0; }
	bool atomSourced(const UfsAtomNode&)	{ return --circWeight_ == 0; }
	bool atomUnsourced(const UfsAtomNode&){ return ++circWeight_ == 1; }
	void doAddIfReason(DefaultUnfoundedCheck& ufs) {
		if (ufs.solver().isFalse(lit)) {
			// body is only a reason if it does not depend on the atoms from the unfounded set
			for (UfsAtomNode** it = preds; *it; ++it) {
				if ( (*it)->typeOrUnfounded && !ufs.solver().isFalse((*it)->lit)) { return; }
			}
			ufs.addReasonLit(lit, this);
		}
	}
private:
	uint32	circWeight_ : 31;
	uint32	choice_			: 1;
};

// Body	object for handling cardinality and weight rules.
// Not the smartest implementation, but should work.
// The major problems with card/weight-rules are:
//	- subgoal false -> body false, does not hold
//	- subgoals can circularly depend on the body
// Consider: {x}. a:- 1{b,x}. b:-a. 
// Since x is external to 1{b,x}, the body is a valid source for a. Therefore, {a} can source b.
// Now, if we would simply count the number of subgoals with a source, 1{b,x} would have 2. Nevertheless,
// as soon as x becomes false, b and a are unfounded, because the loop {a,b} no longer has external bodies.
// 
// The class maintains two counters:
//	- trivWeight_ estimates the weight that is still achievable from subgoals not contained in the body's scc.
//	- circWeight_ sum of weights from subgoals contained in the body's scc having a source.
//	- trivWeight_ is reduced whenever an external subgoal becomes false.
//	- circWeight_ is reduced whenever an internal subgoal loses its source.
//	- As long as trivWeight_ >= bound, body is always a valid source.
//	- As soon as trivWeight_ < bound, we can no longer count on circWeight_, because it may be an overestimation. 
//		Hence, in that case, we temporarily assume the body to be an invalid source.
//	- trivWeight_ is recomputed whenever canSource() is called.
class UfsAggregate : public DefaultUnfoundedCheck::UfsBodyNode {
public:
	typedef DefaultUnfoundedCheck::UfsBodyNode UfsBodyNode;
	typedef DefaultUnfoundedCheck::UfsAtomNode UfsAtomNode;
	UfsAggregate(PrgBodyNode* b) : UfsBodyNode(b->literal(), b->scc()), ufs_(0), lits_(0), w_(0), sumW_(0), bound_(b->bound()), trivWeight_(0), circWeight_(0) {}
	~UfsAggregate() { 
		delete [] lits_;
		w_->~Weights();
		::operator delete(w_);
	}
	bool init(const PrgBodyNode& prgBody, const AtomList& prgAtoms, DefaultUnfoundedCheck& ufs, UfsAtomNode** initList, uint32 len) {
		if (initList == preds) {
			ufs_ = &ufs;
			// compute and allocate necessary storage
			uint32 otherScc = 0;
			for (uint32 p = 0; p != prgBody.posSize(); ++p) {
				PrgAtomNode* pred = prgAtoms[prgBody.pos(p)];
				if (pred->scc() != prgBody.scc() && !ufs.solver().isFalse(pred->literal())) {
					++otherScc;
				}
			}
			for (uint32 n = 0; n != prgBody.negSize(); ++n) {
				otherScc += !ufs.solver().isTrue(prgAtoms[prgBody.neg(n)]->literal());
			}
			lits_ = new Literal[otherScc+1];	// +1: terminating literal
			if (prgBody.type() == WEIGHTRULE) {
				void* m = ::operator new(sizeof(Weights) + ((len+otherScc)*sizeof(weight_t)));
				w_ = new (m) Weights;
				w_->numPos = (uint32)len;
			}
			// init 
			uint32 idx = 0, wsIdx = 0;
			for (uint32 p = 0; p != prgBody.posSize(); ++p) {
				PrgAtomNode* pred = prgAtoms[prgBody.pos(p)];
				if (!ufs.solver().isFalse(pred->literal())) {
					weight_t w = prgBody.weight(p, true);
					sumW_ += w;
					if (pred->scc() != prgBody.scc()) {
						trivWeight_ += w;
						lits_[idx]	= pred->literal();
						if (w_) { w_->weights_[w_->numPos+idx] = w; }
						++idx;
					}
					else if (w_) { w_->weights_[wsIdx++] = w;  }
					ufs.addWatch( ~pred->literal(), this, (static_cast<uint32>(w)<<1)+(pred->scc() != prgBody.scc()));
				}
			}
			for (uint32 n = 0; n != prgBody.negSize(); ++n) {
				PrgAtomNode* pred = prgAtoms[prgBody.neg(n)];
				if (pred->hasVar() && !ufs.solver().isTrue(pred->literal())) {
					weight_t w = prgBody.weight(n, false);
					sumW_ += w;
					trivWeight_ += w;
					ufs.addWatch( pred->literal(), this, (static_cast<uint32>(w)<<1)+1 );
					lits_[idx] = ~pred->literal();
					if (w_) { w_->weights_[w_->numPos+idx] = w; }
					++idx;
				}
			}
			lits_[idx] = posLit(0);
			bound_			= prgBody.bound();
			circWeight_ = 0;
		}
		return weight() >= bound_;
	}
	void onWatch(DefaultUnfoundedCheck& ufs, Literal, uint32 data) {
		if ((data & 1) == 1) { trivWeight_	-= (weight_t)data>>1; }
		if (watches > 0 && trivWeight_ < bound_) {
			ufs.enqueueInvalid(this);
		}
	}
	bool atomSourced(const UfsAtomNode& a)		{ 
		weight_t w = getSccWeight(a);
		circWeight_ += w;
		return (weight()-w) < bound_ && weight() >= bound_;
	}
	bool atomUnsourced(const UfsAtomNode& a)	{ 
		weight_t w = getSccWeight(a);
		circWeight_ -= w;
		return ((weight()+w) >= bound_ && weight() < bound_) || trivWeight_ < bound_;
	}
	bool canSource(const UfsAtomNode&) {
		trivWeight_ = computeTrivWeight();
		return weight() >= bound_;
	}
	void doAddIfReason(DefaultUnfoundedCheck& ufs) {
		if (ufs.solver().isFalse(lit)) {
			weight_t sumTemp = sumW_;
			for (uint32 i = 0; preds[i] != 0; ++i) {
				if ( preds[i]->typeOrUnfounded && !ufs.solver().isFalse(preds[i]->lit)) { 
					sumTemp -= getSccWeight(i);
					if (sumTemp < bound_) { return; }
				}
			}
			ufs.addReasonLit(lit, this);
		}
		else {	// body is not false but not a valid source either - add all false lits to reason set
			for (LitVec::size_type i = 0; !isSentinel(lits_[i]); ++i) {
				if (ufs.solver().isFalse(lits_[i])) {  ufs.addReasonLit(lits_[i], this); }
			}		
			for (UfsAtomNode** z = preds; *z; ++z) {
				if (ufs.solver().isFalse((*z)->lit)) { ufs.addReasonLit((*z)->lit,this); }
			}
		}
	}
private:
	weight_t weight()														const { return trivWeight_ + circWeight_; }
	weight_t getSccWeight(uint32 idx)						const { return w_ == 0 ? 1 : w_->weights_[idx]; }
	weight_t getSccWeight(const UfsAtomNode& a) const {
		if (w_ == 0) return 1;
		uint32 i; 
		for (i = 0; preds[i] != &a; ++i) { ; }	// Intentionally empty - a *must* be there.
		return w_->weights_[i];									// Otherwise something is horribly wrong!
	}
	weight_t computeTrivWeight() const {
		weight_t res = 0;
		for (LitVec::size_type i = 0; !isSentinel(lits_[i]); ++i) {
			if (!ufs_->solver().isFalse(lits_[i])) { 
				res += w_ ? w_->weights_[w_->numPos + i] : 1;
			}
		}
		return res;	
	}
	DefaultUnfoundedCheck* ufs_;// Our unfounded set checker
	Literal*	lits_;						// Array terminated with sentinel literal
	struct Weights {						// 
		uint32		numPos;					// Number of lits from same scc
		weight_t	weights_[0];		// All weights: same scc, other scc
	}*				w_;								// Only used for weight rules, otherwise 0
	weight_t	sumW_;						// maximal achievable weight
	weight_t	bound_;						// weight needed to satisfy body
	weight_t	trivWeight_;			// approximate weight still achievable from lits outside of body's scc; never overestimates
	weight_t	circWeight_;			// sum of weights from lits in same scc having a source
};
/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Construction/Destruction
/////////////////////////////////////////////////////////////////////////////////////////
DefaultUnfoundedCheck::DefaultUnfoundedCheck(ReasonStrategy r)
	: solver_(0) 
	, reasons_(0)
	, activeClause_(0)
	, strategy_(r) {
}
DefaultUnfoundedCheck::~DefaultUnfoundedCheck() { clear(); }

void DefaultUnfoundedCheck::clear() {
	std::for_each( bodies_.begin(), bodies_.end(), DeleteObject());
	std::for_each( atoms_.begin(), atoms_.end(), DeleteObject() );

	atoms_.clear();
	bodies_.clear();
	invalid_.clear();
	watches_.clear();
	delete activeClause_;
	delete [] reasons_;
	reasons_ = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Base interface
/////////////////////////////////////////////////////////////////////////////////////////
void DefaultUnfoundedCheck::reset() {
	assert(loopAtoms_.empty());
	while (!todo_.empty()) {
		todo_.back()->pickedOrTodo = 0;
		todo_.pop_back();
	}
	while (!unfounded_.empty()) {
		unfounded_.back()->typeOrUnfounded = 0;
		unfounded_.pop_back();
	}
	while (!sourceQueue_.empty()) {
		sourceQueue_.back()->resurrectSource();
		sourceQueue_.pop_back();
	}
	invalid_.clear();
	activeClause_->clear();
}

bool DefaultUnfoundedCheck::propagate() {
	return strategy_ != no_reason 
		? assertAtom()
		: assertSet();
}

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Constraint interface
/////////////////////////////////////////////////////////////////////////////////////////
// a (relevant) literal was assigned. Check which node is affected and update it accordingly
Constraint::PropResult DefaultUnfoundedCheck::propagate(const Literal& p, uint32& data, Solver& s) {
	assert(&s == solver_); (void)s;
	uint32 index	= data >> 1;
	uint32 type		= data & 1;
	if (type == 0) {
		// a normal body became false - mark as invalid if necessary
		assert(index < bodies_.size());
		if (bodies_[index]->watches > 0) { invalid_.push_back(bodies_[index]); }
	}
	else {
		// literal is associated with a watch - notify the object 
		assert(index < watches_.size());
		watches_[index].notify(p, this);
	}
	return PropResult(true, true);	// always keep the watch
}
// only used if strategy_ == only_reason
void DefaultUnfoundedCheck::reason(const Literal& p, LitVec& r) {
	assert(strategy_ == only_reason && reasons_);
	r.assign(reasons_[p.var()-1].begin(), reasons_[p.var()-1].end());
}

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck / Dependency graph initialization
/////////////////////////////////////////////////////////////////////////////////////////
// creates a positive-body-atom-dependency graph (PBADG) 
// from the body-atom-dependency-graph given in prgAtoms. 
// The PBADG contains a node for each non-trivially connected atom A and
// a node for each body B, s.th. there is a non-trivially connected atom A with
// B in body(A).
// Pre : a->ignore	= 0 for all new and relevant atoms a
// Pre : b->visited = 1 for all new and relevant bodies b
// Post: a->ignore	= 1 for all atoms that were added to the PBADG
// Post: b->visited = 0 for all bodies that were added to the PBADG
// Note: During initialization the root-member of a node in the BADG is used 
// to store the id of the corresponding node in the PBADG. 
bool DefaultUnfoundedCheck::init(Solver& s, const AtomList& prgAtoms, const BodyList& prgBodies, Var startAtom) {
	assert(!solver_ || solver_ == &s);
	solver_ = &s;
	if (!activeClause_) { activeClause_ = new ClauseCreator(solver_); }
	// add all relevant new atoms
	AtomList::const_iterator atomsEnd = prgAtoms.end();
	AtomVec::size_type oldAt = atoms_.size();
	for (AtomList::const_iterator h = prgAtoms.begin()+startAtom; h != atomsEnd; ++h) {
		if (relevantPrgAtom(*h)) {
			UfsAtomNode* hn = addAtom(*h);
			initAtom(*hn, **h, prgAtoms, prgBodies);
		}
	}
	propagateSource();
	// check for initially unfounded atoms
	for (AtomVec::iterator it = atoms_.begin()+oldAt; it != atoms_.end(); ++it) {
		if (!solver_->isFalse((*it)->lit)
			&& !(*it)->hasSource()
			&& !solver_->force(~(*it)->lit, 0)) {
			return false;
		}
	}
	return true;
}

// if necessary, creates a new atom node and adds it to the PBADG
DefaultUnfoundedCheck::UfsAtomNode* DefaultUnfoundedCheck::addAtom(PrgAtomNode* h) {
	if (!h->ignore()) {	// first time we see this atom -> create node
		atoms_.push_back( new UfsAtomNode(h->literal(), h->scc()) );
		h->setIgnore(true); // from now on, ignore this atom
		h->setRoot( (uint32)atoms_.size() - 1 );
		solver_->setFrozen(h->var(), true);
	}
	return atoms_[ h->root() ];	
}

void DefaultUnfoundedCheck::initAtom(UfsAtomNode& an, const PrgAtomNode& a, const AtomList& prgAtoms, const BodyList& prgBodies) {
	BodyVec bodies;
	for (VarVec::const_iterator it = a.preds.begin(), end = a.preds.end(); it != end; ++it) {
		PrgBodyNode* prgBody = prgBodies[*it];
		if (relevantPrgBody(prgBody)) {
			UfsBodyNode* bn = addBody(prgBody, prgAtoms);
			BodyVec::iterator insPos = bn->scc == an.scc ? bodies.end() : bodies.begin();
			bodies.insert(insPos, bn);
		}
	}
	an.preds = new UfsBodyNode*[bodies.size()+1];
	std::copy(bodies.begin(), bodies.end(), an.preds);
	an.preds[bodies.size()] = 0;
	bodies.clear();
	for (VarVec::const_iterator it = a.posDep.begin(), end = a.posDep.end(); it != end; ++it) {
		PrgBodyNode* prgBody = prgBodies[*it];
		if (prgBody->scc() == a.scc() && relevantPrgBody(prgBody)) {
			bodies.push_back(addBody(prgBody, prgAtoms));
		}
	}
	an.succs = new UfsBodyNode*[bodies.size()+1];
	std::copy(bodies.begin(), bodies.end(), an.succs);
	an.succs[bodies.size()] = 0;
	bodies.clear();
}

// if necessary, creates and initializes a new body node and adds it to the PBADG
DefaultUnfoundedCheck::UfsBodyNode* DefaultUnfoundedCheck::addBody(PrgBodyNode* b, const AtomList& prgAtoms) {
	if (b->visited() == 1) {	// first time we see this body - 
		b->setVisited(0);				// mark as visited and create node of
		RuleType rt = b->type();// suitable type
		UfsBodyNode* bn;
		if (b->scc() == PrgNode::noScc || rt == BASICRULE || rt == CHOICERULE) {
			bn = new UfsNormal(b, rt == CHOICERULE);
		}
		else if (rt == CONSTRAINTRULE || rt == WEIGHTRULE)	{ 
			bn = new UfsAggregate(b); 
		}
		else { assert(false && "Unknown body type!"); bn = 0; }
		bodies_.push_back( bn );
		b->setRoot( (uint32)bodies_.size() - 1 );
		solver_->addWatch(~b->literal(), this, ((uint32)bodies_.size() - 1)<<1);
		// Init node
		AtomVec atoms;
		for (uint32 p = 0; p != b->posSize(); ++p) {
			PrgAtomNode* pred = prgAtoms[b->pos(p)];
			if (relevantPrgAtom(pred) && pred->scc() == bn->scc) {
				atoms.push_back( addAtom(pred) );
			}
		}
		atoms.push_back(0);
		bn->preds				= new UfsAtomNode*[atoms.size()];
		std::copy(atoms.begin(), atoms.end(), bn->preds);
		bool ext = bn->init(*b, prgAtoms, *this, bn->preds, (uint32)atoms.size()-1);	// init preds
		atoms.clear();
		for (VarVec::const_iterator it = b->heads.begin(), end = b->heads.end(); it != end; ++it) {
			PrgAtomNode* a = prgAtoms[*it];
			if (relevantPrgAtom(a)) {
				UfsAtomNode* an = addAtom(a);
				atoms.push_back(an);
				if (!an->hasSource() && (a->scc() != b->scc() || ext) ) {
					an->updateSource(bn);
					sourceQueue_.push_back(an);
				}
			}
		}
		atoms.push_back(0);
		bn->succs				= new UfsAtomNode*[atoms.size()];
		std::copy(atoms.begin(), atoms.end(), bn->succs);
		bn->init(*b, prgAtoms, *this, bn->succs, (uint32)atoms.size()-1);	// init succs
		// exclude body from SAT-based preprocessing
		solver_->setFrozen(b->var(), true);
	}
	return bodies_[ b->root() ];
}

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - finding unfounded sets
/////////////////////////////////////////////////////////////////////////////////////////
bool DefaultUnfoundedCheck::findUnfoundedSet() {
	// first: remove all sources that were recently falsified
	if (!sourceQueue_.empty()) {
		propagateSource();
	}
	for (VarVec::size_type i = 0; i != invalid_.size(); ++i) { 
		removeSource(invalid_[i]); 
	}
	invalid_.clear();
	assert(sourceQueue_.empty() && unfounded_.empty());
	// second: try to re-establish sources.
	while (!todo_.empty()) {
		UfsAtomNode* head = dequeueTodo();
		assert(head->scc != PrgNode::noScc );
		if (!head->hasSource() && !solver_->isFalse(head->lit) && !findSource(head)) {
			return true;	// found an unfounded set - contained in unfounded_
		}
		assert(sourceQueue_.empty());
	}
	return false;			// no unfounded sets
}

// searches a new source for the atom node head.
// If a new source is found the function returns true.
// Otherwise the function returns false and unfounded_ contains head
// as well as atoms with no source that circularly depend on head.
bool DefaultUnfoundedCheck::findSource(UfsAtomNode* head) {
	assert(unfounded_.empty());
	enqueueUnfounded(head);				// unfounded, unless we find a new source
	Queue noSourceYet;
	bool changed = false;
	while (!unfounded_.empty()) {
		head = unfounded_.front();
		if (!head->hasSource()) {	// no source
			unfounded_.pop_front();	// note: current atom is still marked	
			UfsBodyNode** it	= head->preds;
			for (; *it != 0; ++it) {
				if (!solver_->isFalse((*it)->lit )) {
					if ((*it)->scc != head->scc || (*it)->canSource(*head)) {
						head->typeOrUnfounded = 0;	// found a new source,		
						setSource(*it, head);				// set the new source
						propagateSource();					// and propagate it forward
						changed = true;							// may source atoms in noSourceYet!
						break;
					}
					else {
						for (UfsAtomNode** z = (*it)->preds; *z; ++z) {
							if (!(*z)->hasSource() && !solver_->isFalse((*z)->lit)) {
								enqueueUnfounded(*z);
							}	
						}
					}
				}
			}
			if (!head->hasSource()) {
				noSourceYet.push_back(head);// no source found
			}
		}
		else {	// head has a source
			dequeueUnfounded();
		}
	}	// while unfounded_.emtpy() == false
	std::swap(noSourceYet, unfounded_);
	if (changed) {
		// remove all atoms that have a source as they are not unfounded
		Queue::iterator it, j = unfounded_.begin();
		for (it = unfounded_.begin(); it != unfounded_.end(); ++it) {
			if ( (*it)->hasSource() )		{ (*it)->typeOrUnfounded = 0; }
			else												{	*j++ = *it; }
		}
		unfounded_.erase(j, unfounded_.end());
	}
	return unfounded_.empty();
}

// sets body as source for head if necessary.
// PRE: value(body) != value_false
// POST: source(head) != 0
void DefaultUnfoundedCheck::setSource(UfsBodyNode* body, UfsAtomNode* head) {
	assert(!solver_->isFalse(body->lit));
	// For normal rules from not false B follows not false head, but
	// for choice rules this is not the case. Therefore, the 
	// check for isFalse(head) is needed so that we do not inadvertantly
	// source a head that is currently false.
	if (!head->hasSource() && !solver_->isFalse(head->lit)) {
		head->updateSource(body);
		sourceQueue_.push_back(head);
	}
}

// For each successor h of body sets source(h) to 0 if source(h) == body.
// If source(h) == 0, adds h to the todo-Queue.
// This function is called for each body that became invalid during propagation.
void DefaultUnfoundedCheck::removeSource(UfsBodyNode* body) {
	for (UfsAtomNode** it = body->succs; *it; ++it) {
		UfsAtomNode* head = *it;
		if (head->watch() == body) {
			if (head->hasSource()) {
				head->markSourceInvalid();
				sourceQueue_.push_back(head);
			}
			enqueueTodo(head);
		}
	}
	propagateSource();
}	


// propagates recently set source pointers within one strong component.
void DefaultUnfoundedCheck::propagateSource() {
	for (LitVec::size_type i = 0; i < sourceQueue_.size(); ++i) {
		UfsAtomNode* head = sourceQueue_[i];
		UfsBodyNode** it	= head->succs;
		if (head->hasSource()) {
			// propagate a newly added source-pointer
			for (; *it; ++it) {
				if ((*it)->atomSourced(*head) && !solver_->isFalse((*it)->lit)) {
					for (UfsAtomNode** aIt = (*it)->succs; *aIt; ++aIt) {
						setSource(*it, *aIt);
					}
				}
			}
		}
		else {
			// propagate the removal of a source-pointer within this scc
			for (; *it; ++it) {
				if ((*it)->atomUnsourced(*head)) {
					for (UfsAtomNode** aIt = (*it)->succs; *aIt && (*aIt)->scc == (*it)->scc; ++aIt) {
						if ((*aIt)->hasSource() && (*aIt)->watch() == *it) {
							(*aIt)->markSourceInvalid();
							sourceQueue_.push_back(*aIt);
						}
					}
				}
			}
		}
	}
	sourceQueue_.clear();
}


/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Propagating unfounded sets
/////////////////////////////////////////////////////////////////////////////////////////
// asserts all atoms of the unfounded set, then propagates
bool DefaultUnfoundedCheck::assertSet() {
	assert(strategy_ == no_reason);
	activeClause_->clear();
	while (findUnfoundedSet()) {
		while (!unfounded_.empty() && solver_->force(~unfounded_[0]->lit, 0)) {
			dequeueUnfounded();
		}
		if (!unfounded_.empty() || !unitPropagate(*solver_)) {
			return false;
		}
	}
	return true;
}

// as long as an unfounded set U is not empty,
// - asserts the first non-false atom
// - propagates
// - removes the atom from U
bool DefaultUnfoundedCheck::assertAtom() {
	while(findUnfoundedSet()) {
		activeClause_->clear();
		while (!unfounded_.empty()) {
			if (!solver_->isFalse(unfounded_[0]->lit) && !assertAtom(unfounded_[0]->lit)) {
				return false;
			}
			dequeueUnfounded();
		}
		if (!loopAtoms_.empty()) {
			createLoopFormula();
		}
	}
	return true;
}

// asserts an unfounded atom using the selected reason strategy
bool DefaultUnfoundedCheck::assertAtom(Literal a) {
	bool conflict = solver_->isTrue(a);
	if (strategy_ == distinct_reason || activeClause_->empty()) {
		// first atom of unfounded set or distinct reason for each atom requested.
		activeClause_->startAsserting(Constraint_t::learnt_loop, conflict ? a : ~a);
		invSign_ = conflict || strategy_ == only_reason;
		computeReason();
		if (conflict) {
			LitVec cfl;
			activeClause_->swap(cfl);
			assert(!cfl.empty());
			solver_->setConflict( cfl );
			return false;
		}
		else if (strategy_ == only_reason) {
			if (reasons_ == 0) reasons_ = new LitVec[solver_->numVars()];
			reasons_[a.var()-1].assign(activeClause_->lits().begin()+1, activeClause_->lits().end());
			solver_->force( ~a, this );
		}
		else if (strategy_ != shared_reason || activeClause_->size() < 4) {
			activeClause_->end();
		}
		else {
			loopAtoms_.push_back(~a);
			solver_->force(~a, 0);
		}
	}
	// subsequent atoms
	else if (conflict) {
		if (!loopAtoms_.empty()) {
			createLoopFormula();
		}
		if (strategy_ == only_reason) {
			// we already have the reason for the conflict, only add a
			LitVec cfl;
			(*activeClause_)[0] = a;
			activeClause_->swap(cfl);
			solver_->setConflict( cfl );
			return false;
		}
		else {
			// we have a clause - create reason for conflict by inverting literals
			LitVec cfl;
			cfl.push_back(a);
			for (LitVec::size_type i = 1; i < activeClause_->size(); ++i) {
				if (~(*activeClause_)[i] != a) {
					cfl.push_back( ~(*activeClause_)[i] );
				}
			}
			solver_->setConflict( cfl );
			return false;
		}
	}
	else if (strategy_ == only_reason) {
		reasons_[a.var()-1].assign(activeClause_->lits().begin()+1, activeClause_->lits().end());
		solver_->force( ~a, this );
	}
	else if (strategy_ != shared_reason || activeClause_->size() < 4) {
		(*activeClause_)[0] = ~a;
		activeClause_->end();
	}
	else {
		solver_->force(~a, 0);
		loopAtoms_.push_back(~a);
	}
	if ( (conflict = !unitPropagate(*solver_)) == true && !loopAtoms_.empty()) {
		createLoopFormula();
	}
	return !conflict;
}

void DefaultUnfoundedCheck::createLoopFormula() {
	assert(activeClause_->size() > 3);
	if (loopAtoms_.size() == 1) {
		(*activeClause_)[0] = loopAtoms_[0];
		Constraint* ante;
		activeClause_->end(&ante);
		assert(ante != 0 && solver_->isTrue(loopAtoms_[0]) && solver_->reason(loopAtoms_[0].var()).isNull());
		const_cast<Antecedent&>(solver_->reason(loopAtoms_[0].var())) = ante;
	}
	else {
		LoopFormula* lf = LoopFormula::newLoopFormula(*solver_, &(*activeClause_)[1], (uint32)activeClause_->size() - 1, (uint32)activeClause_->secondWatch()-1, (uint32)loopAtoms_.size()); 
		solver_->addLearnt(lf);
		for (VarVec::size_type i = 0; i < loopAtoms_.size(); ++i) {
			assert(solver_->isTrue(loopAtoms_[i]) && solver_->reason(loopAtoms_[i].var()).isNull());
			const_cast<Antecedent&>(solver_->reason(loopAtoms_[i].var())) = lf;
			lf->addAtom(loopAtoms_[i], *solver_);
		}
		lf->updateHeuristic(*solver_);
	}
	loopAtoms_.clear();
}


// computes the reason why a set of atoms is unfounded
void DefaultUnfoundedCheck::computeReason() {
	uint32 ufsScc = unfounded_[0]->scc;
	for (LitVec::size_type i = 0; i < unfounded_.size(); ++i) {
		assert(ufsScc == unfounded_[i]->scc);
		if (!solver_->isFalse(unfounded_[i]->lit)) {
			UfsAtomNode* a = unfounded_[i];
			for (UfsBodyNode** it = a->preds; *it != 0; ++it) {
				if ((*it)->pickedOrTodo == 0) {
					picked_.push_back(*it);
					(*it)->pickedOrTodo = 1;
					(*it)->addIfReason(*this, ufsScc);
				}
			}
		}
	}	
	assert( solver_->level( activeClause_->size() > 1 
		? (*activeClause_)[activeClause_->secondWatch()].var()
		: (*activeClause_)[0].var()
		) == solver_->decisionLevel() && "Loop nogood must contain a literal from current DL!");
	for (BodyVec::iterator it = picked_.begin(); it != picked_.end(); ++it) {
		(*it)->pickedOrTodo = false;
		solver_->data((*it)->lit.var()) &= ~3u;
	}
	for (LitVec::iterator it = pickedAtoms_.begin(); it != pickedAtoms_.end(); ++it) {
		solver_->data(it->var()) &= ~3u;
	}
	picked_.clear();
	pickedAtoms_.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Helpers
/////////////////////////////////////////////////////////////////////////////////////////
void DefaultUnfoundedCheck::addWatch(Literal p, UfsBodyNode* b, uint32 data) {
	uint32 d = (static_cast<uint32>(watches_.size())<<1) + 1;
	watches_.push_back(Watch(b, data));
	solver_->addWatch(p, this, d);
}

void DefaultUnfoundedCheck::addReasonLit(Literal p, const UfsBodyNode* reason) {
	if (solver_->data(p.var()) == 0 && solver_->level(p.var()) > 0) {
		solver_->data(p.var()) |= 1;
		if (p != reason->lit) { pickedAtoms_.push_back(p); }
		if (!invSign_ || (p = ~p) != activeClause_->lits().front()) {
			activeClause_->add(p);
		}
	}
}
}
