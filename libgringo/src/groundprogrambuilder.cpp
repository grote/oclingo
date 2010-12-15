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

#include <gringo/groundprogrambuilder.h>
#include <gringo/output.h>
#include <gringo/storage.h>
#include <gringo/func.h>
#include <gringo/optimize.h>
#include <gringo/compute.h>
#include <gringo/rule.h>
#include <gringo/sumaggrlit.h>
#include <gringo/avgaggrlit.h>
#include <gringo/minmaxaggrlit.h>
#include <gringo/parityaggrlit.h>
#include <gringo/junctionaggrlit.h>
#include <gringo/external.h>
#include <gringo/display.h>

GroundProgramBuilder::GroundProgramBuilder(Output *output)
	: output_(output)
	, stm_(new Statement())
{
}

GroundProgramBuilder::StatementPtr GroundProgramBuilder::getCurrent()
{
	StatementPtr ret = stm_;
	stm_.reset(new Statement());
	return ret;
}

void GroundProgramBuilder::setCurrent(StatementPtr stm)
{
	stm_ = stm;
}

void GroundProgramBuilder::add(Type type, uint32_t n)
{
	stm_->n    = n;
	stm_->type = type;
	add();
}

void GroundProgramBuilder::add()
{
	switch(stm_->type)
	{
		case LIT:
		{
			stm_->lits.push_back(Lit::create(stm_->type, stm_->vals.size() - stm_->n - 1, stm_->n));
			break;
		}
		case TERM:
		{
			if(stm_->n > 0)
			{
				ValVec vals;
				std::copy(stm_->vals.end() - stm_->n, stm_->vals.end(), std::back_inserter(vals));
				stm_->vals.resize(stm_->vals.size() - stm_->n);
				uint32_t name = stm_->vals.back().index;
				stm_->vals.back()  = Val::create(Val::FUNC, storage()->index(Func(storage(), name, vals)));
			}
			break;
		}
		case AGGR_SUM:
		case AGGR_COUNT:
		case AGGR_AVG:
		case AGGR_MIN:
		case AGGR_MAX:
		case AGGR_EVEN:
		case AGGR_EVEN_SET:
		case AGGR_ODD:
		case AGGR_ODD_SET:
		case AGGR_DISJUNCTION:
		{
			assert(stm_->type != AGGR_DISJUNCTION || stm_->n > 0);
			std::copy(stm_->lits.end() - stm_->n, stm_->lits.end(), std::back_inserter(stm_->aggrLits));
			stm_->lits.resize(stm_->lits.size() - stm_->n);
			stm_->lits.push_back(Lit::create(stm_->type, stm_->n ? stm_->aggrLits.size() - stm_->n : stm_->vals.size() - 2, stm_->n));
			break;
		}
		case STM_RULE:
		case STM_CONSTRAINT:
		{
			Rule::Printer *printer = output_->printer<Rule::Printer>();
			printer->begin();
			if(stm_->type == STM_RULE)             { printLit(printer, stm_->lits.size() - stm_->n - 1, true); }
			printer->endHead();
			for(uint32_t i = stm_->n; i >= 1; i--) { printLit(printer, stm_->lits.size() - i, false); }
			printer->end();
			pop(stm_->n + (stm_->type == STM_RULE));
			break;
		}
		case STM_SHOW:
		case STM_HIDE:
		{
			Display::Printer *printer = output_->printer<Display::Printer>();
			printLit(printer, stm_->lits.size() - 1, stm_->type == STM_SHOW);
			pop(1);
			break;
		}
		case STM_EXTERNAL:
		{
			External::Printer *printer = output_->printer<External::Printer>();
			printLit(printer, stm_->lits.size() - 1, true);
			pop(1);
			break;
		}
		case STM_MINIMIZE:
		case STM_MAXIMIZE:
		case STM_MINIMIZE_SET:
		case STM_MAXIMIZE_SET:
		{
			Optimize::Printer *printer = output_->printer<Optimize::Printer>();
			bool maximize = (stm_->type == STM_MAXIMIZE || stm_->type == STM_MAXIMIZE_SET);
			bool set = (stm_->type == STM_MINIMIZE_SET || stm_->type == STM_MAXIMIZE_SET);
			printer->begin(maximize, set);
			for(uint32_t i = stm_->n; i >= 1; i--)
			{

				Lit &a     = stm_->lits[stm_->lits.size() - i];
				Val prio   = stm_->vals[a.offset + a.n + 2];
				Val weight = stm_->vals[a.offset + a.n + 1];
				printer->print(predLitRep(a), weight.num, prio.num);
			}
			printer->end();
			pop(stm_->n);
			break;
		}
		case STM_COMPUTE:
		{
			Compute::Printer *printer = output_->printer<Compute::Printer>();
			printer->begin();
			for(uint32_t i = stm_->n; i >= 1; i--)
			{
				Lit &a = stm_->lits[stm_->lits.size() - i];
				printer->print(predLitRep(a));
			}
			printer->end();
			pop(stm_->n);
			break;
		}
		case META_SHOW:
		case META_HIDE:
		case META_EXTERNAL:
		{
			Val num = stm_->vals.back();
			stm_->vals.pop_back();
			Val id  = stm_->vals.back();
			stm_->vals.pop_back();
			assert(id.type == Val::ID);
			assert(num.type == Val::NUM);
			storage()->domain(id.index, num.num);
			if(stm_->type == META_EXTERNAL) { output_->external(id.index, num.num); }
			else { output_->show(id.index, num.num, stm_->type == META_SHOW); }
			break;
		}
		case META_GLOBALSHOW:
		case META_GLOBALHIDE:
		{
			output_->show(stm_->type == META_GLOBALSHOW);
			break;
		}
	}
}

void GroundProgramBuilder::pop(uint32_t n)
{
	if(n > 0)
	{
		Lit &a = *(stm_->lits.end() - n);
		if(a.type == LIT) { stm_->vals.resize(a.offset); }
		else
		{
			if(a.n > 0)
			{
				Lit &b = stm_->aggrLits[a.offset];
				stm_->aggrLits.resize(a.offset);
				stm_->vals.resize(b.offset - (a.type != AGGR_DISJUNCTION));
			}
			else { stm_->vals.resize(a.offset); }
		}
		stm_->lits.resize(stm_->lits.size() - n);
	}
}

void GroundProgramBuilder::printLit(Printer *printer, uint32_t offset, bool head)
{
	Lit &a = stm_->lits[offset];
	switch(a.type)
	{
		case AGGR_SUM:
		case AGGR_COUNT:
		{
			SumAggrLit::Printer *aggrPrinter = output_->printer<SumAggrLit::Printer>();
			aggrPrinter->begin(head, a.sign, a.type == AGGR_COUNT);
			Val lower;
			Val upper;
			if(a.n == 0)
			{
				lower = stm_->vals[a.offset];
				upper = stm_->vals[a.offset + 1];
			}
			else
			{
				Lit &first = stm_->aggrLits[a.offset];
				Lit &last  = stm_->aggrLits[a.offset + a.n - 1];
				lower = stm_->vals[first.offset - 1];
				upper = stm_->vals[last.offset + last.n + 1 + (a.type == AGGR_SUM)];
			}
			if(lower.type != Val::UNDEF) { assert(lower.type == Val::NUM); aggrPrinter->lower(lower.num); }
			if(upper.type != Val::UNDEF) { assert(upper.type == Val::NUM); aggrPrinter->upper(upper.num); }
			printAggrLits(aggrPrinter, a, a.type == AGGR_SUM);
			aggrPrinter->end();
			break;
		}
		case AGGR_AVG:
		{
			AvgAggrLit::Printer *aggrPrinter = output_->printer<AvgAggrLit::Printer>();
			aggrPrinter->begin(head, a.sign);
			Val lower;
			Val upper;
			if(a.n == 0)
			{
				lower = stm_->vals[a.offset];
				upper = stm_->vals[a.offset + 1];
			}
			else
			{
				Lit &first = stm_->aggrLits[a.offset];
				Lit &last  = stm_->aggrLits[a.offset + a.n - 1];
				lower = stm_->vals[first.offset - 1];
				upper = stm_->vals[last.offset + last.n + 2];
			}
			if(lower.type != Val::UNDEF) { assert(lower.type == Val::NUM); aggrPrinter->lower(lower.num); }
			if(upper.type != Val::UNDEF) { assert(upper.type == Val::NUM); aggrPrinter->upper(upper.num); }
			printAggrLits(aggrPrinter, a, true);
			aggrPrinter->end();
			break;
		}
		case AGGR_MIN:
		case AGGR_MAX:
		{
			MinMaxAggrLit::Printer *aggrPrinter = output_->printer<MinMaxAggrLit::Printer>();
			aggrPrinter->begin(head, a.sign, a.type == AGGR_MAX);
			Val lower;
			Val upper;
			if(a.n == 0)
			{
				lower = stm_->vals[a.offset];
				upper = stm_->vals[a.offset + 1];
			}
			else
			{
				Lit &first = stm_->aggrLits[a.offset];
				Lit &last  = stm_->aggrLits[a.offset + a.n - 1];
				lower = stm_->vals[first.offset - 1];
				upper = stm_->vals[last.offset + last.n + 2];
			}
			if(lower.type != Val::UNDEF) { aggrPrinter->lower(lower); }
			if(upper.type != Val::UNDEF) { aggrPrinter->upper(upper); }
			printAggrLits(aggrPrinter, a, true);
			aggrPrinter->end();
			break;
		}
		case AGGR_EVEN:
		case AGGR_EVEN_SET:
		case AGGR_ODD:
		case AGGR_ODD_SET:
		{
			ParityAggrLit::Printer *aggrPrinter = output_->printer<ParityAggrLit::Printer>();
			bool even = (a.type == AGGR_EVEN || a.type == AGGR_EVEN_SET);
			bool set = (a.type == AGGR_EVEN_SET || a.type == AGGR_ODD_SET);
			aggrPrinter->begin(head, a.sign, even, set);
			printAggrLits(aggrPrinter, a, !set);
			aggrPrinter->end();
			break;
		}
		case AGGR_DISJUNCTION:
		{
			JunctionAggrLit::Printer *aggrPrinter = output_->printer<JunctionAggrLit::Printer>();
			aggrPrinter->begin(head);
			printAggrLits(aggrPrinter, a, false);
			aggrPrinter->end();
			break;
		}
		default:
		{
			assert(a.type == LIT);
			printer->print(predLitRep(a));
			break;
		}
	}
}

void GroundProgramBuilder::printAggrLits(AggrLit::Printer *printer, Lit &a, bool weight)
{
	if(a.n > 0)
	{
		foreach(Lit &b, boost::iterator_range<LitVec::iterator>(stm_->aggrLits.begin() + a.offset, stm_->aggrLits.begin() + a.offset + a.n))
		{
			printer->print(predLitRep(b));
			if(weight) { printer->weight(stm_->vals[b.offset + b.n + 1]); }
			else       { printer->weight(Val::create(Val::NUM, 1)); }
		}
	}
}

PredLitRep *GroundProgramBuilder::predLitRep(Lit &a)
{
	Domain *dom = storage()->domain(stm_->vals[a.offset].index, a.n);
	lit_.dom_   = dom;
	lit_.sign_  = a.sign;
	lit_.vals_.resize(a.n);
	std::copy(stm_->vals.begin() + a.offset + 1, stm_->vals.begin() + a.offset + 1 + a.n, lit_.vals_.begin());
	return &lit_;
}

void GroundProgramBuilder::addVal(const Val &val)
{
	stm_->vals.push_back(val);
}

void GroundProgramBuilder::addSign()
{
	stm_->lits.back().sign = true;
}

Storage *GroundProgramBuilder::storage()
{
	return output_->storage();
}
