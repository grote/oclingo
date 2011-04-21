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
	void DisplayPrinter::print(PredLitRep *l)
	{
		out() << (show() ? "#show " : "#hide ");
		output_->print(l, out());
		out() << ".\n";
	}

	void ExternalPrinter::print(PredLitRep *l)
	{
		out() << "#external ";
		output_->print(l, out());
		out() << ".\n";
	}

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

	void AggrCondPrinter::begin(State state, const ValVec &set)
	{
		currentState_ = &stateMap_[state];
		currentState_->push_back(CondVec::value_type(set, ""));
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
		if(!currentState_->back().second.empty()) { currentState_->back().second += ":"; }
		currentState_->back().second += "#true";
	}

	void AggrCondPrinter::print(PredLitRep *l)
	{
		std::ostringstream os;
		output_->print(l, os);
		if(!currentState_->back().second.empty()) { currentState_->back().second += ":"; }
		currentState_->back().second += os.str();
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
	void AggrLitPrinter<T, Type>::begin(State state, bool head, bool sign, bool complete)
	{
		output_->print();
		_begin(state, head, sign, complete);
	}

	template <class T, uint32_t Type>
	void AggrLitPrinter<T, Type>::_begin(State state, bool head, bool sign, bool complete)
	{
		lower_    = Val::inf();
		upper_    = Val::sup();
		lleq_     = true;
		uleq_     = true;
		sign_     = sign;
		complete_ = complete;
		state_    = state;
		head_     = head;
	}

	template <class T, uint32_t Type>
	void AggrLitPrinter<T, Type>::lower(const Val &l, bool leq)
	{
		lower_ = l;
		lleq_  = leq;
	}

	template <class T, uint32_t Type>
	void AggrLitPrinter<T, Type>::upper(const Val &u, bool leq)
	{
		upper_ = u;
		uleq_  = leq;
	}

	template <class T, uint32_t Type>
	void AggrLitPrinter<T, Type>::end()
	{
		if(complete_)
		{
			// simplification
			AggrCondPrinter::CondVec &conds = static_cast<AggrCondPrinter*>(output()-> template printer<AggrCond::Printer>())->state(state_);
			typedef boost::unordered_map<ValVec, uint32_t> SetMap;
			enum { SINGLE=0, MULTI=1, REMOVED=2 };
			SetMap map;
			foreach(AggrCondPrinter::CondVec::value_type &cond, conds)
			{
				std::pair<SetMap::iterator, bool> res = map.insert(SetMap::value_type(cond.first, SINGLE));
				if(!res.second)                                                                          { res.first->second = MULTI; }
				else if((cond.second.empty() || cond.second == "#true") && simplify(cond.first.front())) { res.first->second = REMOVED; }
			}
			// printing
			if(sign_) { out() << "not "; }
			if(lower_.type != Val::INF)
			{
				lower_.print(output_->storage(), out());
				if(!lleq_) { out() << "<"; }
			}
			func();
			out() << "{";
			uint32_t uid = REMOVED + 1;
			bool printed = false;
			foreach(AggrCondPrinter::CondVec::value_type &cond, conds)
			{
				SetMap::iterator it = map.find(cond.first);
				if(it->second != REMOVED)
				{
					const Val &weight = cond.first.front();
					if(it->second <= REMOVED) { it->second = uid++; }
					if(printed) { out() << ","; }
					else        { printed = true; }
					out() << "<";
					weight.print(output()->storage(), out());
					out() << "," << (it->second - REMOVED) << ">:" << cond.second;
					if(cond.second.empty()) { out() << "#true"; }
				}
			}
			out() << "}";
			if(upper_.type != Val::SUP)
			{
				if(!uleq_) { out() << "<"; }
				upper_.print(output_->storage(), out());
			}
		}
		else
		{
			uint32_t todo = output_->delay();
			todo_.insert(TodoMap::value_type(todo, TodoVal(state_, head_, sign_, lleq_, uleq_, lower_, upper_)));
		}
	}

	template <class T, uint32_t Type>
	void AggrLitPrinter<T, Type>::finish()
	{
		foreach(TodoMap::value_type &val, todo_)
		{
			output_->startDelayed(val.first);
			_begin(val.second.get<0>(), val.second.get<1>(), val.second.get<2>(), true);
			lower(val.second.get<5>(), val.second.get<3>());
			upper(val.second.get<6>(), val.second.get<4>());
			end();
			output_->endDelayed(val.first);
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
		if(lower_.type == Val::NUM) { lower_.num -= weight.num; }
		if(upper_.type == Val::NUM) { upper_.num -= weight.num; }
		return true;
	}

	void SumAggrLitPrinter::func()
	{
		out() << "#sum";
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

	/*

	void AvgAggrLitPrinter::begin(bool head, bool sign)
	{
		(void)head;
		output_->print();
		aggr_.str("");
		sign_       = sign;
		hasUpper_   = false;
		hasLower_   = false;
		printedLit_ = false;
	}

	void AvgAggrLitPrinter::weight(const Val &v)
	{
		aggr_ << "=" << v.number();
	}

	void AvgAggrLitPrinter::lower(int32_t l)
	{
		hasLower_ = true;
		lower_    = l;
	}

	void AvgAggrLitPrinter::upper(int32_t u)
	{
		hasUpper_ = true;
		upper_    = u;
	}

	void AvgAggrLitPrinter::print(PredLitRep *l)
	{
		if(printedLit_) aggr_ << ",";
		else printedLit_ = true;
		output_->print(l, aggr_);
	}

	void AvgAggrLitPrinter::end()
	{
		if(sign_) out() << "not ";
		if(hasLower_) out() << lower_;
		out() << "#avg[" << aggr_.str() << "]";
		if(hasUpper_) out() << upper_;
	}

	void ParityAggrLitPrinter::begin(bool head, bool sign, bool even, bool set)
	{
		(void)head;
		output_->print();
		aggr_.str("");
		sign_       = sign;
		even_       = even;
		set_        = set;
		printedLit_ = false;
	}

	void ParityAggrLitPrinter::print(PredLitRep *l)
	{
		if(printedLit_) aggr_ << ",";
		else printedLit_ = true;
		output_->print(l, aggr_);
	}

	void ParityAggrLitPrinter::weight(const Val &v)
	{
		if(set_) return;
		if(v.number() % 2 == 0) aggr_ << "=0";
		else aggr_ << "=1";
	}

	void ParityAggrLitPrinter::end()
	{
		if(sign_) out() << "not ";
		if(even_) out() << "#even";
		else out() << "#odd";
		if(set_) out() << "{" << aggr_.str() << "}";
		else out() << "[" << aggr_.str() << "]";
	}
	*/

	void JunctionLitPrinter::begin(bool)
	{
	}

	void JunctionLitPrinter::print(PredLitRep *l)
	{
		output_->print();
		output_->print(l, output_->out());
	}
	
	void OptimizePrinter::begin(bool maximize, bool set)
	{
		set_   = set;
		comma_ = false;
		out() << (maximize ? "#maximize" : "#minimize");
		out() << (set_ ? "{" : "[");
	}

	void OptimizePrinter::print(PredLitRep *l, int32_t weight, int32_t prio)
	{
		if(comma_) { out() << ","; }
		else       { comma_ = true; }
		output_->print(l, out());
		if(!set_) { out() << "=" << weight; }
		out() << "@" << prio;
	}

	void OptimizePrinter::end()
	{
		out() << (set_ ? "}.\n" : "].\n");
		output_->endStatement();
	}

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
