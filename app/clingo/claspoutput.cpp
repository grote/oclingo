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
	, lastUnnamed_(0)
{
}

void ClaspOutput::initialize()
{
	LparseConverter::initialize();
	b_->setCompute(false_, false);
	lastUnnamed_ = atomUnnamed_.size();
	atomUnnamed_.clear();
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
	atomUnnamed_[symbol - lastUnnamed_] = false;
}

void ClaspOutput::printExternalTableEntry(uint32_t, uint32_t)
{
}

uint32_t ClaspOutput::symbol()
{
	uint32_t atom = b_->newAtom();
	atomUnnamed_.resize(atom + 1 - lastUnnamed_, true);
	return atom;
}

void ClaspOutput::doFinalize()
{
	printSymbolTable();
	for(uint32_t i = 0; i < atomUnnamed_.size(); i++) { if(atomUnnamed_[i]) { b_->setAtomName(i + lastUnnamed_, 0); } }
	lastUnnamed_+= atomUnnamed_.size();
	atomUnnamed_.clear();
}

const LparseConverter::SymbolMap &ClaspOutput::symbolMap(uint32_t domId) const
{
	return symTab_[domId];
}

ValRng ClaspOutput::vals(Domain *dom, uint32_t offset) const
{
	return ValRng(vals_.begin() + offset, vals_.begin() + offset + dom->arity());
}

ClaspOutput::~ClaspOutput()
{
}

iClaspOutput::iClaspOutput(bool shiftDisj)
	: ClaspOutput(shiftDisj)
	, initialized(false)
{
}

void iClaspOutput::initialize()
{
	if(!initialized) {
		initialized = true;
		ClaspOutput::initialize();
	}
	else {
		if(incUids_.at(0)) b_->unfreeze(incUids_.at(0));
		incUids_.pop_front();
	}
}

uint32_t iClaspOutput::getNewIncUid()
{
	// create a new uid
	int uid = symbol();
	b_->freeze(uid);

	return uid;
}

int iClaspOutput::getIncAtom(uint32_t vol_window)
{
	if(incUids_.size() < vol_window) {
		incUids_.resize(vol_window, 0);
	}
	if(incUids_.at(vol_window-1) == 0) {
		incUids_.at(vol_window-1) = getNewIncUid();
	}

	return incUids_.at(vol_window-1);
}

std::deque<uint32_t> iClaspOutput::getIncUids()
{
	return incUids_;
}
