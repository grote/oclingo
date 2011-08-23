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

#include "output/plainoutput_impl.h"
#include <gringo/plainoutput.h>
#include <gringo/domain.h>
#include <gringo/predlit.h>

namespace plainoutput_impl
{

//////////////////////////////////////// DisplayPrinter ////////////////////////////////////////

void DisplayPrinter::begin(const Val &head, Type type)
{
	out() << (type == Display::HIDEPRED ? "#hide " : "#show ");
	head.print(output()->storage(), out());
	ignore_  = type != Display::SHOWTERM;
	ignored_ = false;
}

void DisplayPrinter::print(PredLitRep *l)
{
	if(!ignore_ || ignored_)
	{
		out() << ":";
		output_->print(l, out());
	}
	else { ignored_ = true; }
}

void DisplayPrinter::end()
{
	out() << (!ignore_ ? ":" : "") << ".\n";
	output()->endStatement();
}

//////////////////////////////////////// ExternalPrinter ////////////////////////////////////////

void ExternalPrinter::print(PredLitRep *l)
{
	if (printed_) { out() << ":"; }
	else          { printed_ = true; }
	output_->print(l, out());
}

void ExternalPrinter::begin()
{
	out() << "#external ";
	printed_ = false;
}

void ExternalPrinter::endHead()
{
}

void ExternalPrinter::end()
{
	out() << ".\n";
	output_->endStatement();
}

//////////////////////////////////////// RulePrinter ////////////////////////////////////////

void RulePrinter::begin()
{
	output_->beginRule();
}

void RulePrinter::endHead()
{
	output_->endHead();
}

void RulePrinter::print(PredLitRep *l)
{
	output_->print();
	output_->print(l, out());
}

void RulePrinter::end()
{
	output_->endRule();
}

void AggrCondPrinter::begin(AggrCond::Style style, State state, const ValVec &set)
{
	currentState_ = &stateMap_[state];
	currentState_->push_back(CondVec::value_type(set, style, ""));
}

AggrCondPrinter::CondVec &AggrCondPrinter::state(State state)
{
	return stateMap_[state];
}

void AggrCondPrinter::endHead()
{
}

void AggrCondPrinter::trueLit()
{
	if(!currentState_->back().get<2>().empty()) { currentState_->back().get<2>() += ":"; }
	currentState_->back().get<2>() += "#true";
}

void AggrCondPrinter::print(PredLitRep *l)
{
	std::ostringstream os;
	output_->print(l, os);
	if(!currentState_->back().get<2>().empty()) { currentState_->back().get<2>() += ":"; }
	currentState_->back().get<2>() += os.str();
}

void AggrCondPrinter::end()
{
}

//////////////////////////////////////// AggrLitPrinter ////////////////////////////////////////

template <class T, uint32_t Type>
AggrLitPrinter<T, Type>::AggrLitPrinter(PlainOutput *output)
	: output_(output)
{
	output->regDelayedPrinter(this);
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::begin(State state, bool head, bool sign, bool complete, bool set)
{
	output_->print();
	current_.get<0>() = state;
	current_.get<1>() = head;
	current_.get<2>() = sign;
	current_.get<3>() = true;
	current_.get<4>() = true;
	current_.get<5>() = Val::inf();
	current_.get<6>() = Val::sup();
	current_.get<7>() = set;
	complete_         = complete;
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::lower(const Val &l, bool leq)
{
	current_.get<3>() = leq;
	current_.get<5>() = l;
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::upper(const Val &u, bool leq)
{
	current_.get<4>() = leq;
	current_.get<6>() = u;
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::end()
{
	if(complete_)
	{
		// simplification
		AggrCondPrinter::CondVec &conds = static_cast<AggrCondPrinter*>(output()-> template printer<AggrCond::Printer>())->state(current_.get<0>());
		typedef boost::unordered_map<ValVec, uint32_t> SetMap;
		enum { SINGLE=0, MULTI=1, REMOVED=2 };
		SetMap map;
		foreach(AggrCondPrinter::CondVec::value_type &cond, conds)
		{
			std::pair<SetMap::iterator, bool> res = map.insert(SetMap::value_type(cond.get<0>(), SINGLE));
			if(!res.second)                                                                                 { res.first->second = MULTI; }
			else if((cond.get<2>().empty() || cond.get<2>() == "#true") && simplify(cond.get<0>().front())) { res.first->second = REMOVED; }
		}
		// printing
		if(current_.get<2>()) { out() << "not "; }
		if(current_.get<5>().type != Val::INF)
		{
			current_.get<5>().print(output_->storage(), out());
			if(!current_.get<3>()) { out() << "<"; }
		}
		func();
		out() << (current_.get<7>() ? "{" : "[");
		bool printed = false;
		foreach(AggrCondPrinter::CondVec::value_type &cond, conds)
		{
			SetMap::iterator it = map.find(cond.get<0>());
			if(it->second != REMOVED)
			{
				if(printed) { out() << ","; }
				else        { printed = true; }
				if(cond.get<1>() == AggrCond::STYLE_DLV)
				{
					out() << "<";
					bool comma = false;
					foreach(const Val &val, cond.get<0>())
					{
						if(comma) { out() << ","; }
						else      { comma = true; }
						val.print(output()->storage(), out());
					}
					out() << ">:" << cond.get<2>();
					if(cond.get<2>().empty()) { out() << "#true"; }
				}
				else
				{
					out() << cond.get<2>();
					if(cond.get<2>().empty()) { out() << "#true"; }
					if(!current_.get<7>())
					{
						const Val &weight = cond.get<0>().front();
						out() << "=";
						weight.print(output()->storage(), out());
					}
				}
			}
		}
		out() << (current_.get<7>() ? "}" : "]");
		if(current_.get<6>().type != Val::SUP)
		{
			if(!current_.get<4>()) { out() << "<"; }
			current_.get<6>().print(output_->storage(), out());
		}
	}
	else
	{
		DelayedOutput::Offset todo = output_->beginDelay();
		todo_.insert(TodoMap::value_type(todo, current_));
	}
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::finish()
{
	foreach(TodoMap::value_type &val, todo_)
	{
		current_  = val.second;
		complete_ = true;
		end();
		output_->contDelay(val.first);
		output_->endDelay(val.first);
	}
	todo_.clear();
}

template <class T, uint32_t Type>
AggrLitPrinter<T, Type>::~AggrLitPrinter()
{
}

//////////////////////////////////////// SumAggrLitPrinter ////////////////////////////////////////

SumAggrLitPrinter::SumAggrLitPrinter(PlainOutput *output)
	: AggrLitPrinter<SumAggrLit>(output)
{
}

bool SumAggrLitPrinter::simplify(const Val &weight)
{
	assert(weight.type == Val::NUM);
	if(current_.get<5>().type == Val::NUM) { current_.get<5>().num -= weight.num; }
	if(current_.get<6>().type == Val::NUM) { current_.get<6>().num -= weight.num; }
	return true;
}

void SumAggrLitPrinter::func()
{
	AggrCond::Style           style = AggrCond::STYLE_LPARSE;
	AggrCondPrinter::CondVec &conds = static_cast<AggrCondPrinter*>(output()->printer<AggrCond::Printer>())->state(current_.get<0>());
	foreach(AggrCondPrinter::CondVec::value_type &cond, conds)
	{
		if(cond.get<1>() == AggrCond::STYLE_DLV)
		{
			style = AggrCond::STYLE_DLV;
			break;
		}
	}
	if(style == AggrCond::STYLE_DLV) { out() << "#sum"; }
}

//////////////////////////////////////// MinMaxAggrLitPrinter ////////////////////////////////////////
template <uint32_t Type>
MinMaxAggrLitPrinter<Type>::MinMaxAggrLitPrinter(PlainOutput *output)
	: AggrLitPrinter<MinMaxAggrLit, Type>(output)
{
}

template <uint32_t Type>
bool MinMaxAggrLitPrinter<Type>::simplify(const Val &)
{
	return false;
}

template <uint32_t Type>
void MinMaxAggrLitPrinter<Type>::func()
{
	this->template out() << (Type == MinMaxAggrLit::MAXIMUM ? "#max" : "#min");
}

//////////////////////////////////////// JunctionLitPrinter ////////////////////////////////////////

JunctionLitPrinter::JunctionLitPrinter(PlainOutput *output)
	: output_(output)
	, current_(0)
	, body_(false)
{
	output->regDelayedPrinter(this);
}

void JunctionLitPrinter::beginHead(bool disjunction, uint32_t uidJunc, uint32_t uidSubst)
{
	body_    = false;
	current_ = &condMap_.insert(CondMap::value_type(CondKey(uidJunc, uidSubst), "")).first->second;
	if (!current_->empty()) { *current_+= disjunction ? "|" : ","; }
}

void JunctionLitPrinter::beginBody()
{
	body_ = true;
}

void JunctionLitPrinter::printCond(bool)
{
}

void JunctionLitPrinter::printJunc(bool, uint32_t uidJunc, uint32_t uidSubst)
{
	output()->print();
	DelayedOutput::Offset todo = output_->beginDelay();
	todo_.push_back(TodoValue(todo, CondKey(uidJunc, uidSubst)));
}

void JunctionLitPrinter::print(PredLitRep *l)
{
	std::stringstream ss;
	if (body_) { ss << ":"; }
	output()->print(l, ss);
	*current_+= ss.str();
}

void JunctionLitPrinter::finish()
{
	foreach (TodoVec::value_type &value, todo_)
	{
		CondMap::iterator it = condMap_.find(value.second);
		out() << (it != condMap_.end() ? it->second : "#true");
		output_->contDelay(value.first);
		output_->endDelay(value.first);
	}
	todo_.clear();
	condMap_.clear();
}


//////////////////////////////////////// OptimizePrinter ////////////////////////////////////////

void OptimizePrinter::begin(Optimize::Type type, bool maximize)
{
	first_    = true;
	type_     = type;
	maximize_ = maximize;
}

void OptimizePrinter::print(const ValVec &set)
{
	weight_ = set[0];
	prio_   = set[1];
	if(type_ == Optimize::CONSTRAINT) { out() << ":~"; }
	else
	{
		if(type_ == Optimize::SYMSET)
		{
			out() << "<";
			set.front().print(output()->storage(), out());
			for(ValVec::const_iterator it = set.begin() + 3; it != set.end(); it++)
			{
				out() << ":";
				it->print(output()->storage(), out());
			}
			out() << ">";
			first_ = false;
		}
		output()->setLineCallback(boost::bind(&PlainOutput::addOptimize, output(), set[2], maximize_, type_ == Optimize::MULTISET, _1));
	}
}

void OptimizePrinter::print(PredLitRep *l)
{
	if(!first_) { out() << (type_ == Optimize::CONSTRAINT ? "," : ":"); }
	else        { first_ = false; }
	output()->print(l, out());
}

void OptimizePrinter::end()
{
	if(type_ == Optimize::CONSTRAINT)
	{
		out() << ".<";
		weight_.print(output()->storage(), out());
		out() << ",";
		prio_.print(output()->storage(), out());
		out() << ">\n";
	}
	else
	{
		if(type_ == Optimize::MULTISET)
		{
			out() << "=";
			weight_.print(output()->storage(), out());
		}
		out() << "@";
		prio_.print(output()->storage(), out());
	}
	output_->endStatement();
}

//////////////////////////////////////// ComputePrinter ////////////////////////////////////////

void ComputePrinter::print(PredLitRep *l)
{
	output_->addCompute(l);
}

}

GRINGO_REGISTER_PRINTER(plainoutput_impl::DisplayPrinter, Display::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::ExternalPrinter, External::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::RulePrinter, Rule::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::AggrCondPrinter, AggrCond::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::SumAggrLitPrinter, AggrLit::Printer<SumAggrLit>, PlainOutput)
typedef AggrLit::Printer<MinMaxAggrLit, MinMaxAggrLit::MINIMUM> MinPrinterBase;
typedef AggrLit::Printer<MinMaxAggrLit, MinMaxAggrLit::MAXIMUM> MaxPrinterBase;
GRINGO_REGISTER_PRINTER(plainoutput_impl::MinAggrLitPrinter, MinPrinterBase, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::MaxAggrLitPrinter, MaxPrinterBase, PlainOutput)
/*
GRINGO_REGISTER_PRINTER(plainoutput_impl::AvgAggrLitPrinter, AvgAggrLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::MinMaxAggrLitPrinter, MinMaxAggrLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::ParityAggrLitPrinter, ParityAggrLit::Printer, PlainOutput)
*/
GRINGO_REGISTER_PRINTER(plainoutput_impl::JunctionLitPrinter, JunctionLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::OptimizePrinter, Optimize::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::IncPrinter, IncLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::ComputePrinter, Compute::Printer, PlainOutput)
