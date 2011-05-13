// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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


#include <gringo/gringo.h>
#include <gringo/display.h>
#include <gringo/optimize.h>
#include <gringo/compute.h>
#include <gringo/external.h>
#include <gringo/junctionlit.h>
#include <gringo/predlitrep.h>
#include <gringo/domain.h>
#include <gringo/storage.h>

#include "output/lparserule_impl.h"

namespace lparseconverter_impl
{

class DisplayPrinter : public Display::Printer
{
public:
	DisplayPrinter(LparseConverter *output) : output_(output) { }
	void print(PredLitRep *l);
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter *output_;
};

class ExternalPrinter : public External::Printer
{
public:
	ExternalPrinter(LparseConverter *output) : output_(output) { }
	void print(PredLitRep *l);
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter *output_;
};

class OptimizePrinter : public Optimize::Printer
{
public:
	OptimizePrinter(LparseConverter *output) : output_(output) { }
	void begin(bool maximize, bool set) { maximize_ = maximize; (void)set; }
	void print(PredLitRep *l, int32_t weight, int32_t prio);
	void end() { }
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter *output_;
	bool maximize_;
};

class ComputePrinter : public Compute::Printer
{
public:
	ComputePrinter(LparseConverter *output) : output_(output) { }
	void print(PredLitRep *l);
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter *output_;
};

class JunctionPrinter : public JunctionLit::Printer
{
public:
	JunctionPrinter(LparseConverter *output) : output_(output) { }
	void begin(bool head);
	void print(PredLitRep *l);
	void weight(const Val &v) { (void)v; }
	void end();
	Output *output() const { return output_; }
private:
	LparseConverter           *output_;
	LparseConverter::AtomVec   pos_;
	LparseConverter::AtomVec   neg_;
	bool                       head_;
};

class IncPrinter : public IncLit::Printer
{
public:
	IncPrinter(LparseConverter *output) : output_(output) {  }
	void print(PredLitRep *l);
	void print();
	Output *output() const { return output_; }
private:
	LparseConverter *output_;
};

///////////////////////////////// DisplayPrinter /////////////////////////////////

void DisplayPrinter::print(PredLitRep *l)
{
	if(show()) output_->showAtom(l);
	else output_->hideAtom(l);
}

///////////////////////////////// ExternalPrinter /////////////////////////////////

void ExternalPrinter::print(PredLitRep *l)
{
	output_->externalAtom(l);
}

///////////////////////////////// OptimizePrinter /////////////////////////////////

void OptimizePrinter::print(PredLitRep *l, int32_t weight, int32_t prio)
{
	output_->prioLit(l, weight, prio, maximize_);
}

///////////////////////////////// ComputePrinter /////////////////////////////////

void ComputePrinter::print(PredLitRep *l)
{
	output_->addCompute(l);
}

///////////////////////////////// JunctionPrinter /////////////////////////////////

void JunctionPrinter::begin(bool head)
{
	head_ = head;
	pos_.clear();
	neg_.clear();
}

void JunctionPrinter::print(PredLitRep *l)
{
	uint32_t sym = output_->symbol(l);
	assert(sym > 0);
	if(l->sign()) { neg_.push_back(sym); }
	else          { pos_.push_back(sym); }
}

void JunctionPrinter::end()
{
	RulePrinter *printer = static_cast<RulePrinter *>(output_->printer<Rule::Printer>());
	if(head_)
	{
		if(pos_.size() == 1 && neg_.size() == 0) { printer->setHead(pos_[0]); }
		else if(!output_->shiftDisjunctions())   { printer->setHead(pos_, false); }
		else
		{
			// add replacement as head
			uint32_t d = output_->symbol();
			printer->setHead(d);

			// write a shifted rule for every atom in the disjunction
			for(size_t i = 0; i < pos_.size(); i++)
			{
				LparseConverter::AtomVec neg;
				for(size_t k = 0; k < pos_.size(); k++)
				{
					if(k != i) { neg.push_back(pos_[k]); }
				}
				output_->printBasicRule(pos_[i], LparseConverter::AtomVec(1, d), neg);
			}

		}
	}
	else
	{
		for(size_t i = 0; i < pos_.size(); i++) { printer->addBody(pos_[i], false); }
		for(size_t i = 0; i < neg_.size(); i++) { printer->addBody(neg_[i], true); }
	}
}

///////////////////////////////// IncPrinter /////////////////////////////////

void IncPrinter::print(PredLitRep *) { }

void IncPrinter::print()
{
	RulePrinter *printer = static_cast<RulePrinter *>(output_->printer<Rule::Printer>());
	int atom = output_->getIncAtom();
	if(atom > 0) { printer->addBody(atom, false); }
}

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::DisplayPrinter, Display::Printer, LparseConverter)
//GRINGO_REGISTER_PRINTER(lparseconverter_impl::OptimizePrinter, Optimize::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::ComputePrinter, Compute::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::ExternalPrinter, External::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::JunctionPrinter, JunctionLit::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::IncPrinter, IncLit::Printer, LparseConverter)
