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
#include <gringo/show.h>
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
	void begin(const Val &head, Type type);
	void print(PredLitRep *l);
	void end();
	LparseConverter *output() const { return output_; }
private:
	LparseConverter        *output_;
	LparseConverter::LitVec body_;
	Val                     head_;
	Display::Type           type_;
};

class ExternalPrinter : public External::Printer
{
public:
	ExternalPrinter(LparseConverter *output) : output_(output) { }
	void print(PredLitRep *l);
	LparseConverter *output() const { return output_; }
private:
	LparseConverter *output_;
};

class OptimizePrinter : public Optimize::Printer
{
	typedef LparseConverter::LitVec LitVec;
public:
	OptimizePrinter(LparseConverter *output) : output_(output) { }
	void begin(Optimize::Type type, bool maximize);
	void print(const ValVec &set);
	void print(PredLitRep *l);
	void end();
	LparseConverter *output() const { return output_; }
private:
	LparseConverter *output_;
	bool             maximize_;
	LitVec           lits_;
	ValVec           set_;
};

class ComputePrinter : public Compute::Printer
{
public:
	ComputePrinter(LparseConverter *output) : output_(output) { }
	void print(PredLitRep *l);
	LparseConverter *output() const { return output_; }
private:
	LparseConverter *output_;
};

class JunctionLitPrinter : public JunctionLit::Printer, public DelayedPrinter
{
	typedef std::pair<uint32_t, uint32_t> CondKey;
	typedef std::pair<uint32_t, LparseConverter::LitVec> CondValue;
	typedef std::map<CondKey, CondValue> CondMap;
public:
	JunctionLitPrinter(LparseConverter *output);
	void beginHead(bool disjunction, uint32_t uidJunc, uint32_t uidSubst);
	void beginBody();
	void printCond(bool bodyComplete);
	void printJunc(bool disjunction, uint32_t juncUid, uint32_t substUid);
	void print(PredLitRep *l);
	void finish();
	LparseConverter *output() const { return output_; }
private:
	LparseConverter         *output_;
	LparseConverter::LitVec *current_;
	LparseConverter::LitVec  head_;
	LparseConverter::LitVec  body_;
	CondMap                  condMap_;
	bool                     disjunction_;
};

class IncPrinter : public IncLit::Printer
{
public:
	IncPrinter(LparseConverter *output) : output_(output) {  }
	void print(PredLitRep *l);
	void print(uint32_t vol_window);
	LparseConverter *output() const { return output_; }
private:
	LparseConverter *output_;
};

///////////////////////////////// DisplayPrinter /////////////////////////////////

void DisplayPrinter::begin(const Val &head, Type type)
{
	head_ = head;
	type_ = type;
	body_.clear();
}

void DisplayPrinter::print(PredLitRep *l)
{
	body_.push_back(int32_t(l->sign() ? -1 : 1) * output_->symbol(l));
}

void DisplayPrinter::end()
{
	output()->display(head_, body_, type_ != Display::HIDEPRED);
}

///////////////////////////////// ExternalPrinter /////////////////////////////////

void ExternalPrinter::print(PredLitRep *l)
{
	output_->externalAtom(l);
}

///////////////////////////////// OptimizePrinter /////////////////////////////////

void OptimizePrinter::begin(Optimize::Type type, bool maximize)
{
	lits_.clear();
	maximize_ = maximize;
}

void OptimizePrinter::print(const ValVec &set)
{
	set_ = set;
}

void OptimizePrinter::print(PredLitRep *l)
{
	if(l->sign()) { lits_.push_back(-output()->symbol(l)); }
	else          { lits_.push_back( output()->symbol(l)); }
}

void OptimizePrinter::end()
{
	int32_t lit;
	if(lits_.empty())          { lit = -output()->falseSymbol(); }
	else if(lits_.size() == 1) { lit = lits_.back(); }
	else
	{
		lit = output()->symbol();
		output_->printBasicRule(lit, lits_);
	}
	output_->prioLit(lit, set_, maximize_);
}

///////////////////////////////// ComputePrinter /////////////////////////////////

void ComputePrinter::print(PredLitRep *l)
{
	output_->addCompute(l);
}

///////////////////////////////// JunctionLitPrinter /////////////////////////////////

JunctionLitPrinter::JunctionLitPrinter(LparseConverter *output)
	: output_(output)
	, current_(0)
	, disjunction_(false)
{
	output->regDelayedPrinter(this);
}

void JunctionLitPrinter::beginHead(bool disjunction, uint32_t juncUid, uint32_t substUid)
{
	CondMap::iterator it = condMap_.find(CondKey(juncUid, substUid));
	if (it == condMap_.end())
	{
		it = condMap_.insert(CondMap::value_type(CondKey(juncUid, substUid), CondValue(output_->symbol(), LparseConverter::LitVec()))).first;
	}
	current_ = &it->second.second;
	disjunction_ = disjunction;
	body_.clear();
	head_.clear();
}

void JunctionLitPrinter::beginBody()
{
	std::swap(body_, head_);
}

void JunctionLitPrinter::printCond(bool bodyComplete)
{
	if (disjunction_)
	{
		// Note: should not happen but could be handled too
		// assert(head > 0);

	}
	else
	{
		foreach (int32_t head, head_)
		{
			if (body_.empty()) { current_->push_back(head); }
			else
			{
				int32_t sym = output()->symbol();
				int32_t neg = output()->symbol();
				output()->printBasicRule(sym, 1, head);
				output()->printBasicRule(neg, body_);
				output()->printBasicRule(sym, 1, -neg);
				if (!bodyComplete)
				{
					uint32_t negHead = output()->symbol();
					output()->printBasicRule(negHead, 1, -neg);
					LparseConverter::AtomVec h, p, n;
					h.push_back(sym);
					h.push_back(neg);
					n.push_back(negHead);
					//sym | -head | neg
					output()->printDisjunctiveRule(h, p, n);
				}
				current_->push_back(sym);
			}
		}
	}
}

void JunctionLitPrinter::printJunc(bool disjunction, uint32_t juncUid, uint32_t substUid)
{
	CondMap::iterator it = condMap_.find(CondKey(juncUid, substUid));
	if (it == condMap_.end())
	{
		it = condMap_.insert(CondMap::value_type(CondKey(juncUid, substUid), CondValue(output_->symbol(), LparseConverter::LitVec()))).first;
	}
	RulePrinter *printer = static_cast<RulePrinter*>(output()->printer<Rule::Printer>());
	if (disjunction) { printer->setHead(it->second.first); }
	else { printer->addBody(it->second.first, false); }
}

void JunctionLitPrinter::print(PredLitRep *l)
{
	body_.push_back(output_->symbol(l));
}

void JunctionLitPrinter::finish()
{
	foreach (CondMap::value_type &value, condMap_)
	{
		output_->printBasicRule(value.second.first, value.second.second);
	}
	condMap_.clear();
}

///////////////////////////////// IncPrinter /////////////////////////////////

void IncPrinter::print(PredLitRep *) { }

void IncPrinter::print(uint32_t vol_window)
{
	RulePrinter *printer = static_cast<RulePrinter *>(output_->printer<Rule::Printer>());
	int atom = output_->getIncAtom(vol_window);
	if(atom > 0) { printer->addBody(atom, false); }
}

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::DisplayPrinter, Display::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::OptimizePrinter, Optimize::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::ComputePrinter, Compute::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::ExternalPrinter, External::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::JunctionLitPrinter, JunctionLit::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::IncPrinter, IncLit::Printer, LparseConverter)
