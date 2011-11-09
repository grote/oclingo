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

#include <map>
#include "clingo/claspoutput.h"

class ExternalKnowledge;

class oClaspOutput : public iClaspOutput
{
public:
	oClaspOutput(Grounder* grounder, Clasp::Solver* solver, bool shiftDisj, IncConfig &config, uint32_t port, bool import);
	~oClaspOutput();
	ExternalKnowledge& getExternalKnowledge();
	void startExtInput();
	void stopExtInput();
	void printBasicRule(uint32_t head, const AtomVec &pos, const AtomVec &neg);
	uint32_t getVolAtom(int vol_window);
	void freezeAtom(uint32_t symbol);
	void unfreezeAtom(uint32_t symbol);
	uint32_t getVolExtAtom();
	uint32_t getVolWindowAtom(int window);
	uint32_t getVolAtomAss();
	VarVec   getVolWindowAtomAss(int step);
	void updateVolWindowAtoms(int step);
	void finalizeVolAtom();
	void deprecateVolAtom();
	void unfreezeOldVolAtoms();
protected:
	void doFinalize();
	void printExternalTableEntry(const Symbol &symbol);
private:
	ExternalKnowledge* ext_;
	bool ext_input_;
	uint32_t vol_atom_;
	uint32_t vol_atom_frozen_;
	VarVec vol_atoms_old_;
	VarSet vol_window_atoms_frozen_;

	std::map<int, uint32_t> vol_atom_map_;
};
