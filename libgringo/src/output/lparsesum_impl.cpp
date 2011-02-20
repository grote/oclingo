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

#include "output/lparsesum_impl.h"
#include "output/lparserule_impl.h"
#include <gringo/predlitrep.h>
#include <gringo/domain.h>
#include <gringo/storage.h>

namespace lparseconverter_impl
{

namespace
{
class SumAggr
{
	typedef LparseConverter::AtomVec AtomVec;
	typedef LparseConverter::WeightVec WeightVec;

public:

	SumAggr(bool sign, int32_t lower, int32_t upper);

	tribool simplify();
	void printHead(LparseConverter *output, uint32_t a);
	void printBody(LparseConverter *output, uint32_t a);
	bool hasUpper() const;
	bool hasLower() const;

	void push(uint32_t p, int32_t w);

private:
	AtomVec   pos_;
	AtomVec   neg_;
	WeightVec wPos_;
	WeightVec wNeg_;
	int32_t   lower_;
	int32_t   upper_;
	bool      sign_;
	bool      card_;
};

}

////////////////////////////////// CondLitPrinter //////////////////////////////////

void CondLitPrinter::begin(uint32_t state, const ValVec &set)
{
	isHead_  = true;
	CondMap::mapped_type &conds = stateMap_.insert(StateMap::value_type(state, StateMap::mapped_type())).first->second.insert(CondMap::value_type(set, CondMap::mapped_type())).first->second;
	conds.push_back(Cond());
	current_ = &conds.back();
}

CondLitPrinter::CondMap &CondLitPrinter::state(uint32_t state)
{
	return stateMap_.find(state)->second;
}

void CondLitPrinter::endHead()
{
	isHead_ = false;
}

void CondLitPrinter::print(PredLitRep *l)
{
	uint32_t sym = output_->symbol(l);
	if(isHead_)
	{
		assert(!l->sign());
		current_->head.push_back(sym);
	}
	else if(l->sign()) { current_->pos.push_back(sym); }
	else               { current_->neg.push_back(sym); }
}

////////////////////////////////// SumAggrLitPrinter //////////////////////////////////

SumAggrLitPrinter::SumAggrLitPrinter(LparseConverter *output)
	: output_(output)
{
	output->regDelayedPrinter(this);
}

void SumAggrLitPrinter::begin(uint32_t state, bool head, bool sign, bool)
{
	lower_ = std::numeric_limits<int32_t>::min();
	upper_ = std::numeric_limits<int32_t>::max();
	state_ = state;
	head_  = head;
	sign_  = sign;
}

void SumAggrLitPrinter::end()
{
	uint32_t a = output_->symbol();
	RulePrinter *printer = static_cast<RulePrinter*>(output_->printer<Rule::Printer>());
	if(head_)
	{
		assert(!sign_);
		printer->setHead(a);
	}
	else { printer->addBody(a, sign_); }

	todo_.insert(TodoMap::value_type(TodoKey(state_, TodoKey::second_type(lower_, upper_)), TodoVal(a, head_, sign_)));
}

void SumAggrLitPrinter::finish()
{

	foreach(TodoMap::value_type &todo, todo_)
	{
		uint32_t a = todo.second.get<0>();
		SumAggr  aggr(todo.second.get<2>(), todo.first.second.first, todo.first.second.second);

		CondLitPrinter::CondMap &conds = static_cast<CondLitPrinter*>(output()->printer<CondLit::Printer>())->state(todo.first.first);
		foreach(CondLitPrinter::CondMap::value_type &condTerm, conds)
		{
			std::sort(condTerm.second.begin(), condTerm.second.end());
			condTerm.second.erase(std::unique(condTerm.second.begin(), condTerm.second.end()), condTerm.second.end());
			uint32_t hc = output_->symbol();
			aggr.push(hc, condTerm.first[0].num);

			foreach(CondLitPrinter::Cond &cond, condTerm.second)
			{
				if(!cond.head.empty())
				{
					// L #aggr { H:C, ... } U :- a.
					// c :- C, a.
					uint32_t c = output_->symbol();
					cond.pos.push_back(a);
					output_->printBasicRule(c, cond.pos, cond.neg);
					foreach(uint32_t head, cond.head)
					{
						LparseConverter::AtomVec pos;
						// {H} :- c.
						pos.push_back(c);
						output_->printChoiceRule(LparseConverter::AtomVec(1, head), pos, LparseConverter::AtomVec());
						// hc :- H, c.
						pos.push_back(head);
						output_->printBasicRule(hc, pos, LparseConverter::AtomVec());
					}
				}
				else
				{
					// hc :- C.
					output_->printBasicRule(hc, cond.pos, cond.neg);
				}
			}
		}
		if(todo.second.get<1>()) { aggr.printHead(output_, a); }
		else                     { aggr.printBody(output_, a); }
	}
	todo_.clear();
}

////////////////////////////////// SumAggr //////////////////////////////////

SumAggr::SumAggr(bool sign, int32_t lower, int32_t upper)
	: lower_(lower)
	, upper_(upper)
	, sign_(sign)
	, card_(false)
{
}

void SumAggr::push(uint32_t p, int32_t w)
{
	pos_.push_back(p);
	wPos_.push_back(w);
}

bool SumAggr::hasUpper() const
{
	return upper_ != std::numeric_limits<int32_t>::max();
}

bool SumAggr::hasLower() const
{
	return lower_ != std::numeric_limits<int32_t>::min();
}

tribool SumAggr::simplify()
{
	int64_t sum = 0;
	size_t j = 0;
	card_ = true;
	for(size_t i = 0; i < pos_.size(); i++)
	{
		if(wPos_[i] < 0)
		{
			lower_-= wPos_[i];
			upper_-= wPos_[i];
			neg_.push_back(pos_[i]);
			wNeg_.push_back(-wPos_[i]);
		}
		else if(wPos_[i] > 0)
		{
			card_ = card_ && wPos_[i] == 1;
			if(j < i)
			{
				pos_[j]  = pos_[i];
				wPos_[j] = wPos_[i];
			}
			sum+= wPos_[j++];
		}
	}
	pos_.resize(j);
	wPos_.resize(j);
	j = 0;
	for(size_t i = 0; i < neg_.size(); i++)
	{
		if(wNeg_[i] < 0)
		{
			card_ = card_ && wNeg_[i] == -1;
			lower_-= wNeg_[i];
			upper_-= wNeg_[i];
			sum   -= wNeg_[i];
			pos_.push_back(neg_[i]);
			wPos_.push_back(-wNeg_[i]);
		}
		else if(wNeg_[i] > 0)
		{
			card_ = card_ && wNeg_[i] == 1;
			if(j < i)
			{
				neg_[j]  = neg_[i];
				wNeg_[j] = wNeg_[i];
			}
			sum+= wNeg_[j++];
		}
	}
	neg_.resize(j);
	wNeg_.resize(j);

	// turn into count aggregate (important for optimizations later on)
	if(wNeg_.size() + wPos_.size() == 1 && sum > 1)
	{
		if(hasLower())             { lower_ = (lower_ + sum - 1) / sum; }
		if(hasUpper())             { upper_ = upper_ / sum; }
		if(wPos_.size() > 0)      { wPos_[0] = 1; }
		else if(wNeg_.size() > 0) { wNeg_[0] = 1; }
		sum = 1;
	}

	// TODO: multiple occurrences of the same literal could be combined
	//       in the head even between true and false literals!!!
	if(hasLower() && lower_ <= 0)   { lower_ = std::numeric_limits<int32_t>::min(); }
	if(hasUpper() && upper_ >= sum) { upper_ = std::numeric_limits<int32_t>::max(); }
	if(hasUpper() && hasLower() && lower_ > upper_) { return sign_; }
	if((upper_ < 0) || (sum < lower_))              { return sign_; }
	else if(!hasLower() && !hasUpper())             { return !sign_; }
	else                                            { return unknown; }
}

void SumAggr::printHead(LparseConverter *output, uint32_t a)
{
	tribool truthValue = simplify();

	assert(!sign_);
	if(!truthValue)
	{
		output->printBasicRule(output->falseSymbol(), AtomVec(1, a), AtomVec());
	}
	else if(boost::logic::indeterminate(truthValue))
	{
		if(pos_.size() == 1 && neg_.size() == 0 && hasLower())
		{
			output->printBasicRule(pos_[0], AtomVec(1, a), AtomVec());
		}
		else
		{
			assert(hasLower() || hasUpper());
			if(hasLower())
			{
				uint32_t l = output->symbol();
				if(card_) { output->printConstraintRule(l, lower_, pos_, neg_); }
				else      { output->printWeightRule(l, lower_, pos_, neg_, wPos_, wNeg_); }
				output->printBasicRule(output->falseSymbol(), AtomVec(1, a), AtomVec(1, l));
			}
			if(hasUpper())
			{
				uint32_t u = output->symbol();
				if(card_) { output->printConstraintRule(u, upper_ + 1, pos_, neg_); }
				else      { output->printWeightRule(u, upper_ + 1, pos_, neg_, wPos_, wNeg_); }
				LparseConverter::AtomVec pos;
				pos.push_back(u);
				pos.push_back(a);
				output->printBasicRule(output->falseSymbol(), pos, AtomVec());
			}
		}
	}
}

void SumAggr::printBody(LparseConverter *output, uint32_t a)
{
	tribool truthValue = simplify();
	if(truthValue)
	{
		output->printBasicRule(a, AtomVec(), AtomVec());
	}
	else if(boost::logic::indeterminate(truthValue))
	{
		if(pos_.size() == 1 && neg_.empty() && hasLower())
		{
			if(sign_) { output->printBasicRule(a, AtomVec(), AtomVec(1, pos_[0])); }
			else      { output->printBasicRule(a, AtomVec(1, pos_[0]), AtomVec()); }
		}
		else if(pos_.size() == 1 && neg_.empty() && !sign_ && hasUpper())
		{
			output->printBasicRule(a, AtomVec(), AtomVec(1, pos_[0]));
		}
		else if(pos_.empty() && neg_.size() == 1 && !sign_ && hasLower())
		{
			output->printBasicRule(a, AtomVec(), AtomVec(1, neg_[0]));
		}
		else if(pos_.empty() && neg_.size() == 1 &&  sign_ && hasUpper())
		{
			output->printBasicRule(a, AtomVec(), AtomVec(1, neg_[0]));
		}
		else
		{
			if(!hasLower() || !hasUpper() || !sign_)
			{
				if(hasLower())
				{
					uint32_t l = output->symbol();
					if(card_) { output->printConstraintRule(l, lower_, pos_, neg_); }
					else      { output->printWeightRule(l, lower_, pos_, neg_, wPos_, wNeg_); }
					if(sign_) { output->printBasicRule(a, AtomVec(), AtomVec(1, l)); }
					else      { output->printBasicRule(a, AtomVec(1, l), AtomVec()); }
				}
				if(hasUpper())
				{
					uint32_t u = output->symbol();
					if(card_) { output->printConstraintRule(u, upper_ + 1, pos_, neg_); }
					else      { output->printWeightRule(u, upper_ + 1, pos_, neg_, wPos_, wNeg_); }
					if(!sign_) { output->printBasicRule(a, AtomVec(), AtomVec(1, u)); }
					else
					{
						uint32_t n = output->symbol();
						output->printBasicRule(a, AtomVec(), AtomVec(1, n));
						output->printBasicRule(n, AtomVec(), AtomVec(1, u));
					}
				}
			}
			else
			{
				uint32_t l = output->symbol();
				uint32_t u = output->symbol();
				uint32_t n = output->symbol();
				output->printBasicRule(a, AtomVec(), AtomVec(1, n));
				output->printBasicRule(n, LparseConverter::AtomVec(1, l), LparseConverter::AtomVec(1, u));
				if(card_) { output->printConstraintRule(l, lower_, pos_, neg_); }
				else      { output->printWeightRule(l, lower_, pos_, neg_, wPos_, wNeg_); }
				if(card_) { output->printConstraintRule(u, upper_ + 1, pos_, neg_); }
				else      { output->printWeightRule(u, upper_ + 1, pos_, neg_, wPos_, wNeg_); }
			}
		}
	}
}

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::CondLitPrinter, CondLit::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::SumAggrLitPrinter, SumAggrLit::Printer, LparseConverter)
