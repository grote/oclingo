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

GRINGO_EXPORT_PRINTER(ExtVolPrinter)

oClaspOutput::oClaspOutput(Grounder* grounder, Clasp::Solver* solver, bool shiftDisj)
	: iClaspOutput(shiftDisj)
	, ext_input_(false)
	, vol_atom_(0)
{
	ext_ = new ExternalKnowledge(grounder, this, solver);
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

void oClaspOutput::printBasicRule(int head, const AtomVec &pos, const AtomVec &neg) {
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
	std::cerr << "GOT NEW SYMBOL FOR VOL ATOM " << atom << std::endl; std::cerr.flush();
	b_->setAtomName(atom, 0);
	atomUnnamed_[atom - lastUnnamed_] = false;
	return atom;
}

void oClaspOutput::unfreezeAtom(uint32_t symbol) {
	std::cerr << "UNFREEZE " << symbol << std::endl;
	std::cerr.flush();
	b_->unfreeze(symbol);
}

uint32_t oClaspOutput::getVolAtom() {
	if(!vol_atoms_old_.size() || vol_atom_ == vol_atoms_old_.back()) {
		vol_atom_ = unnamedSymbol();
	}
	std::cerr << "GET VOL ATOM " << vol_atom_ << std::endl; std::cerr.flush();
	return vol_atom_;
}

// IMPORTANT: call after getVolAtomFalseAss()
uint32_t oClaspOutput::getVolAtomAss() {
	std::cerr << "GET VOL ATOM ASS " << vol_atoms_old_.size() << std::endl; std::cerr.flush();

	// deprecate vol atom if not null and not already deprecated
	if(vol_atom_ && (!vol_atoms_old_.size() || vol_atom_ != vol_atoms_old_.back())) {
		vol_atoms_old_.push_back(vol_atom_);
	}
	return vol_atom_;
}

VarVec& oClaspOutput::getVolAtomFalseAss() {
	return vol_atoms_old_;
}

void oClaspOutput::finalizeVolAtom() {
	std::cerr << "FINALIZE VOL ATOM " << std::endl; std::cerr.flush();

	if(vol_atom_) {
		std::cerr << "FREEZE VOL ATOM " << vol_atom_ << std::endl; std::cerr.flush();
		b_->freeze(vol_atom_);
	}

	// TODO unfreeze old volatile atoms if frozen last step
}

void oClaspOutput::doFinalize() {
	// TODO remove debug loops
	for(uint32_t domId = 0; domId < newSymbols_.size(); domId++)
	{
		for(std::vector<AtomRef>::iterator it = newSymbols_[domId].begin(); it != newSymbols_[domId].end(); it++)
		{
			std::cerr << "NEW SYMBOL: " << it->symbol << std::endl;
		}
	}

	printExternalTable();

	// add premature knowledge here, so externals are already defined
	ext_->addPrematureKnowledge();

	// freeze externals that were not defined with premature knowledge
	VarVec* externals = ext_->getFreezers();
	foreach(uint32_t symbol, *externals) {
		std::cerr << "FREEZE " << symbol << std::endl; std::cerr.flush();
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
