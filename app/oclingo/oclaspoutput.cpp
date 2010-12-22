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

oClaspOutput::oClaspOutput(Grounder* grounder, Clasp::Solver* solver, bool shiftDisj) : iClaspOutput(shiftDisj) {
	ext_ = new ExternalKnowledge(grounder, this, solver);
}

ExternalKnowledge& oClaspOutput::getExternalKnowledge() {
	assert(ext_);
	return *ext_;
}

void oClaspOutput::unfreezeAtom(uint32_t symbol) {
	std::cerr << "UNFREEZE " << symbol << std::endl;
	std::cerr.flush();
	b_->unfreeze(symbol);
}

void oClaspOutput::doFinalize()
{
	for(uint32_t domId = 0; domId < newSymbols_.size(); domId++)
	{
		for(std::vector<AtomRef>::iterator it = newSymbols_[domId].begin(); it != newSymbols_[domId].end(); it++)
		{
			std::cerr << "NEW SYMBOL: " << it->symbol << std::endl;
		}
	}

	ClaspOutput::doFinalize();
	printExternalTable();
}

void oClaspOutput::printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name)
{
	ext_->addExternal(atom.symbol);
	std::cerr << "FREEZE " << atom.symbol << "   (" << name << "/" << arity << " Offset: " << atom.offset << ")" <<  std::endl;
	b_->freeze(atom.symbol);
}

/*
void oClaspOutput::initialize()
{
	if(!incUid_) { ClaspOutput::initialize(); }
	else { b_->unfreeze(incUid_); }
	// create a new uid
	incUid_ = symbol();
	b_->freeze(incUid_);
}

int oClaspOutput::getIncAtom()
{
	return incUid_;
}
*/
