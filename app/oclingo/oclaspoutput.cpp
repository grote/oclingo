// Copyright (c) 2010, Torsten Grote <tgrote@uni-potsdam.de>
// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#include "oclaspoutput.h"
#include "oclingo/externalknowledge.h"

GRINGO_EXPORT_PRINTER(ExtVolPrinter)

oClaspOutput::oClaspOutput(Grounder* grounder, Clasp::Solver* solver, bool shiftDisj, uint32_t port, bool import)
	: iClaspOutput(shiftDisj)
	, ext_input_(false)
	, vol_atom_(0)
	, vol_atom_frozen_(0)
{
	ext_ = new ExternalKnowledge(grounder, this, solver, port, import);
}

oClaspOutput::~oClaspOutput()
{
	delete ext_; // TODO change pointer
}

ExternalKnowledge& oClaspOutput::getExternalKnowledge() {
	assert(ext_);
	return *ext_;
}

void oClaspOutput::startExtInput() {
		ext_input_ = true;
}

void oClaspOutput::stopExtInput() {
		ext_input_ = false;
}

void oClaspOutput::printBasicRule(uint32_t head, const AtomVec &pos, const AtomVec &neg) {
	if(ext_input_) {
		Symbol const &sym = symbol(head);
		if (sym.external) { head = sym.external; }
		if(ext_->checkHead(sym)) {
			ext_->addHead(head);
		} else {
			 // don't add rule since it was not defined external or already added
			return;
		}
	}
	ClaspOutput::printBasicRule(head, pos, neg);
}

uint32_t oClaspOutput::unnamedSymbol() {
	uint32_t atom = symbol();
	b_->setAtomName(atom, 0);
	atomUnnamed_[atom - lastUnnamed_] = false;

	return atom;
}

void oClaspOutput::freezeAtom(uint32_t symbol) {
	b_->freeze(symbol);
}

void oClaspOutput::unfreezeAtom(uint32_t symbol) {
	b_->unfreeze(symbol);
}

uint32_t oClaspOutput::getVolAtom() {
	if(vol_atom_ == 0) {
		vol_atom_ = unnamedSymbol();
	}

	return vol_atom_;
}

uint32_t oClaspOutput::getVolWindowAtom(int window) {
	// atoms are expired at "step"
	int step = window + ext_->getControllerStep();

	// get new atom for step
	if(vol_atom_map_.find(step) == vol_atom_map_.end()) {
		vol_atom_map_[step] = unnamedSymbol();
	}

	// freeze atom if not already frozen
	if(vol_window_atoms_frozen_.find(vol_atom_map_[step]) == vol_window_atoms_frozen_.end()) {
		vol_window_atoms_frozen_.insert(vol_atom_map_[step]);
		// needs to be frozen before assumption for this atom is retrieved
		b_->freeze(vol_atom_map_[step]);
	}

	return vol_atom_map_[step];
}

uint32_t oClaspOutput::getVolAtomAss() {
	return vol_atom_;
}

VarVec oClaspOutput::getVolWindowAtomAss(int step) {
	VarVec vec;

	// find old volatile atoms
	for(std::map<int, uint32_t>::iterator it = vol_atom_map_.begin(); it != vol_atom_map_.end(); ++it) {
		// extract assumption atoms from map if they expire after current step
		if(it->first > step) {
			vec.push_back(it->second);
		}
	}

	return vec;
}

void oClaspOutput::updateVolWindowAtoms(int step) {
	std::map<int, uint32_t>::iterator del = vol_atom_map_.begin();
	std::map<int, uint32_t>::const_iterator end = vol_atom_map_.end();

	// find old volatile atoms
	for(std::map<int, uint32_t>::iterator it = vol_atom_map_.begin(); it != end; ++it) {
		if(it->first <= step) {
			// unfreeze expired atoms
			b_->unfreeze(it->second);
			vol_window_atoms_frozen_.erase(it->second);

			// set deletion marker
			if(it->first >= del->first) {
				del = it;
			}
		}
	}

	// deprecate and erase old volatile atoms
	if(vol_atom_map_.size() > 0 && del->first <= step) {
		vol_atom_map_.erase(vol_atom_map_.begin(), ++del);
	}
}

void oClaspOutput::finalizeVolAtom() {
	if(vol_atom_ && vol_atom_ != vol_atom_frozen_) {
		b_->freeze(vol_atom_);
		vol_atom_frozen_ = vol_atom_;
	}
}

void oClaspOutput::deprecateVolAtom() {
	// deprecate vol atom if not null and not already deprecated
	if(vol_atom_ && (!vol_atoms_old_.size() || vol_atom_ != vol_atoms_old_.back())) {
		vol_atoms_old_.push_back(vol_atom_);
		vol_atom_ = 0;
	}
}

// make sure to call between updateProgram and endProgram
void oClaspOutput::unfreezeOldVolAtoms() {
	// unfreeze all old (deprecated) volatile atoms
	foreach(VarVec::value_type atom, vol_atoms_old_) {
		unfreezeAtom(atom);
	}
	vol_atoms_old_.clear();	
}

void oClaspOutput::doFinalize() {
	printExternalTable();
	ClaspOutput::doFinalize();
}

void oClaspOutput::printExternalTableEntry(const Symbol &symbol)
{
	ext_->addExternal(symbol.external);
}
