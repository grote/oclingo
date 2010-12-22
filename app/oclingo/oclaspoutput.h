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

#pragma once

//#include <gringo/gringo.h>
#include "clingo/claspoutput.h"
#include "oclingo/externalknowledge.h"

class oClaspOutput : public iClaspOutput
{
public:
	oClaspOutput(Grounder* grounder, Clasp::Solver* solver, bool shiftDisj);
	ExternalKnowledge& getExternalKnowledge();
	void unfreezeAtom(uint32_t symbol);
protected:
	void doFinalize();
	void printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name);
private:
	ExternalKnowledge* ext_;
};
