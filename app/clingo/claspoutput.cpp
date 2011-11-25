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

#include "clingo/claspoutput.h"
#include <clasp/program_builder.h>
#include <gringo/storage.h>
#include <gringo/domain.h>

ClaspOutput::ClaspOutput(bool shiftDisj, IncConfig &config, bool incremental)
	: LparseConverter(shiftDisj)
	, b_(0)
	, config_(config)
	, initialized(false)
	, trueAtom_(0)
	, incremental_(incremental)
{
}

void ClaspOutput::initialize()
{
	if (!initialized)
	{
		initialized = true;
		LparseConverter::initialize();
		b_->setCompute(false_, false);
	}
}

void ClaspOutput::printBasicRule(uint32_t head, const AtomVec &pos, const AtomVec &neg)
{
	b_->startRule();
	b_->addHead(head);
	foreach(AtomVec::value_type atom, neg) { b_->addToBody(atom, false); }
	foreach(AtomVec::value_type atom, pos) { b_->addToBody(atom, true); }
	b_->endRule();
}

void ClaspOutput::printConstraintRule(uint32_t head, int32_t bound, const AtomVec &pos, const AtomVec &neg)
{
	b_->startRule(Clasp::CONSTRAINTRULE, bound);
	b_->addHead(head);
	foreach(AtomVec::value_type atom, neg) { b_->addToBody(atom, false); }
	foreach(AtomVec::value_type atom, pos) { b_->addToBody(atom, true); }
	b_->endRule();
}

void ClaspOutput::printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg)
{
	b_->startRule(Clasp::CHOICERULE);
	foreach(AtomVec::value_type atom, head) { b_->addHead(atom); }
	foreach(AtomVec::value_type atom, neg) { b_->addToBody(atom, false); }
	foreach(AtomVec::value_type atom, pos) { b_->addToBody(atom, true); }
	b_->endRule();
}

void ClaspOutput::printWeightRule(uint32_t head, int32_t bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg)
{
	b_->startRule(Clasp::WEIGHTRULE, bound);
	b_->addHead(head);
	WeightVec::const_iterator itW = wNeg.begin();
	for(AtomVec::const_iterator it = neg.begin(); it != neg.end(); it++, itW++)
		b_->addToBody(*it, false, *itW);
	itW = wPos.begin();
	for(AtomVec::const_iterator it = pos.begin(); it != pos.end(); it++, itW++)
		b_->addToBody(*it, true, *itW);
	b_->endRule();
}

void ClaspOutput::printMinimizeRule(const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg)
{
	b_->startRule(Clasp::OPTIMIZERULE);
	WeightVec::const_iterator itW = wNeg.begin();
	for(AtomVec::const_iterator it = neg.begin(); it != neg.end(); it++, itW++)
		b_->addToBody(*it, false, *itW);
	itW = wPos.begin();
	for(AtomVec::const_iterator it = pos.begin(); it != pos.end(); it++, itW++)
		b_->addToBody(*it, true, *itW);
	b_->endRule();
}

void ClaspOutput::printDisjunctiveRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg)
{
	(void)head;
	(void)pos;
	(void)neg;
	throw std::runtime_error("Error: clasp cannot handle disjunctive rules use option --shift!");
}

void ClaspOutput::printComputeRule(int models, const AtomVec &pos, const AtomVec &neg)
{
	(void)models;
	foreach(AtomVec::value_type atom, neg) { b_->setCompute(atom, false); }
	foreach(AtomVec::value_type atom, pos) { b_->setCompute(atom, true); }
}

void ClaspOutput::printSymbolTableEntry(uint32_t symbol, const std::string &name)
{
	b_->setAtomName(symbol, name.c_str());
}

void ClaspOutput::printExternalTableEntry(const Symbol &symbol)
{
	externalAtoms_[config_.incStep].push_back(symbol.symbol);
	b_->freeze(symbol.symbol);
}

uint32_t ClaspOutput::symbol()
{
	return b_->newAtom();
}

void ClaspOutput::doFinalize()
{
	printSymbolTable();
	printExternalTable();
	VolMap::iterator end = volUids_.lower_bound(config_.incStep + 1);
	for (VolMap::iterator it = volUids_.begin(); it != end; it++)
	{
		b_->startRule().addHead(it->second).endRule();
	}
	volUids_.erase(volUids_.begin(), end);
	// Note: make sure that there is always a volatile atom
	// this is important to prevent clasp from polluting its top level
	if (incremental_) { getVolAtom(1); }
}

void ClaspOutput::forgetStep(int step)
{
	ExternalMap::iterator it = externalAtoms_.find(step);
	if (it != externalAtoms_.end())
	{
		foreach (uint32_t sym, it->second) { b_->unfreeze(sym); }
		externalAtoms_.erase(it);
	}
}

uint32_t ClaspOutput::getNewVolUid(int step)
{
	if (step < config_.incStep + 1)
	{
		// this case should be relatively unlikely
		// there are better ways to handle an out-of-order volatile statements
		if (!trueAtom_) 
		{
			trueAtom_ = symbol();
			b_->startRule().addHead(trueAtom_).endRule();
		}
		return trueAtom_;
	}
	else
	{
		uint32_t &sym = volUids_[step];
		if(!sym)
		{
			sym = symbol();
			b_->freeze(sym);
		}
		return sym;
	}
}

uint32_t ClaspOutput::getVolAtom(int vol_window)
{
	// volatile atom expires at current step + vol window size
	return getNewVolUid(config_.incStep + vol_window);
}

uint32_t ClaspOutput::getAssertAtom(Val term)
{
	uint32_t &sym = assertUids_[term];
	if (!sym)
	{
		sym = symbol();
		b_->freeze(sym);
	}
	return sym;
}

void ClaspOutput::retractAtom(Val term)
{
	// TODO add better error handling
	AssertMap::iterator it = assertUids_.find(term);
	if(it != assertUids_.end())
	{
		b_->startRule().addHead(it->second).endRule();
		assertUids_.erase(it);
	}
	else
	{
		std::cerr << "Error: Term ";
		term.print(storage(), std::cerr);
		std::cerr << " was not used to assert rules.";
	}
}

ClaspOutput::~ClaspOutput()
{
}

