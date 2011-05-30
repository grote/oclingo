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

#include <gringo/gringo.h>
#include <gringo/lparseconverter.h>
#include <clasp/program_builder.h>
#include <clingcon/cspoutputinterface.h>
#include <clingcon/cspsolver.h>

class CSPOutput : public Clingcon::CSPOutputInterface
{
	typedef std::vector<bool> BoolVec;
public:
        CSPOutput(bool shiftDisj, Clingcon::CSPSolver* cspsolver);
	virtual void initialize();
	void setProgramBuilder(Clasp::ProgramBuilder* api) { b_ = api; }
	const SymbolMap &symbolMap(uint32_t domId) const;
	ValRng vals(Domain *dom, uint32_t offset) const;
        ~CSPOutput();
protected:
        virtual void printBasicRule(uint32_t head, const AtomVec &pos, const AtomVec &neg);
        void printConstraintRule(uint32_t head, int bound, const AtomVec &pos, const AtomVec &neg);
	void printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg);
        void printWeightRule(uint32_t head, int bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg);
	void printMinimizeRule(const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg);
	void printDisjunctiveRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg);
	void printComputeRule(int models, const AtomVec &pos, const AtomVec &neg);
	void printSymbolTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name);
	void printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name);
	uint32_t symbol();
        uint32_t symbol(const std::string& name, bool freeze);
	virtual void doFinalize();
protected:
	Clasp::ProgramBuilder *b_;
	BoolVec  atomUnnamed_;
	uint32_t lastUnnamed_;
        Clingcon::CSPSolver* cspsolver_;
};

class iCSPOutput : public CSPOutput
{
public:
        iCSPOutput(bool shiftDisj, Clingcon::CSPSolver* cspsolver);
        ~iCSPOutput();
	void initialize();
	int getIncAtom();
private:
	int incUid_;
};
