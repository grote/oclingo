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

oClaspOutput::oClaspOutput(Grounder* grounder, Clasp::Solver* solver, bool shiftDisj, uint32_t port)
	: iClaspOutput(shiftDisj)
	, ext_input_(false)
	, vol_atom_(0)
	, vol_atom_frozen_(0)
{
	ext_ = new ExternalKnowledge(grounder, this, solver, port);
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
		if(ext_->checkHead(head)) {
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
	return getVolAtom(); // TODO remove this and take care of freeze/unfreeze vol_window_atoms

	int step = window + ext_->getControllerStep() + 1;

	if(vol_atom_map_.find(step) == vol_atom_map_.end()) {
		vol_atom_map_[step] = unnamedSymbol();
	}

	// freeze new atom right away
	b_->freeze(vol_atom_map_[step]);
	std::cout << "FREEZE VALUE " << vol_atom_map_[step] << " FOR KEY " << step << std::endl;

	return vol_atom_map_[step];
}

uint32_t oClaspOutput::getVolAtomAss() {
	return vol_atom_;
}

VarVec oClaspOutput::getVolWindowAtomAss(int step) {
	std::cerr << "CURRENT STEP: " << step << std::endl;

	VarVec vec;
	std::map<int, uint32_t>::iterator del = vol_atom_map_.begin();
	std::map<int, uint32_t>::const_iterator end = vol_atom_map_.end();

	// find old volatile atoms
	for(std::map<int, uint32_t>::iterator it = vol_atom_map_.begin(); it != end; ++it) {
		std::cout << "KEY " << it->first << std::endl;
		std::cout << "VALUE " << it->second << std::endl;
		// extract assumption atoms from map
		if(it->first > step) {
			vec.push_back(it->second);
			std::cout << "ASS KEY " << it->first << std::endl;
			std::cout << "ASS VALUE " << it->second << std::endl;
		} else {
			// make sure old atoms are unfreezed
			vol_window_atoms_old_.push_back(it->second);
			std::cout << "OLD KEY " << it->first << std::endl;
			std::cout << "OLD VALUE " << it->second << std::endl;

			// set deletion marker
			if(it->first >= del->first) {
				del = it;
				std::cout << "SET NEW BORDER TO " << del->first;
				std::cout << " - " << del->second << std::endl;
			}
		}
	}

	// deprecate and erase old volatile atoms
	if(vol_atom_map_.size() > 0 && del->first <= step) {
		// vol_window_atoms_old_.push_back()
		if(del == vol_atom_map_.end()) {
			vol_atom_map_.clear();
			std::cerr << "   del == end" << std::endl;
		} else {
			vol_atom_map_.erase(vol_atom_map_.begin(), ++del);
			std::cerr << "   del++" << std::endl;
		}
	}

	return vec;
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
	// also unfreeze old volatile sliding window atoms
	vol_atoms_old_.insert(vol_atoms_old_.end(), vol_window_atoms_old_.begin(), vol_window_atoms_old_.end());

	// unfreeze all old (deprecated) volatile atoms
	foreach(VarVec::value_type atom, vol_atoms_old_) {
		unfreezeAtom(atom);
	}
	vol_atoms_old_.clear();
	vol_window_atoms_old_.clear();
}

void oClaspOutput::doFinalize() {
	printExternalTable();

	// add premature knowledge here, so externals are already defined
	ext_->addPrematureKnowledge();

	// freeze externals that were not defined with premature knowledge
	VarVec* externals = ext_->getFreezers();
	foreach(uint32_t symbol, *externals) {
		b_->freeze(symbol);
	}
	externals->clear();

	ClaspOutput::doFinalize();
}

void oClaspOutput::printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name)
{
	(void) arity;
	(void) name;
	ext_->addExternal(atom.symbol);
}
