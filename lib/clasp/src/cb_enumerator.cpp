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

#include <clasp/include/cb_enumerator.h>
#include <clasp/include/solver.h>
#include <clasp/include/clause.h>
#include <clasp/include/smodels_constraints.h>

namespace Clasp {

CBConsequences::CBPrinter::CBPrinter()  {}
CBConsequences::CBPrinter::~CBPrinter() {}

CBConsequences::CBConsequences(Solver& s, AtomIndex* index, Consequences_t type, MinimizeConstraint* m, CBPrinter* printer, bool r) 
	: printer_(printer)
	, current_(0)
	, index_(index)
	, mini_(m)
	, type_(type)
	, restart_(r) {
	for (AtomIndex::size_type i = 0; i != index_->size(); ++i) {
		if (!(*index_)[i].name.empty()) { 
			s.setFrozen((*index_)[i].lit.var(), true);
			if (type_ == cautious_consequences) {
				(*index_)[i].lit.watch();  
			}
		}
	} 
}

CBConsequences::~CBConsequences() {
	if (current_) current_->destroy();
	for (ConstraintDB::size_type i = 0; i != locked_.size(); ++i) {
		locked_[i]->destroy();
	}
	locked_.clear();
	delete printer_;
}

bool CBConsequences::backtrackFromModel(Solver& s) {
	updateLastModel(s);
	uint32 miniBL = mini_ ? mini_->setModel(s) : s.decisionLevel();
	type_ == brave_consequences
			? updateBraveModel(s)
			: updateCautiousModel(s);
	printer_->printConsequences(s, *this);
	s.undoUntil(!C_.empty() ? s.level(C_[0].var()) : 0);
	if (current_ != 0) {
		// Remove current constraint; it is now obsolete.
		LearntConstraint* c = (LearntConstraint*)current_;
		c->removeWatches(s);
		if (!c->locked(s)) {
			c->destroy();
		}
		else {
			locked_.push_back(current_);
		}
		current_ = 0;
	}
	if (s.decisionLevel() == s.rootLevel()) return false;
	if (C_.size() > 1) {
		LitVec clLits; clLits.push_back(~C_[0]);
		uint32 sw = 1;
		for (uint32 i = 1; i != C_.size(); ++i) {
			clLits.push_back(~C_[i]);
			if (s.level(C_[i].var()) > s.level(clLits[sw].var())) { sw = i; }
		}
		// Create new clause, but do not add to DB of solver.
		current_ = Clause::newLearntClause(s, clLits, Constraint_t::learnt_conflict, sw);
	}
	if (restart_) {
		s.undoUntil(0);
		if (C_.size() == 1) {
			s.addUnary(~C_[0]);
		}
	}
	else {
		s.setConflict(C_);
		if (!s.resolveConflict()) {
			return false;
		}
		if (miniBL < s.decisionLevel()) {
			s.undoUntil(miniBL);
		}
	}
	return !mini_ || mini_->backpropagate(s);
}

void CBConsequences::updateLastModel(Solver& s) {
	C_.clear();
	// Delete old constraints that are no longer locked.
	ConstraintDB::size_type j = 0; 
	for (ConstraintDB::size_type i = 0; i != locked_.size(); ++i) {
		LearntConstraint* c = (LearntConstraint*)locked_[i];
		if (c->locked(s)) locked_[j++] = c;
		else c->destroy();
	}
	locked_.erase(locked_.begin()+j, locked_.end());
}

void CBConsequences::add(const Solver& s, Literal p) {
	assert(s.isTrue(p));
	if (s.level(p.var()) > 0) {
		C_.push_back(p);
		if (s.level(p.var()) > s.level(C_[0].var())) {
			std::swap(C_[0], C_.back());
		}
	}
}

void CBConsequences::updateBraveModel(Solver& s) {
	for (AtomIndex::size_type i = 0; i != index_->size(); ++i) {
		if (!(*index_)[i].name.empty()) {
			Literal& p = (*index_)[i].lit;
			if (s.isTrue(p))  { p.watch(); }
			if (!p.watched()) { add(s, ~p); }
		}
	}
}

void CBConsequences::updateCautiousModel(Solver& s) {
	for (AtomIndex::size_type i = 0; i != index_->size(); ++i) {
		Literal& p = (*index_)[i].lit;
		if (p.watched()) { 
			if      (s.isFalse(p))  { p.clearWatch(); }
			else if (s.isTrue(p))   { add(s, p); }
		}
	}
}

void CBConsequences::endSearch(Solver& s) {
	printer_->printReport(s, *this);
	CBConsequences::updateLastModel(s);
}
}
