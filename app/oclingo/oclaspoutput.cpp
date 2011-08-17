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
	return getVolAtom(); // TODO remove this and take care of vol_window_atoms

	int step = window + ext_->getControllerStep();

	if(vol_atom_map_.find(step) == vol_atom_map_.end()) {
		vol_atom_map_[step] = unnamedSymbol();
	}

	return vol_atom_map_[step];
}

uint32_t oClaspOutput::getVolAtomAss() {
	return vol_atom_;
}

VarVec& oClaspOutput::getVolAtomFalseAss() {
	return vol_atoms_old_;
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

// TODO add volatile window handling

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
