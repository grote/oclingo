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

#include "clingcon/cspoutput.h"
#include <clasp/program_builder.h>
#include <gringo/storage.h>
#include <gringo/domain.h>
#include <gringo/lparseconverter.h>
#include <clingcon/cspprinter.h>
#include <clingcon/cspconstraint.h>
#include <clingcon/cspprinter.h>
#include <clingcon/cspglobalprinter.h>
#include <clingcon/groundconstraintvarlit.h>
#include <gringo/litdep.h>

CSPOutput::CSPOutput(bool shiftDisj, IncConfig &config, bool incremental, Clingcon::CSPSolver* cspsolver)
        : Clingcon::CSPOutputInterface(shiftDisj)
	, b_(0)
        , config_(config)
        , initialized(false)
        , trueAtom_(0)
        , incremental_(incremental)
        , cspsolver_(cspsolver)
{
}

void CSPOutput::initialize()
{
    if (!initialized)
    {
        initialized=true;
        Clingcon::CSPOutputInterface::initialize();
	b_->setCompute(false_, false);
    }
}

void CSPOutput::printBasicRule(uint32_t head, const AtomVec &pos, const AtomVec &neg)
{
	b_->startRule();
	b_->addHead(head);
	foreach(AtomVec::value_type atom, neg) { b_->addToBody(atom, false); }
	foreach(AtomVec::value_type atom, pos) { b_->addToBody(atom, true); }
	b_->endRule();
}

void CSPOutput::printConstraintRule(uint32_t head, int bound, const AtomVec &pos, const AtomVec &neg)
{
	b_->startRule(Clasp::CONSTRAINTRULE, bound);
	b_->addHead(head);
	foreach(AtomVec::value_type atom, neg) { b_->addToBody(atom, false); }
	foreach(AtomVec::value_type atom, pos) { b_->addToBody(atom, true); }
	b_->endRule();
}

void CSPOutput::printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg)
{
	b_->startRule(Clasp::CHOICERULE);
	foreach(AtomVec::value_type atom, head) { b_->addHead(atom); }
	foreach(AtomVec::value_type atom, neg) { b_->addToBody(atom, false); }
	foreach(AtomVec::value_type atom, pos) { b_->addToBody(atom, true); }
	b_->endRule();
}

void CSPOutput::printWeightRule(uint32_t head, int bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg)
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

void CSPOutput::printMinimizeRule(const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg)
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

void CSPOutput::printDisjunctiveRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg)
{
	(void)head;
	(void)pos;
	(void)neg;
	throw std::runtime_error("Error: clasp cannot handle disjunctive rules use option --shift!");
}

void CSPOutput::printComputeRule(int models, const AtomVec &pos, const AtomVec &neg)
{
	(void)models;
	foreach(AtomVec::value_type atom, neg) { b_->setCompute(atom, false); }
	foreach(AtomVec::value_type atom, pos) { b_->setCompute(atom, true); }
}



void CSPOutput::printSymbolTableEntry(uint32_t symbol, const std::string &name)
{
       b_->setAtomName(symbol, name.c_str());
}


void CSPOutput::printExternalTableEntry(const Symbol &symbol)
{
    externalAtoms_[config_.incStep].push_back(symbol.symbol);
    b_->freeze(symbol.symbol);
}

uint32_t CSPOutput::symbol()
{
        return b_->newAtom();
}

uint32_t CSPOutput::symbol(const std::string& name, bool freeze)
{
    uint32_t atom = symbol();
    b_->setAtomName(atom, name.c_str());
    if (freeze)
    {
        AtomVec vec;
        vec.push_back(atom);
        printChoiceRule(vec,AtomVec(),AtomVec());
    }
    return atom;
}

void CSPOutput::doFinalize()
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

        //Clingcon::ConstraintVec* constraints = storage()->output()->printer<Clingcon::LParseCSPPrinter>()->getConstraints();
        Clingcon::ConstraintVec* constraints = static_cast<Clingcon::LParseCSPLitPrinter*>(storage()->output()->printer<Clingcon::CSPLit::Printer>())->getConstraints();
        //Clingcon::ConstraintVec* constraints = static_cast<Clingcon::LParseCSPPrinter*>(storage()->output()->printer<LparseConverter>())->getConstraints();


        for (Clingcon::ConstraintVec::iterator i = constraints->begin(); i != constraints->end(); ++i)
        {
            cspsolver_->addConstraint(i->second,i->first);
        }

        //Clingcon::LParseCSPDomainPrinter::Domains* domains = static_cast<Clingcon::LParseCSPDomainPrinter*>(storage()->output()->printer<Clingcon::CSPDomainLiteral::Printer>())->getDomains();

        /*
        for(Clingcon::LParseCSPDomainPrinter::Domains::iterator i = domains->begin(); i != domains->end(); ++i)
        {
            for (Clingcon::LParseCSPDomainPrinter::Domain::iterator j = i->second.begin(); j != i->second.end(); ++j)
                cspsolver_->addDomain(i->first, j->lower, j->upper);
        }*/

        const Clingcon::LParseCSPDomainPrinter::Domain& r = static_cast<Clingcon::LParseCSPDomainPrinter*>(storage()->output()->printer<Clingcon::CSPDomainLiteral::Printer>())->getDefaultDomain();
        cspsolver_->setDomain(r.left, r.right);

        Clingcon::LParseGlobalConstraintPrinter::GCvec& t = static_cast<Clingcon::LParseGlobalConstraintPrinter*>(storage()->output()->printer<Clingcon::GlobalConstraint::Printer>())->getGlobalConstraints();
        cspsolver_->addGlobalConstraints(t);
        //for (Clingcon::LParseGlobalConstraintPrinter::GCvec::const_iterator i = t.begin(); i != t.end(); ++i)
        //    cspsolver_->addGlobalConstraint(i->release());

}

/*
const LparseConverter::SymbolMap &CSPOutput::symbolMap(uint32_t domId) const
{
	return symTab_[domId];
}
*/


/*
ValRng CSPOutput::vals(Domain *dom, uint32_t offset) const
{
	return ValRng(vals_.begin() + offset, vals_.begin() + offset + dom->arity());
}*/

void CSPOutput::forgetStep(int step)
{
    ExternalMap::iterator it = externalAtoms_.find(step);
    if (it != externalAtoms_.end())
    {
        foreach (uint32_t sym, it->second) { b_->unfreeze(sym); }
        externalAtoms_.erase(it);
    }
}

uint32_t CSPOutput::getNewVolUid(int step)
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

uint32_t CSPOutput::getVolAtom(int vol_window)
{
    // volatile atom expires at current step + vol window size
    return getNewVolUid(config_.incStep + vol_window);
}

uint32_t CSPOutput::getAssertAtom(Val term)
{
    uint32_t &sym = assertUids_[term];
    if (!sym)
    {
        sym = symbol();
        b_->freeze(sym);
    }
    return sym;
}


void CSPOutput::retractAtom(Val term)
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

CSPOutput::~CSPOutput()
{
}
