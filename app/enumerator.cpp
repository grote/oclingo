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
#include "enumerator.h"
#include <clasp/include/solver.h>
#include <iostream>
namespace Clasp {

/////////////////////////////////////////////////////////////////////////////////////////
// class StdOutPrinter
/////////////////////////////////////////////////////////////////////////////////////////
StdOutPrinter::StdOutPrinter() : index(0) {}
void StdOutPrinter::printModel(const Solver& s) {
	if (index) {
		std::cout << "Answer: " << s.stats.models << "\n";
		for (AtomIndex::size_type i = 0; i != index->size(); ++i) {
			if (s.value((*index)[i].lit.var()) == trueValue((*index)[i].lit) && !(*index)[i].name.empty()) {
				std::cout << (*index)[i].name << " ";
			}
		}
		std::cout << std::endl;
	}
	else {
		const uint32 numVarPerLine = 10;
		std::cout << "c Model: " << s.stats.models << "\n";
		std::cout << "v ";
		for (Var v = 1, cnt=0; v <= s.numVars(); ++v) {
			if (s.value(v) == value_false) std::cout << "-";
			std::cout << v;
			if (++cnt == numVarPerLine && v+1 <= s.numVars()) { cnt = 0; std::cout << "\nv"; }
			std::cout << " ";
		}
		std::cout << "0 \n" << std::flush;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// class MinimizeEnumerator
/////////////////////////////////////////////////////////////////////////////////////////
MinimizeEnumerator::~MinimizeEnumerator() {
	mini_->destroy();
}
void MinimizeEnumerator::updateModel(Solver& s)  {
	Enumerator::updateModel(s);
	mini_->printOptimum(std::cout);
}

/////////////////////////////////////////////////////////////////////////////////////////
// class CBConsequences
/////////////////////////////////////////////////////////////////////////////////////////
CBConsequences::CBConsequences(Solver& s, AtomIndex* index, Consequences_t type, bool quiet) : current_(0), index_(index), type_(type), quiet_(quiet) {
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
}

void CBConsequences::updateModel(Solver& s) {
	C_.clear();
	// Delete old constraints that are no longer locked.
	ConstraintDB::size_type j = 0; 
	for (ConstraintDB::size_type i = 0; i != locked_.size(); ++i) {
		if (locked_[i]->locked(s)) locked_[j++] = locked_[i];
		else locked_[i]->destroy();
	}
	locked_.erase(locked_.begin()+j, locked_.end());
	type_ == brave_consequences
			? updateBraveModel(s)
			: updateCautiousModel(s);
	if (!quiet_) printMarked();
}

bool CBConsequences::backtrackFromModel(Solver& s) { 
	s.undoUntil(!C_.empty() ? s.level(C_[0].var()) : 0);
	if (current_ != 0) {
		// Remove current constraint; it is now obsolete.
		current_->removeWatches(s);
		if (!current_->locked(s)) {
			current_->destroy();
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
		current_ = (Clause*)Clause::newLearntClause(s, clLits, Constraint_t::learnt_conflict, sw);
	}
	s.setConflict(C_);
	return s.resolveConflict();
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
			if (s.isTrue(p))	{ p.watch(); }
			if (!p.watched()) { add(s, ~p); }
		}
	}
}

void CBConsequences::updateCautiousModel(Solver& s) {
	for (AtomIndex::size_type i = 0; i != index_->size(); ++i) {
		Literal& p = (*index_)[i].lit;
		if (p.watched()) { 
			if			(s.isFalse(p))	{ p.clearWatch(); }
			else if (s.isTrue(p))		{ add(s, p); }
		}
	}
}

void CBConsequences::report(const Solver& s) const {
	if (quiet_ || s.stats.models == 0) {
		printMarked();
	}
}

void CBConsequences::printMarked() const {
	std::cout << (type_ == cautious_consequences ? "Cautious" : "Brave") << " consequences: \n";
	for (AtomIndex::size_type i = 0; i != index_->size(); ++i) {
		if ((*index_)[i].lit.watched()) {
			std::cout << (*index_)[i].name << " ";
		}
	}
	std::cout << std::endl;
}

}
