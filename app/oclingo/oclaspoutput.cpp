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

oClaspOutput::oClaspOutput(Grounder* grounder, Clasp::Solver* solver, bool shiftDisj, IncConfig &config, uint32_t port, bool import)
	: ClaspOutput(shiftDisj, config, true)
	, vol_atom_(0)
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

uint32_t oClaspOutput::getQueryAtom() {
	if(vol_atom_ == 0) {
		vol_atom_ = symbol();
		b_->freeze(vol_atom_);
	}
	return vol_atom_;
}

// make sure to call between updateProgram and endProgram
void oClaspOutput::deactivateQueryAtom() {
	if(vol_atom_) {
		b_->startRule().addHead(vol_atom_).endRule();
		vol_atom_ = 0;
	}
}

// volatile parts from controller need to be relative to controller step
uint32_t oClaspOutput::getVolTimeDecayAtom(int window) {
	return getNewVolUid(ext_->getControllerStep() + window);
}
