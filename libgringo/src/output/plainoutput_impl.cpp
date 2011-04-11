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

	SumAggrLitPrinter::SumAggrLitPrinter(PlainOutput *output)
		: output_(output)
	{
		output->regDelayedPrinter(this);
	}

	void SumAggrLitPrinter::begin(State state, bool head, bool sign, bool complete)
	{
		output_->print();
		_begin(state, head, sign, complete);
	}

	void SumAggrLitPrinter::_begin(State state, bool head, bool sign, bool complete)
	{
		lower_    = std::numeric_limits<int32_t>::min();
		upper_    = std::numeric_limits<int32_t>::max();
		sign_     = sign;
		complete_ = complete;
		state_    = state;
		head_     = head;
	}

	void SumAggrLitPrinter::lower(int32_t l, bool leq) { lower_ = leq ? l : l + 1; }

	void SumAggrLitPrinter::upper(int32_t u, bool leq) { upper_ = leq ? u : u - 1; }

	void SumAggrLitPrinter::end()
	{
		// Note: the latter part can be reused for other aggregates!
		if(complete_)
		{
			if(sign_) { out() << "not "; }
			AggrCondPrinter::CondVec &conds = static_cast<AggrCondPrinter*>(output()->printer<AggrCond::Printer>())->state(state_);
			typedef boost::unordered_map<ValVec, uint32_t> SetMap;
			SetMap map;
			foreach(AggrCondPrinter::CondVec::value_type &cond, conds)
			{
				std::pair<SetMap::iterator, bool> res = map.insert(SetMap::value_type(cond.first, 0));
				if(!res.second) { res.first->second = 1; }
				else if(cond.second.empty() || cond.second == "#true")
				{
					int32_t weight = cond.first.front().num;
					if(lower_ != std::numeric_limits<int32_t>::min()) { lower_ -= weight; }
					if(upper_ != std::numeric_limits<int32_t>::max()) { upper_ -= weight; }
				}
			}
			if(lower_ != std::numeric_limits<int32_t>::min()) { out() << lower_; }
			out() << "[";
			uint32_t uid = 2;
			bool printed = false;
			foreach(AggrCondPrinter::CondVec::value_type &cond, conds)
			{
				int32_t weight = cond.first.front().num;
				if(!cond.second.empty() && cond.second != "#true" && weight != 0)
				{
					SetMap::iterator it = map.find(cond.first);
					if(it->second == 1) { it->second = uid++; }
					if(printed) { out() << ","; }
					if(it->second > 0)
					{
						out() << "<" << weight << "," << (it->second - 2) << ">:";
					}
					out() << cond.second;
					if(it->second == 0 && weight != 1) { out() << "=" << weight; }
					printed = true;
				}
			}
			out() << "]";
			if(upper_ != std::numeric_limits<int32_t>::max()) { out() << upper_; }
		}
		else
		{
			uint32_t todo = output_->delay();
			todo_.insert(TodoMap::value_type(todo, TodoVal(state_, head_, sign_, lower_, upper_)));
		}
	}

	void SumAggrLitPrinter::finish()
	{
		foreach(TodoMap::value_type &val, todo_)
		{
			output_->startDelayed(val.first);
			_begin(val.second.get<0>(), val.second.get<1>(), val.second.get<2>(), true);
			lower(val.second.get<3>(), true);
			upper(val.second.get<4>(), true);
			end();
			output_->endDelayed(val.first);
		}
		todo_.clear();
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

	void MinMaxAggrLitPrinter::begin(bool head, bool sign, bool max)
	{
		(void)head;
		output_->print();
		aggr_.str("");
		sign_       = sign;
		max_        = max;
		hasUpper_   = false;
		hasLower_   = false;
		printedLit_ = false;
	}

	void MinMaxAggrLitPrinter::weight(const Val &v)
	{
		aggr_ << "=";
		v.print(output_->storage(), aggr_);
	}

	void MinMaxAggrLitPrinter::lower(const Val &l)
	{
		hasLower_ = true;
		lower_    = l;
	}

	void MinMaxAggrLitPrinter::upper(const Val &u)
	{
		hasUpper_ = true;
		upper_    = u;
	}

	void MinMaxAggrLitPrinter::print(PredLitRep *l)
	{
		if(printedLit_) aggr_ << ",";
		else printedLit_ = true;
		output_->print(l, aggr_);
	}

	void MinMaxAggrLitPrinter::end()
	{
		if(sign_) out() << "not ";
		if(hasLower_) lower_.print(output_->storage(), out());
		if(max_) out() << "#max[" << aggr_.str() << "]";
		else out() << "#min[" << aggr_.str() << "]";
		if(hasUpper_) upper_.print(output_->storage(), out());
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
GRINGO_REGISTER_PRINTER(plainoutput_impl::SumAggrLitPrinter, SumAggrLit::Printer, PlainOutput)
/*
GRINGO_REGISTER_PRINTER(plainoutput_impl::AvgAggrLitPrinter, AvgAggrLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::MinMaxAggrLitPrinter, MinMaxAggrLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::ParityAggrLitPrinter, ParityAggrLit::Printer, PlainOutput)
*/
GRINGO_REGISTER_PRINTER(plainoutput_impl::JunctionLitPrinter, JunctionLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::OptimizePrinter, Optimize::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::IncPrinter, IncLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::ComputePrinter, Compute::Printer, PlainOutput)
