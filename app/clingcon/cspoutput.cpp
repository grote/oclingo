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
#include <gringo/litdep.h>

CSPOutput::CSPOutput(bool shiftDisj, Clingcon::CSPSolver* cspsolver)
        : Clingcon::CSPOutputInterface(0, shiftDisj)
	, b_(0)
	, lastUnnamed_(0)
        , cspsolver_(cspsolver)
{
}

void CSPOutput::initialize()
{
        Clingcon::CSPOutputInterface::initialize();
	b_->setCompute(false_, false);
	lastUnnamed_ = atomUnnamed_.size();
	atomUnnamed_.clear();
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

void CSPOutput::printSymbolTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name)
{
	std::stringstream ss;
	ss << name;
	if(arity > 0)
	{
		ValVec::const_iterator k = vals_.begin() + atom.offset;
		ValVec::const_iterator end = k + arity;
		ss << "(";
		k->print(s_, ss);
		for(++k; k != end; ++k)
		{
			ss << ",";
			k->print(s_, ss);
		}
		ss << ")";
	}
	b_->setAtomName(atom.symbol, ss.str().c_str());
	atomUnnamed_[atom.symbol - lastUnnamed_] = false;
}

void CSPOutput::printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name)
{
	(void)atom;
	(void)arity;
	(void)name;
}

uint32_t CSPOutput::symbol()
{
	uint32_t atom = b_->newAtom();
	atomUnnamed_.resize(atom + 1 - lastUnnamed_, true);
	return atom;
}

uint32_t CSPOutput::symbol(const std::string& name, bool)
{
    uint32_t atom = b_->newAtom();
    b_->setAtomName(atom, name.c_str());
    b_->freeze(atom);
    return atom;
}

void CSPOutput::doFinalize()
{
	printSymbolTable();
	for(uint32_t i = 0; i < atomUnnamed_.size(); i++) { if(atomUnnamed_[i]) { b_->setAtomName(i + lastUnnamed_, 0); } }
	lastUnnamed_+= atomUnnamed_.size();
	atomUnnamed_.clear();

        //Clingcon::ConstraintVec* constraints = storage()->output()->printer<Clingcon::LParseCSPPrinter>()->getConstraints();
        Clingcon::ConstraintVec* constraints = static_cast<Clingcon::LParseCSPLitPrinter*>(storage()->output()->printer<Clingcon::CSPLit::Printer>())->getConstraints();
        //Clingcon::ConstraintVec* constraints = static_cast<Clingcon::LParseCSPPrinter*>(storage()->output()->printer<LparseConverter>())->getConstraints();


        for (Clingcon::ConstraintVec::iterator i = constraints->begin(); i != constraints->end(); ++i)
        {
            cspsolver_->addConstraint(*i->second,i->first);
        }

        Clingcon::LParseCSPDomainPrinter::Domains* domains = static_cast<Clingcon::LParseCSPDomainPrinter*>(storage()->output()->printer<Clingcon::CSPDomainLiteral::Printer>())->getDomains();

        for(Clingcon::LParseCSPDomainPrinter::Domains::iterator i = domains->begin(); i != domains->end(); ++i)
        {
            for (Clingcon::LParseCSPDomainPrinter::Domain::iterator j = i->second.begin(); j != i->second.end(); ++j)
                cspsolver_->addDomain(i->first, j->lower, j->upper);
        }

        const Clingcon::LParseCSPDomainPrinter::Domain& r = static_cast<Clingcon::LParseCSPDomainPrinter*>(storage()->output()->printer<Clingcon::CSPDomainLiteral::Printer>())->getDefaultDomain();
        for (Clingcon::LParseCSPDomainPrinter::Domain::const_iterator i = r.begin(); i != r.end(); ++i)
            cspsolver_->addDomain(i->lower, i->upper);

        Clingcon::LParseGlobalConstraintPrinter::GCvec& t = static_cast<Clingcon::LParseGlobalConstraintPrinter*>(storage()->output()->printer<Clingcon::GlobalConstraint::Printer>())->getGlobalConstraints();
        cspsolver_->addGlobalConstraints(t);
        //for (Clingcon::LParseGlobalConstraintPrinter::GCvec::const_iterator i = t.begin(); i != t.end(); ++i)
        //    cspsolver_->addGlobalConstraint(i->release());

}

const LparseConverter::SymbolMap &CSPOutput::symbolMap(uint32_t domId) const
{
	return symTab_[domId];
}

ValRng CSPOutput::vals(Domain *dom, uint32_t offset) const
{
	return ValRng(vals_.begin() + offset, vals_.begin() + offset + dom->arity());
}

CSPOutput::~CSPOutput()
{
}

iCSPOutput::iCSPOutput(bool shiftDisj, Clingcon::CSPSolver* cspsolver)
        : CSPOutput(shiftDisj,cspsolver)
	, incUid_(0)
{
}

void iCSPOutput::initialize()
{
        if(!incUid_) { CSPOutput::initialize(); }
	else { b_->unfreeze(incUid_); }
	// create a new uid
	incUid_ = symbol();
	b_->freeze(incUid_);
}

int iCSPOutput::getIncAtom()
{
	return incUid_;
}


iCSPOutput::~iCSPOutput()
{
}
