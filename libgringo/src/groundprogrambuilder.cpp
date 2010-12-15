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
	, stack_(new Stack())
{
}

void GroundProgramBuilder::add(StackPtr stm)
{
	stack_ = stm;
	add();
}

GroundProgramBuilder::StackPtr GroundProgramBuilder::get(Type type, uint32_t n)
{
	stack_->n    = n;
	stack_->type = type;
	StackPtr ret = stack_;
	stack_.reset(new Stack());
	return ret;
}

void GroundProgramBuilder::add(Type type, uint32_t n)
{
	stack_->n    = n;
	stack_->type = type;
	add();
}

void GroundProgramBuilder::add()
{
	switch(stack_->type)
	{
		case LIT:
		{
			stack_->lits.push_back(Lit::create(stack_->type, stack_->vals.size() - stack_->n - 1, stack_->n));
			break;
		}
		case TERM:
		{
			if(stack_->n > 0)
			{
				ValVec vals;
				std::copy(stack_->vals.end() - stack_->n, stack_->vals.end(), std::back_inserter(vals));
				stack_->vals.resize(stack_->vals.size() - stack_->n);
				uint32_t name = stack_->vals.back().index;
				stack_->vals.back()  = Val::create(Val::FUNC, storage()->index(Func(storage(), name, vals)));
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
			assert(stack_->type != AGGR_DISJUNCTION || stack_->n > 0);
			std::copy(stack_->lits.end() - stack_->n, stack_->lits.end(), std::back_inserter(stack_->aggrLits));
			stack_->lits.resize(stack_->lits.size() - stack_->n);
			stack_->lits.push_back(Lit::create(stack_->type, stack_->n ? stack_->aggrLits.size() - stack_->n : stack_->vals.size() - 2, stack_->n));
			break;
		}
		case STM_RULE:
		case STM_CONSTRAINT:
		{
			Rule::Printer *printer = output_->printer<Rule::Printer>();
			printer->begin();
			if(stack_->type == STM_RULE)             { printLit(printer, stack_->lits.size() - stack_->n - 1, true); }
			printer->endHead();
			for(uint32_t i = stack_->n; i >= 1; i--) { printLit(printer, stack_->lits.size() - i, false); }
			printer->end();
			pop(stack_->n + (stack_->type == STM_RULE));
			break;
		}
		case STM_SHOW:
		case STM_HIDE:
		{
			Display::Printer *printer = output_->printer<Display::Printer>();
			printLit(printer, stack_->lits.size() - 1, stack_->type == STM_SHOW);
			pop(1);
			break;
		}
		case STM_EXTERNAL:
		{
			External::Printer *printer = output_->printer<External::Printer>();
			printLit(printer, stack_->lits.size() - 1, true);
			pop(1);
			break;
		}
		case STM_MINIMIZE:
		case STM_MAXIMIZE:
		case STM_MINIMIZE_SET:
		case STM_MAXIMIZE_SET:
		{
			Optimize::Printer *printer = output_->printer<Optimize::Printer>();
			bool maximize = (stack_->type == STM_MAXIMIZE || stack_->type == STM_MAXIMIZE_SET);
			bool set = (stack_->type == STM_MINIMIZE_SET || stack_->type == STM_MAXIMIZE_SET);
			printer->begin(maximize, set);
			for(uint32_t i = stack_->n; i >= 1; i--)
			{

				Lit &a     = stack_->lits[stack_->lits.size() - i];
				Val prio   = stack_->vals[a.offset + a.n + 2];
				Val weight = stack_->vals[a.offset + a.n + 1];
				printer->print(predLitRep(a), weight.num, prio.num);
			}
			printer->end();
			pop(stack_->n);
			break;
		}
		case STM_COMPUTE:
		{
			Compute::Printer *printer = output_->printer<Compute::Printer>();
			printer->begin();
			for(uint32_t i = stack_->n; i >= 1; i--)
			{
				Lit &a = stack_->lits[stack_->lits.size() - i];
				printer->print(predLitRep(a));
			}
			printer->end();
			pop(stack_->n);
			break;
		}
		case META_SHOW:
		case META_HIDE:
		case META_EXTERNAL:
		{
			Val num = stack_->vals.back();
			stack_->vals.pop_back();
			Val id  = stack_->vals.back();
			stack_->vals.pop_back();
			assert(id.type == Val::ID);
			assert(num.type == Val::NUM);
			storage()->domain(id.index, num.num);
			if(stack_->type == META_EXTERNAL) { output_->external(id.index, num.num); }
			else { output_->show(id.index, num.num, stack_->type == META_SHOW); }
			break;
		}
		case META_GLOBALSHOW:
		case META_GLOBALHIDE:
		{
			output_->show(stack_->type == META_GLOBALSHOW);
			break;
		}
	}
}

void GroundProgramBuilder::pop(uint32_t n)
{
	if(n > 0)
	{
		Lit &a = *(stack_->lits.end() - n);
		if(a.type == LIT) { stack_->vals.resize(a.offset); }
		else
		{
			if(a.n > 0)
			{
				Lit &b = stack_->aggrLits[a.offset];
				stack_->aggrLits.resize(a.offset);
				stack_->vals.resize(b.offset - (a.type != AGGR_DISJUNCTION));
			}
			else { stack_->vals.resize(a.offset); }
		}
		stack_->lits.resize(stack_->lits.size() - n);
	}
}

void GroundProgramBuilder::printLit(Printer *printer, uint32_t offset, bool head)
{
	Lit &a = stack_->lits[offset];
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
				lower = stack_->vals[a.offset];
				upper = stack_->vals[a.offset + 1];
			}
			else
			{
				Lit &first = stack_->aggrLits[a.offset];
				Lit &last  = stack_->aggrLits[a.offset + a.n - 1];
				lower = stack_->vals[first.offset - 1];
				upper = stack_->vals[last.offset + last.n + 1 + (a.type == AGGR_SUM)];
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
				lower = stack_->vals[a.offset];
				upper = stack_->vals[a.offset + 1];
			}
			else
			{
				Lit &first = stack_->aggrLits[a.offset];
				Lit &last  = stack_->aggrLits[a.offset + a.n - 1];
				lower = stack_->vals[first.offset - 1];
				upper = stack_->vals[last.offset + last.n + 2];
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
				lower = stack_->vals[a.offset];
				upper = stack_->vals[a.offset + 1];
			}
			else
			{
				Lit &first = stack_->aggrLits[a.offset];
				Lit &last  = stack_->aggrLits[a.offset + a.n - 1];
				lower = stack_->vals[first.offset - 1];
				upper = stack_->vals[last.offset + last.n + 2];
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
		foreach(Lit &b, boost::iterator_range<LitVec::iterator>(stack_->aggrLits.begin() + a.offset, stack_->aggrLits.begin() + a.offset + a.n))
		{
			printer->print(predLitRep(b));
			if(weight) { printer->weight(stack_->vals[b.offset + b.n + 1]); }
			else       { printer->weight(Val::create(Val::NUM, 1)); }
		}
	}
}

PredLitRep *GroundProgramBuilder::predLitRep(Lit &a)
{
	Domain *dom = storage()->domain(stack_->vals[a.offset].index, a.n);
	lit_.dom_   = dom;
	lit_.sign_  = a.sign;
	lit_.vals_.resize(a.n);
	std::copy(stack_->vals.begin() + a.offset + 1, stack_->vals.begin() + a.offset + 1 + a.n, lit_.vals_.begin());
	return &lit_;
}

void GroundProgramBuilder::addVal(const Val &val)
{
	stack_->vals.push_back(val);
}

void GroundProgramBuilder::addSign()
{
	stack_->lits.back().sign = true;
}

Storage *GroundProgramBuilder::storage()
{
	return output_->storage();
}
