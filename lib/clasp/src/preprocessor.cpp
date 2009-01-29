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
#include <clasp/include/preprocessor.h>
#include <clasp/include/program_builder.h>
namespace Clasp { namespace {

struct LessBodySize {
	LessBodySize(const BodyList& bl) : bodies_(&bl) {}
	bool operator()(Var b1, Var b2 ) const {
		return (*bodies_)[b1]->size() < (*bodies_)[b2]->size()
			|| ((*bodies_)[b1]->size() == (*bodies_)[b2]->size() && (*bodies_)[b1]->type() == BASICRULE && (*bodies_)[b2]->type() != BASICRULE);
	}
private:
	const BodyList* bodies_;
};
}

/////////////////////////////////////////////////////////////////////////////////////////
// simple preprocessing
//
// Simplifies the program by computing max consequences.
// If bodyEq == false, adds a variable for each supported atom and body
// Otherwise: Adds a variable for each supported atom and each body not equivalent to 
// some atom.
/////////////////////////////////////////////////////////////////////////////////////////
bool Preprocessor::preprocessSimple(bool bodyEq) {
	if (prg_->incData_) { updatePreviouslyDefinedAtoms(prg_->incData_->startAtom_, false); }
	Literal marked = negLit(0); marked.watch();
	if (bodyEq) {
		std::stable_sort(prg_->initialSupp_.begin(), prg_->initialSupp_.end(), LessBodySize(prg_->bodies_));
		for (VarVec::size_type i = 0; i != prg_->initialSupp_.size(); ++i) {
			prg_->bodies_[prg_->initialSupp_[i]]->setLiteral(marked);
		}
	}
	for (VarVec::size_type i = 0; i < prg_->initialSupp_.size(); ++i) {
		PrgBodyNode* b = prg_->bodies_[prg_->initialSupp_[i]];
		b->buildHeadSet();
		if (!bodyEq) b->setLiteral(posLit(prg_->vars_.add(Var_t::body_var)));
		for (VarVec::const_iterator h = b->heads.begin(); h != b->heads.end(); ++h) {
			if ( !prg_->atoms_[*h]->hasVar() ) {
				prg_->atoms_[*h]->preds.clear();
				bool m = bodyEq && b->size() == 0 && b->type() != CHOICERULE;
				if (m) {
					prg_->atoms_[*h]->setLiteral(posLit(0));
					prg_->atoms_[*h]->setValue(value_true);
					m = true;
				}
				else {
					prg_->atoms_[*h]->setLiteral(posLit(prg_->vars_.add(Var_t::atom_var)));
				}
				const VarVec& bl = prg_->atoms_[*h]->posDep;
				for (VarVec::const_iterator it = bl.begin(); it != bl.end(); ++it) {
					PrgBodyNode* nb = prg_->bodies_[*it];
					if (m) { nb->setLiteral(marked); }
					if (!nb->isSupported() && nb->onPosPredSupported(*h)) { prg_->initialSupp_.push_back( *it ); }
				}
			}
			prg_->atoms_[*h]->preds.push_back(prg_->initialSupp_[i]);
		}
	}
	if (bodyEq) {
		std::pair<uint32, uint32> ignore;
		for (VarVec::size_type i = 0; i < prg_->initialSupp_.size(); ++i) {
			uint32 bodyId   = prg_->initialSupp_[i];
			PrgBodyNode* b  = prg_->bodies_[bodyId];
			if (b->literal().watched() && !b->simplifyBody(*prg_, bodyId, ignore, *this, true)) {
				return false;
			}
			Literal l; l.watch();
			if      (b->ignore())     { l = negLit(0); }  
			else if (b->size() == 0)  { l = posLit(0); }
			else if (b->size() == 1)  { 
				l = b->posSize() == 1 ? prg_->atoms_[b->pos(0)]->literal() : ~prg_->atoms_[b->neg(0)]->literal(); 
				prg_->vars_.setAtomBody(l.var());
				prg_->stats.incBAEqs();
			}
			else {
				if (b->type() != CHOICERULE) {
					for (VarVec::size_type i = 0; i != b->heads.size(); ++i) {
						PrgAtomNode* a = prg_->atoms_[b->heads[i]];
						if (a->preds.size() == 1) {
							l = a->literal();
							prg_->stats.incBAEqs();
							break;
						}
					}
				}
				if (l.watched()) l = posLit(prg_->vars_.add(Var_t::body_var));
			}
			b->setLiteral(l);
		}
	}
	return true;
}

// If the program is defined incrementally, mark atoms from previous steps as supported
// Pre: Atoms that are true/false have their value-member set
void Preprocessor::updatePreviouslyDefinedAtoms(Var startAtom, bool strong) {
	for (Var i = 0; i != startAtom; ++i) {
		PrgAtomNode* a = prg_->atoms_[i];
		if (!strong && a->visited() == 0) {
			// a is an atom about to be unfrozen
			a->preds.clear();
		}
		PrgAtomNode* aEq = 0;
		if (strong) {
			nodes_[i].aSeen = 1;
			uint32 eq = prg_->getEqAtom(i);
			if (eq != i) {
				aEq = prg_->atoms_[eq];
				a->clearVar(true);
				a->setValue( aEq->value() );
			}
		}
		ValueRep v = a->hasVar() || aEq ? a->value() : value_false;
		for (VarVec::size_type b = 0; b != a->posDep.size(); ++b) {
			PrgBodyNode* body = prg_->bodies_[a->posDep[b]];
			if (strong && (aEq || v != value_free)) {
				nodes_[a->posDep[b]].sBody = 1;
			}
			if (v != value_false) {
				if (strong) ++nodes_[a->posDep[b]].known;
				bool isSupp = body->isSupported();
				body->onPosPredSupported(Var(i));
				if (!isSupp && body->isSupported()) {
					prg_->initialSupp_.push_back(a->posDep[b]);
				}
			}
		}
		if (strong) {
			for (VarVec::size_type b = 0; b != a->negDep.size(); ++b) {
				if (aEq || v != value_free) {
					nodes_[a->negDep[b]].sBody = 1;
				}
				if (v != value_true) {
					++nodes_[a->negDep[b]].known;
				}
			}
			if (v != value_free) {
				a->posDep.clear();
				a->negDep.clear();
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// equivalence preprocessing
//
/////////////////////////////////////////////////////////////////////////////////////////
bool Preprocessor::preprocessEq(uint32 maxIters) {
	nodes_.resize( prg_->bodies_.size() >= prg_->atoms_.size() ? prg_->bodies_.size() : prg_->atoms_.size() );
	Var startAtom   = prg_->incData_?prg_->incData_->startAtom_ : 0;
	Var stopAtom    = startAtom;
	Var startVar    = prg_->incData_?prg_->incData_->startVar_  : 1;
	conflict_ = false;
	pass_     = 0;
	maxPass_  = maxIters;
	do {
		++pass_;
		updatePreviouslyDefinedAtoms(startAtom, true);
		std::stable_sort(prg_->initialSupp_.begin(), prg_->initialSupp_.end(), LessBodySize(prg_->bodies_));
		prg_->vars_.shrink(startVar);
		litToNode_.clear();
		for (Var i = startAtom; i < stopAtom; ++i) {
			prg_->atoms_[i]->clearVar(false);
		} 
		if (!classifyProgram(startAtom, stopAtom)) return false;
	} while (stopAtom != (uint32)prg_->atoms_.size());
	// replace equivalent atoms in minimize rules
	for (ProgramBuilder::MinimizeRule* r = prg_->minimize_; r; r = r->next_) {
		for (WeightLitVec::iterator it = r->lits_.begin(); it != r->lits_.end(); ++it) {
			it->first = Literal(prg_->getEqAtom(it->first.var()), it->first.sign());
		}
	}
	return true;
}

bool Preprocessor::classifyProgram(uint32 startAtom, uint32& stopAtom) {
	conflict_ = false;
	follow_.clear();
	Var bodyId, bodyEqId; PrgBodyNode* body;
	Var atomId, atomEqId; PrgAtomNode* atom;
	// classify only supported bodies
	for (VarVec::size_type i = 0; i < prg_->initialSupp_.size(); ++i) {
		bodyId  = prg_->initialSupp_[i];
		body    = prg_->bodies_[bodyId];
		if (nodes_[bodyId].bSeen != 0 || body->ignore()) {
			if (body->ignore() && body->hasVar()) { body->clearVar(false); }
			continue;
		}
		follow_.push_back(bodyId);
		VarVec::size_type index = 0;
		while ( (bodyId = nextBodyId(index)) != varMax ) {
			body        = prg_->bodies_[bodyId];
			if (pass_==1&&body->heads.size()>1) { nodes_[bodyId].sHead = 1; }
			bodyEqId    = addBodyVar(bodyId, body);
			if (conflict_) return false;
			for (VarVec::const_iterator it = body->heads.begin(), end = body->heads.end(); it != end; ++it) {
				atomId    = *it;
				atom      = prg_->atoms_[atomId];
				atomEqId  = addAtomVar(atomId, atom, body);
				if (conflict_) return false;
				if (atomId != atomEqId && atom->hasVar()) {
					atom->clearVar(true); // equivalent atoms don't need vars
					// remove atom from head of body - equivalent atoms don't need definitions
					setSimplifyHeads(bodyEqId);
				}
				if (bodyId != bodyEqId) {
					// body is replaced with bodyEqId - move heads to bodyEqId
					prg_->bodies_[bodyEqId]->heads.push_back( atomEqId );
					// mark atom so that the link between atomEqId and bodyId can be replaced
					// with a link to bodyEqId.
					setSimplifyBodies(atomEqId);
				}
			}
			setHasBody(body->literal());
			if (bodyId != bodyEqId) {
				// from now on, ignore body.
				body->clearVar(true); body->setIgnore(true);
				// mark for head-simplification - will remove possible duplicates
				setSimplifyHeads(bodyEqId); 
				prg_->stats.incEqs(Var_t::body_var);
			}
		}
		follow_.clear();
	}
	return simplifyClassifiedProgram(startAtom, stopAtom);
}

bool Preprocessor::simplifyClassifiedProgram(uint32 startAtom, uint32& stopAtom) {
	stopAtom = (uint32)prg_->atoms_.size();
	prg_->initialSupp_.clear();
	std::pair<uint32, uint32> hash;
	for (uint32 i = 0; i != (uint32)prg_->bodies_.size(); ++i) {
		PrgBodyNode* b = prg_->bodies_[i];
		if (nodes_[i].bSeen == 0 || !b->hasVar()) {
			// bSeen == 0   : body is unsupported
			// !b->hasVar() : body is eq to other body or was derived to false
			// In either case, body is no longer relevant and can be ignored.
			b->clearVar(true); b->setIgnore(true);
			nodes_[i].bSeen = 0;
		}
		else { 
			assert(!b->ignore());
			bool hadHeads = !b->heads.empty();
			if (nodes_[i].sBody == 1 && !b->simplifyBody(*prg_, i, hash, *this, true)) {
				return false;
			}
			if (nodes_[i].sHead == 1 && !b->simplifyHeads(*prg_, *this, true)) {
				return false;
			}
			nodes_[i].bSeen = 0;
			if (hadHeads && b->value() == value_false && pass_ != maxPass_) {
				// New false body. If it was derived to false, we can ignore the body.
				// If it was forced to false (i.e. it is a selfblocker), reclassify 
				// as soon as it becomes supported.
				if      (b->ignore())         { newFalseBody(b, i, hash.first); }
				else if (b->resetSupported()) { prg_->initialSupp_.push_back(i); }
				// Reclassify, because an atom may have lost its source
				stopAtom = 0;
			}
			else if (b->heads.empty() && b->value() != value_false) {
				// Body is no longer needed. All heads are either superfluous or equivalent
				// to other atoms. 
				if (pass_ != maxPass_) {
					if (getRootAtom(b->literal()) == varMax) stopAtom = 0;  
					b->clearVar(true);
					b->setIgnore(true);
				}
			}
			else if (b->value() == value_true && b->var() != 0 && pass_ != maxPass_) {
				// New fact body. Merge with existing fact body if any.
				// Reclassify, if we derived at least one new fact.
				if (newFactBody(b, i, hash.first)) {
					stopAtom = 0;
				}
			}
			else if (b->resetSupported()) {
				prg_->initialSupp_.push_back(i);
			}
			nodes_[i].sBody = 0; nodes_[i].sHead = 0;
			nodes_[i].known = 0; 
		}
	}
	for (uint32 i = startAtom; i != (uint32)prg_->atoms_.size(); ++i) {
		PrgAtomNode* a = prg_->atoms_[i];
		if (a->hasVar()) {
			PrgAtomNode::SimpRes r(true, uint32(-1));
			if (nodes_[i].asBody == 1 && (r = a->simplifyBodies(Var(i), *prg_, *this, true)).first == false) {
				return false;
			}
			nodes_[i].asBody = 0; nodes_[i].aSeen = 0;
			if (stopAtom != prg_->atoms_.size() || (pass_ != maxPass_ && reclassify(a, i, r.second))) {
				stopAtom = std::min(stopAtom,i);
				a->clearVar(false);
			}
		}
	}
	return true;
}

// Derived a new fact body. The body is eq to True, therefore does not need a separate variable.
bool Preprocessor::newFactBody(PrgBodyNode* body, uint32 id, uint32 oldHash) {
	// first: delete old entry because hash has changed
	ProgramBuilder::BodyRange ra = prg_->bodyIndex_.equal_range(oldHash);
	for (; ra.first != ra.second && prg_->bodies_[ra.first->second] != body; ++ra.first);
	if (ra.first != ra.second) prg_->bodyIndex_.erase(ra.first);
	// second: check for an existing fact body
	// if none is found, add this body with its new hash to bodyIndex
	ra = prg_->bodyIndex_.equal_range(0);
	for (; ra.first != ra.second; ++ra.first ) {
		PrgBodyNode* other = prg_->bodies_[ra.first->second];
		if (body->equal(*other)) { 
			// Found an equivalent body, merge heads... 
			for (VarVec::size_type i = 0; i != body->heads.size(); ++i) {
				other->heads.push_back( body->heads[i] );
				setSimplifyBodies( body->heads[i] );
			}
			if (nodes_[ra.first->second].sHead == 0) {
				other->simplifyHeads(*prg_, *this, true);
			}
			// and remove this body from Program.
			body->heads.clear();
			body->setIgnore(true);
			body->clearVar(true);
			nodes_[id].bSeen  = 1;
			nodes_[id].eq     = ra.first->second;
			return true;
		}
	}
	// No equivalent body found. 
	prg_->bodyIndex_.insert(ProgramBuilder::BodyIndex::value_type(0, id));
	body->resetSupported();
	prg_->initialSupp_.push_back(id);
	// Only reclassify if this body derives new facts 
	return body->type() != CHOICERULE;
}

// body became false after it was used to derived its heads.
void Preprocessor::newFalseBody(PrgBodyNode* body, uint32 id, uint32 oldHash) {
	body->clearVar(true); body->setIgnore(true);
	nodes_[id].bSeen = 1;
	for (VarVec::size_type i = 0; i != body->heads.size(); ++i) {
		setSimplifyBodies(body->heads[i]);
	}
	body->heads.clear();
	// delete body from index - no longer needed
	ProgramBuilder::BodyRange ra = prg_->bodyIndex_.equal_range(oldHash);
	for (; ra.first != ra.second && prg_->bodies_[ra.first->second] != body; ++ra.first);
	if (ra.first != ra.second) prg_->bodyIndex_.erase(ra.first);
}

// check if atom has a distinct var although it is eq to some body
bool Preprocessor::reclassify(PrgAtomNode* a, uint32 atomId, uint32 diffLits) const {
	if ((a->preds.empty() && a->hasVar()) || 
		(a->preds.size() == 1
		&& prg_->bodies_[a->preds[0]]->type() != CHOICERULE
		&& prg_->bodies_[a->preds[0]]->literal() != a->literal())) {
		return true;
	}
	else if (a->preds.size() > 1 && diffLits == 1 && prg_->bodies_[a->preds[0]]->literal() != a->literal()) {
		for (VarVec::size_type i = 1; i != a->preds.size(); ++i) {
			PrgBodyNode* b = prg_->bodies_[a->preds[i]];
			VarVec::iterator it = std::lower_bound(b->heads.begin(), b->heads.end(), atomId);
			if (it != b->heads.end()) {
				b->heads.erase(it);
				if (b->heads.empty() && b->value() != value_false) {
					b->setIgnore(true);
					b->clearVar(true);
				}
			}
		}
		a->preds.erase(a->preds.begin()+1, a->preds.end());
		return true;
	}
	return false;
}

// associates a variable with the body if necessary
uint32 Preprocessor::addBodyVar(Var bodyId, PrgBodyNode* body) {
	// make sure we don't add an irrelevant body
	assert(body->value() == value_false || !body->heads.empty());
	assert(body->isSupported());
	body->clearVar(false);            // clear var in case we are iterating
	nodes_[bodyId].eq = bodyId;       // initially only eq to itself
	nodes_[bodyId].bSeen = 1;         // mark as seen, so we don't classify the body again
	bool changed  = nodes_[bodyId].sBody == 1;
	bool known    = nodes_[bodyId].known == body->size();
	std::pair<uint32, uint32> hashes;
	if (changed && !body->simplifyBody(*prg_, bodyId, hashes, *this, known)) {
		conflict_ = true;
		return bodyId;
	}
	if (nodes_[bodyId].sHead == 1) {  // remove any duplicates from head
		conflict_ = !body->simplifyHeads(*prg_, *this, false);
		nodes_[bodyId].sHead = 0;
		assert(!conflict_);
		if (body->heads.empty() && body->value() == value_free) {
			body->setIgnore(true);
			return bodyId;
		}
	}
	if      (body->ignore())            { return bodyId; }
	else if (!known && body->size()!=0) {
		body->setLiteral( posLit(prg_->vars_.add(Var_t::body_var)) );
		setSimplifyBody(bodyId);        // simplify strongly later
		return bodyId;
	}
	// all predecessors of this body are known
	if (changed) {  // and body has changed - check for equivalent body
		nodes_[bodyId].sBody = 0;
		// first: delete old entry because hash has changed
		ProgramBuilder::BodyRange ra = prg_->bodyIndex_.equal_range(hashes.first);
		for (; ra.first != ra.second && prg_->bodies_[ra.first->second] != body; ++ra.first);
		if (ra.first != ra.second) prg_->bodyIndex_.erase(ra.first);
		// second: check for an equivalent body. 
		// if none is found, add this body with its new hash to bodyIndex
		ra = prg_->bodyIndex_.equal_range(hashes.second);
		bool foundEq = false;
		for (; ra.first != ra.second; ++ra.first ) {
			PrgBodyNode* other = prg_->bodies_[ra.first->second];
			if (body->equal(*other)) { 
				foundEq = true;
				if (!other->ignore()) {
					// found an equivalent body - try to merge them
					conflict_ = !mergeBodies(body, bodyId, ra.first->second);
					return nodes_[bodyId].eq;
				}
			}
		}
		// The body is still unique, remember it under its new hash
		if (!foundEq) prg_->bodyIndex_.insert(ProgramBuilder::BodyIndex::value_type(hashes.second, bodyId));
	}
	if      (body->size() == 0) { 
		body->setLiteral( posLit(0) ); 
	}
	else if (body->size() == 1) { // body is equivalent to an atom or its negation
		// We know that body is equivalent to an atom. Now check if the atom is itself
		// equivalent to a body. If so, the body is equivalent to the atom's body.
		PrgAtomNode* a = 0;
		if (body->posSize() == 1) {
			a = prg_->atoms_[body->pos(0)];
			body->setLiteral( a->literal() );
		}
		else {
			body->setLiteral( ~prg_->atoms_[body->neg(0)]->literal() );
			Var dualAtom = getRootAtom(body->literal());
			a = dualAtom != varMax ? prg_->atoms_[dualAtom] : 0;
		}
		PrgBodyNode* root;
		if (a && a->preds.size() == 1 && (root = prg_->bodies_[a->preds[0]])->literal() == body->literal() && root->compatibleType(body)) {
			conflict_ = !mergeBodies(body, bodyId,a->preds[0]);
		}
		else {
			prg_->stats.incBAEqs();
			prg_->vars_.setAtomBody(body->var());
		}
	}
	else                        { // body is a conjunction - start new eq class
		body->setLiteral( posLit(prg_->vars_.add(Var_t::body_var)) );
	}
	return nodes_[bodyId].eq;
}

// associates a variable with the atom if necessary
// b is the supported body that brings a into the closure
uint32 Preprocessor::addAtomVar(Var atomId, PrgAtomNode* a, PrgBodyNode* b) {
	if ( nodes_[atomId].aSeen == 1 ) { 
		if (hasBody(b->literal()) && a->preds.size() > 1) {
			nodes_[atomId].asBody = 1;
		}
		return prg_->getEqAtom(atomId);   
	}
	nodes_[atomId].aSeen = 1;
	uint32 retId = atomId;
	// if set of defining bodies of this atom has changed, update it...
	if (nodes_[atomId].asBody == 1 && a->simplifyBodies(atomId, *prg_, *this, false).first == false) {
		conflict_ = true;
		return atomId;
	}
	if (pass_ == 1) { nodes_[atomId].asBody = 1; }
	if (b->type() != CHOICERULE && (b->value() == value_true || a->preds.size() == 1)) {
		// Atom is equivalent to b
		prg_->stats.incBAEqs();
		a->setLiteral( b->literal() );
		prg_->vars_.setAtomBody(a->var());
	}
	else {
		a->setLiteral( posLit(prg_->vars_.add(Var_t::atom_var)) ); 
	}
	if (b->value() == value_true) {
		// Since b is true, it is always a valid support for a, a can never become unfounded. 
		// So ignore it during SCC check and unfounded set computation.
		a->setIgnore(true);
		if (b->type() != CHOICERULE && !a->setValue(value_true)) {
			conflict_ = true;
			return atomId;
		}
	}
	PrgAtomNode* eqAtom = 0;
	Var eqAtomId = getRootAtom(a->literal());
	if ( eqAtomId == varMax ) {
		setRootAtom(a->literal(), atomId);
	}
	else {
		eqAtom = prg_->atoms_[eqAtomId];
		if (!mergeAtoms(atomId, eqAtomId)) {
			conflict_ = true;
			return atomId;
		}
		retId = eqAtomId;
	}
	// If atom a has a truth-value or is eq to a', we'll remove
	// it from all bodies. If there is an atom x, s.th. a.lit == ~x.lit, we mark all
	// bodies containing both a and x for simplification in order to detect
	// duplicates/contradictory body-literals.
	// In case that a == a', we also mark all bodies containing a
	// for head simplification in order to detect rules like: a' :- a,B. and a' :- B,not a.
	bool stableTruth  = a->value() == value_true || a->value() == value_false;
	bool removeAtom   = stableTruth || eqAtom != 0;
	bool removeNeg    = removeAtom || a->value() == value_weak_true;
	if (!removeAtom && getRootAtom(~a->literal()) != varMax) {
		PrgAtomNode* comp = prg_->atoms_[getRootAtom(~a->literal())];
		VarVec::size_type i, end;
		for (i = 0, end = comp->posDep.size(); i != end; ++i) { prg_->bodies_[comp->posDep[i]]->setVisited(1); }
		for (i = 0, end = comp->negDep.size(); i != end; ++i) { prg_->bodies_[comp->negDep[i]]->setVisited(1); }
	}
	for (VarVec::size_type i = 0; i != a->posDep.size();) {
		Var bodyId      = a->posDep[i];
		PrgBodyNode* bn = prg_->bodies_[ bodyId ];
		if (bn->ignore()) { a->posDep[i] = a->posDep.back(); a->posDep.pop_back(); continue;}
		bool supp = bn->isSupported() || (a->value() != value_false && bn->onPosPredSupported(atomId));
		if (++nodes_[bodyId].known == bn->size() && nodes_[bodyId].bSeen == 0 && supp) {
			follow_.push_back( bodyId );
		}
		else if (supp) {
			prg_->initialSupp_.push_back(bodyId);
		}
		if (removeAtom || bn->visited() == 1) { 
			setSimplifyBody(bodyId);
			if (eqAtom != 0) {
				setSimplifyHeads(bodyId);
				if (!stableTruth) { eqAtom->posDep.push_back( bodyId ); }
			}
		}
		++i;
	}
	for (VarVec::size_type i = 0; i != a->negDep.size();) {
		Var bodyId      = a->negDep[i];
		PrgBodyNode* bn = prg_->bodies_[ bodyId ];
		if (bn->ignore()) { a->negDep[i] = a->negDep.back(); a->negDep.pop_back(); continue;}
		if (++nodes_[bodyId].known == bn->size() && nodes_[bodyId].bSeen == 0) {
			follow_.push_back( bodyId );
		}
		if (removeNeg || bn->visited() == 1) { 
			setSimplifyBody(bodyId);
			if (eqAtom != 0) {
				setSimplifyHeads(bodyId);
				if (!stableTruth && a->value() != value_weak_true) { eqAtom->negDep.push_back( bodyId ); }
			}
		}
		++i;
	}
	if (removeNeg)  { a->negDep.clear(); }
	if (removeAtom) { a->posDep.clear(); }
	else  if  (getRootAtom(~a->literal()) != varMax) {
		PrgAtomNode* comp = prg_->atoms_[getRootAtom(~a->literal())];
		VarVec::size_type i, end;
		for (i = 0, end = comp->posDep.size(); i != end; ++i) { prg_->bodies_[comp->posDep[i]]->setVisited(0); }
		for (i = 0, end = comp->negDep.size(); i != end; ++i) { prg_->bodies_[comp->negDep[i]]->setVisited(0); }
	}
	return retId;
}

// body is equivalent to rootId and about to be replaced with rootId
// if one of them is a selfblocker, after the merge both are
bool Preprocessor::mergeBodies(PrgBodyNode* body, Var bodyId, Var rootId) {
	PrgBodyNode* root = prg_->bodies_[rootId];
	assert(!root->ignore() && root->compatibleType(body));
	assert(body->value() != value_false || body->heads.empty());
	assert(root->value() != value_false || root->heads.empty());
	nodes_[bodyId].eq = rootId;
	if (root->value() == value_false || body->value() == value_false) {
		PrgBodyNode* falseBody  = root->value() == value_false ? root : body;
		PrgBodyNode* other      = falseBody == root ? body : root;
		assert(falseBody->heads.empty());
		for (VarVec::size_type i = 0; i != other->heads.size(); ++i) {
			setSimplifyBodies( other->heads[i] );
		}
		other->heads.clear();
		if (!root->setValue(value_false)) return false;
	}
	if (nodes_[rootId].bSeen != 0) {
		body->setLiteral( root->literal() );  
	}
	else if (!body->heads.empty()) {
		// root is not yet classified. Move body's heads to root so that they are added 
		// when root is eventually classified. After the move, we can ignore body.
		setSimplifyHeads(rootId);
		for (VarVec::size_type i = 0; i != body->heads.size(); ++i) {
			root->heads.push_back( body->heads[i] );
			setSimplifyBodies( body->heads[i] );
		}
		body->heads.clear();
	}
	return true;
}

bool Preprocessor::mergeAtoms(Var atomId, Var atomEqId) {
	prg_->addEq(atomId, atomEqId);
	if (prg_->atoms_[atomId]->ignore()) {
		prg_->atoms_[atomEqId]->setIgnore(true);
	}
	if (prg_->atoms_[atomId]->value() != value_free) {
		return prg_->atoms_[atomEqId]->setValue(prg_->atoms_[atomId]->value());
	}
	return true;
}

}
