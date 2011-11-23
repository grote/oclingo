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

ClaspOutput::ClaspOutput(bool shiftDisj)
	: LparseConverter(shiftDisj)
	, b_(0)
{
}

void ClaspOutput::initialize()
{
	LparseConverter::initialize();
	b_->setCompute(false_, false);
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
	(void) symbol;
}

uint32_t ClaspOutput::symbol()
{
	return b_->newAtom();
}

void ClaspOutput::doFinalize()
{
	printSymbolTable();
}

ClaspOutput::~ClaspOutput()
{
}

iClaspOutput::iClaspOutput(bool shiftDisj, IncConfig &config)
	: ClaspOutput(shiftDisj)
	, config_(config)
	, initialized(false)
{
}

void iClaspOutput::initialize()
{
	if(!initialized) {
		initialized = true;
		ClaspOutput::initialize();
	}
	// unfreeze volatile atom for coming step
	else {
		VolMap::iterator it = volUids_.find(config_.incStep+1);
		if(it != volUids_.end()) {
			b_->unfreeze(volUids_[config_.incStep+1]);
			volUids_.erase(it);
		}
	}
}

uint32_t iClaspOutput::getNewVolUid(int step)
{
	if(volUids_.find(step) == volUids_.end()) {
		// create a new uid
		volUids_[step] = symbol();
		b_->freeze(volUids_[step]);
	}

	return volUids_[step];
}

uint32_t iClaspOutput::getVolAtom(int vol_window = 1)
{
	// volatile atom expires at current step + vol window size
	return getNewVolUid(config_.incStep + vol_window);
}

ClaspOutput::VolMap iClaspOutput::getVolUids()
{
	return volUids_;
}

uint32_t iClaspOutput::getAssertAtom(Val term)
{
	if(assertUids_.find(term) == assertUids_.end()) {
		// create a new uid
		assertUids_[term] = symbol();
		b_->freeze(assertUids_[term]);
	}

	return assertUids_[term];
}

ClaspOutput::AssertMap iClaspOutput::getAssertUids()
{
	return assertUids_;
}

void iClaspOutput::retractAtom(Val term)
{
	// TODO add better error handling
	AssertMap::iterator it = assertUids_.find(term);
	if(it != assertUids_.end()) {
		b_->unfreeze(assertUids_[term]);
		assertUids_.erase(it);
	} else {
		std::cerr << "Error: Term ";
		term.print(storage(), std::cerr);
		std::cerr << " was not used to assert rules.";
	}
}
