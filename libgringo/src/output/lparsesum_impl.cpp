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

template <class C>
void sort_unique(C &c)
{
	std::sort(c.begin(), c.end());
	c.erase(std::unique(c.begin(), c.end()), c.end());
}

template <class T, class P, class C>
T combine_adjacent(T first, T last, P p, C c)
{
	T result = first;
	while (++first != last)
	{
		if (!p(*result, *first)) { *(++result) = *first; }
		else if(result != first)  { c(*result, *first); }
	}
	return ++result;
}
class SumAggr
{
	typedef LparseConverter::AtomVec AtomVec;
	typedef LparseConverter::WeightVec WeightVec;

public:

	SumAggr(const SumAggrLitPrinter::TodoKey &key, const SumAggrLitPrinter::TodoVal &val);

	tribool simplify();
	// TODO: merge!!
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
	bool      card_;
};

}

////////////////////////////////// AggrCondPrinter::Cond //////////////////////////////////

void AggrCondPrinter::Cond::simplify()
{
	foreach(HeadLits hl, lits) { sort_unique(hl.second); }
	sort_unique(lits);
}

////////////////////////////////// AggrCondPrinter //////////////////////////////////

void AggrCondPrinter::begin(State state, const ValVec &set)
{
	CondMap &conds = stateMap_.insert(StateMap::value_type(state, CondMap(boost::bind(Val::compare, output()->storage(), _1, _2) < 0))).first->second;
	current_ = &conds[set];
	current_->lits.push_back(Cond::HeadLits());
}

AggrCondPrinter::CondMap *AggrCondPrinter::state(State state)
{
	StateMap::iterator it = stateMap_.find(state);
	if(it != stateMap_.end()) { return &stateMap_.find(state)->second; }
	else                      { return 0; }
}

void AggrCondPrinter::endHead()
{
	LitVec &lits = current_->lits.back().second;
	assert(lits.size() <= 1);
	if(!lits.empty() && lits.back() > 0)
	{
		current_->lits.back().first = lits.back();
		lits.pop_back();
	}
}

void AggrCondPrinter::print(PredLitRep *l)
{
	// TODO: possibly push the predlit rep!
	int32_t sym = (int32_t)output_->symbol(l);
	current_->lits.back().second.push_back(l->sign() ? -sym : sym);
}

////////////////////////////////// SumAggrLitPrinter::TodoKey //////////////////////////////////

SumAggrLitPrinter::TodoKey::TodoKey()
{
}

SumAggrLitPrinter::TodoKey::TodoKey(State state)
	: state(state)
	, lower(Val::inf())
	, upper(Val::sup())
	, lleq(true)
	, uleq(true)
{
}

bool SumAggrLitPrinter::TodoKey::operator==(const TodoKey &k) const
{
	return
		state == k.state &&
		lower == k.lower &&
		upper == k.upper &&
		lleq  == k.lleq  &&
		uleq  == k.uleq;
}

size_t hash_value(const SumAggrLitPrinter::TodoKey &v)
{
	size_t hash = 0;
	boost::hash_combine(hash, v.state);
	boost::hash_combine(hash, v.lower);
	boost::hash_combine(hash, v.upper);
	boost::hash_combine(hash, v.lleq);
	boost::hash_combine(hash, v.uleq);
	return hash;
}

////////////////////////////////// SumAggrLitPrinter::TodoVal //////////////////////////////////

SumAggrLitPrinter::TodoVal::TodoVal()
{
}

SumAggrLitPrinter::TodoVal::TodoVal(uint32_t symbol, bool head)
	: symbol(symbol)
	, head(head)
{
}

////////////////////////////////// SumAggrLitPrinter //////////////////////////////////

SumAggrLitPrinter::SumAggrLitPrinter(LparseConverter *output)
	: output_(output)
{
	output->regDelayedPrinter(this);
}

void SumAggrLitPrinter::begin(State state, bool head, bool sign, bool)
{
	key_ = TodoKey(state);
	val_ = TodoVal(output_->symbol(), head);
	RulePrinter *printer = static_cast<RulePrinter*>(output()->printer<Rule::Printer>());
	if(head)
	{
		assert(!sign);
		printer->setHead(val_.symbol);
	}
	else { printer->addBody(val_.symbol, sign); }
}

void SumAggrLitPrinter::lower(const Val &l, bool leq)
{
	key_.lower = l;
	key_.lleq  = leq;
}

void SumAggrLitPrinter::upper(const Val &u, bool leq)
{
	key_.upper = u;
	key_.uleq  = leq;
}

void SumAggrLitPrinter::end()
{
	todo_.insert(TodoMap::value_type(key_, val_));
}

namespace
{
	struct CondEqual
	{
		bool operator()(const SumAggrLitPrinter::SetCondVec::value_type &a, const SumAggrLitPrinter::SetCondVec::value_type &b)
		{
			return a.second->lits == b.second->lits;
		}
	};

	struct CondLess
	{
		bool operator()(const SumAggrLitPrinter::SetCondVec::value_type &a, const SumAggrLitPrinter::SetCondVec::value_type &b)
		{
			return a.second->lits < b.second->lits;
		}
	};
}

void SumAggrLitPrinter::combine(SetCondVec::value_type &a, const SetCondVec::value_type &b)
{
	a.first[0].num+= b.first[0].num;
}

bool SumAggrLitPrinter::analyze(const SetCondVec::value_type &a, int64_t &min, int64_t &max, int64_t &fix)
{
	int32_t w = a.first[0].num;
	if(a.second->lits.empty())
	{
		fix+= w;
		min+= w;
		max+= w;
		return true;
	}
	else
	{
		if(w < 0) { min+= w; }
		else      { max+= w; }
		return false;
	}
}

void SumAggrLitPrinter::printAggr(const TodoKey &key, const TodoVal &val, SetCondVec &condVec)
{
	// aggregate specific
	std::sort(condVec.begin(), condVec.end(), CondLess());
	condVec.erase(combine_adjacent(condVec.begin(), condVec.end(), CondEqual(), &SumAggrLitPrinter::combine), condVec.end());
	int64_t min = 0, max = 0, fix = 0;
	condVec.erase(std::remove_if(condVec.begin(), condVec.end(), boost::bind(&SumAggrLitPrinter::analyze, _1, boost::ref(min), boost::ref(max), boost::ref(fix))), condVec.end());

	// aggregate generic
	if(key.lower.compare(max) > -!key.lleq || key.upper.compare(min) < !key.lleq || key.upper.compare(key.lower, output()->storage()) < !(key.lleq && key.uleq))
	{
		if(val.head) { output()->printBasicRule(output()->falseSymbol(), AtomVec(1, val.symbol), AtomVec()); }
	}
	else
	{
		bool hasLower  = key.lower.compare(min) > -!key.lleq;
		bool hasUpper  = key.upper.compare(max) <  !key.uleq;
		bool hasBound  = hasLower || hasUpper;
		bool singleton = hasBound && condVec.size() == 1;
		if(singleton)
		{
			assert(hasUpper != hasLower);
			uint32_t cur = 0;
			bool     ass = false;
			foreach(AggrCondPrinter::Cond::HeadLits hl, condVec.back().second->lits)
			{
				if(!ass)
				{
					cur = hl.first;
					ass = true;
				}
				if(hl.first == 0 || cur != hl.first)
				{
					singleton = false;
					break;
				}
			}
		}
		LitVec  conds;
		AtomVec trivChoice;
		foreach(SetCondVec::value_type &cond, condVec)
		{
			if(cond.second->lits.size() == 1 && cond.second->lits.back().first != 0 && cond.second->lits.back().second.empty())
			{
				trivChoice.push_back(cond.second->lits.back().first);
				if(hasLower || hasUpper) { conds.push_back(0); }
			}
			else
			{
				foreach(AggrCondPrinter::Cond::HeadLits &hl, cond.second->lits)
				{
					if(hl.first != 0 || hasBound)
					{
						AtomVec pos, neg;
						foreach(int32_t lit, hl.second)
						{
							if(lit < 0) { neg.push_back(-lit); }
							else        { pos.push_back(lit); }
						}
						if(hasBound)
						{
							if(hl.second.size() > 1)
							{
								uint32_t cond = output()->symbol();
								output()->printBasicRule(cond, pos, neg);
								pos.clear();
								neg.clear();
								pos.push_back(cond);
								conds.push_back(cond);
							}
							else if(hl.second.size() == 1) { conds.push_back(hl.second.back()); }
							else                           { conds.push_back(0); }
						}
						if(hl.first != 0)
						{
							pos.push_back(val.symbol);
							if(hl.second.empty()) { trivChoice.push_back(hl.first); }
							else if(!singleton)   { output()->printChoiceRule(AtomVec(1, hl.first), pos, neg); }
							else
							{
								if(hasLower) { output()->printBasicRule(hl.first, pos, neg); }
								else
								{
									pos.push_back(hl.first);
									output()->printBasicRule(output()->falseSymbol(), pos, neg);
								}
							}
						}
					}
				}
			}
		}
		if(!trivChoice.empty())
		{
			if(singleton)
			{
				assert(trivChoice.size() == 1);
				uint32_t head = trivChoice.back();
				if(hasLower) { output()->printBasicRule(head, AtomVec(1, val.symbol), AtomVec()); }
				else
				{
					AtomVec pos;
					pos.push_back(val.symbol);
					pos.push_back(head);
					output()->printBasicRule(output()->falseSymbol(), pos, AtomVec());
				}
			}
			else { output()->printChoiceRule(trivChoice, AtomVec(1, val.symbol), AtomVec()); }
		}
		if(!singleton)
		{
			if(!hasBound)
			{
				if(!val.head) { output()->printBasicRule(val.symbol, AtomVec(), AtomVec()); }
			}
			else
			{
				// aggregate specific
				std::cerr << "todo: " << val.symbol << std::endl;
				// create cond lits for checking:
				// if head:   :- not l { cond lits } u, a.
				// else:    a :-     l { cond lits } u.
			}
		}
	}
}

void SumAggrLitPrinter::finish()
{
	foreach(TodoMap::value_type &todo, todo_)
	{
		AggrCondPrinter::CondMap *conds = static_cast<AggrCondPrinter*>(output()->printer<AggrCond::Printer>())->state(todo.first.state);
		if(conds)
		{
			foreach(AggrCondPrinter::CondMap::value_type &c, *conds) { c.second.simplify(); }
			SetCondVec condVec;
			foreach(AggrCondPrinter::CondMap::value_type &c, *conds) { condVec.push_back(SetCondVec::value_type(c.first, &c.second)); }
			printAggr(todo.first, todo.second, condVec);
		}
	}
	todo_.clear();
}

////////////////////////////////// SumAggr //////////////////////////////////

/*
SumAggr::SumAggr(TodoKey &key, TodoVal &val)
	: key(key)
	, val(val)
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
	return upper_;
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
		// TODO: more could be done already at a previous place
		if(pos_.size() == 1 && neg_.size() == 0 && hasLower())
		{
			output->printBasicRule(output->falseSymbol(), AtomVec(1, a), AtomVec(1, pos_[0]));
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
			if(hasLower() && !hasUpper())
			{
				uint32_t l = output->symbol();
				if(card_) { output->printConstraintRule(l, lower_, pos_, neg_); }
				else      { output->printWeightRule(l, lower_, pos_, neg_, wPos_, wNeg_); }
				if(sign_) { output->printBasicRule(a, AtomVec(), AtomVec(1, l)); }
				else      { output->printBasicRule(a, AtomVec(1, l), AtomVec()); }
			}
			else if(!hasLower() && hasUpper())
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
			else
			{
				uint32_t l = output->symbol();
				uint32_t u = output->symbol();
				uint32_t n;
				if(sign_)
				{
					n = output->symbol();
					output->printBasicRule(a, AtomVec(), AtomVec(1, n));
				}
				else { n = a; }
				output->printBasicRule(n, LparseConverter::AtomVec(1, l), LparseConverter::AtomVec(1, u));
				if(card_) { output->printConstraintRule(l, lower_, pos_, neg_); }
				else      { output->printWeightRule(l, lower_, pos_, neg_, wPos_, wNeg_); }
				if(card_) { output->printConstraintRule(u, upper_ + 1, pos_, neg_); }
				else      { output->printWeightRule(u, upper_ + 1, pos_, neg_, wPos_, wNeg_); }
			}
		}
	}
}
*/

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::AggrCondPrinter, AggrCond::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::SumAggrLitPrinter, AggrLit::Printer<SumAggrLit>, LparseConverter)
