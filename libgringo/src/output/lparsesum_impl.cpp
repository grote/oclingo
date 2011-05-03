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
		if(w < 0)  { min+= w; }
		else       { max+= w; }
		return false;
	}
}

void SumAggrLitPrinter::printCond(bool &single, int32_t &c, int32_t cond, uint32_t head)
{
	bool complex = cond != 0 && head != 0;
	if(c == 0)
	{
		if(complex)
		{
			c = output()->symbol();
			single = false;
		}
		else if(cond != 0) { c = cond; }
		else if(head != 0) { c = head; }
	}
	if(complex || (head != 0 && head != c) || (cond != 0 && cond != c))
	{
		if(single)
		{
			int32_t cc = c;
			c          = output()->symbol();
			output()->printBasicRule(c, 1, cc);
			single = false;
		}
		output()->printBasicRule(c, 2, cond, head);
	}
}

void SumAggrLitPrinter::getCondSyms(LitVec &conds, SetCondVec &condVec, LitVec &condSyms)
{
	LitVec::iterator it = conds.begin();
	foreach(SetCondVec::value_type &cond, condVec)
	{
		bool    single  = true;
		int32_t c       = 0;
		foreach(Cond::HeadLits &hl, cond.second->lits) { printCond(single, c, *it++, hl.first); }
		condSyms.push_back(c);
	}
}

void SumAggrLitPrinter::printSum(uint32_t sym, int64_t bound, LitVec &condSyms, SetCondVec &condVec)
{
	LitVec::iterator it = condSyms.begin();
	AtomVec pos, neg;
	LparseConverter::WeightVec wPos, wNeg;
	int64_t max  = 0;
	int32_t maxi = 0;
	bool card    = true;
	foreach(SetCondVec::value_type &cond, condVec)
	{
		int32_t condSym = *it++;
		int32_t weight  = cond.first[0].num;
		if(condSym == 0) { bound -= weight; }
		else
		{
			if(weight < 0)
			{
				// TODO: issue a warning
				condSym*= -1;
				weight *= -1;
				bound  += weight;
			}
			if(condSym > 0)
			{
				pos.push_back(condSym);
				wPos.push_back(weight);
			}
			else if(condSym < 0)
			{
				neg.push_back(-condSym);
				wNeg.push_back(weight);
			}
			if(weight != 1) { card = false; }
			max += weight;
			maxi = std::max(maxi, weight);
		}
	}
	if(bound > max)             { }
	else if(bound <= 0)         { output()->printBasicRule(sym, 0); }
	else if(max - maxi < bound) { output()->printBasicRule(sym, pos, neg); }
	else if(card)               { output()->printConstraintRule(sym, bound, pos, neg); }
	else                        { output()->printWeightRule(sym, bound, pos, neg, wPos, wNeg); }
}

void SumAggrLitPrinter::printAggr(const TodoKey &key, const TodoVal &val, SetCondVec &condVec)
{
	// aggregate specific
	std::sort(condVec.begin(), condVec.end(), CondLess());
	condVec.erase(combine_adjacent(condVec.begin(), condVec.end(), CondEqual(), &SumAggrLitPrinter::combine), condVec.end());
	int64_t min = 0, max = 0, fix = 0;
	condVec.erase(std::remove_if(condVec.begin(), condVec.end(), boost::bind(&SumAggrLitPrinter::analyze, _1, boost::ref(min), boost::ref(max), boost::ref(fix))), condVec.end());

	// aggregate generic
	if(key.lower.compare(max) > -!key.lleq || key.upper.compare(min) < !key.uleq || key.upper.compare(key.lower, output()->storage()) < !(key.lleq && key.uleq))
	{
		if(val.head) { output()->printBasicRule(output()->falseSymbol(), 1, val.symbol); }
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
			singleton    = false;
			foreach(Cond::HeadLits hl, condVec.back().second->lits)
			{
				if(hl.first != 0 && cur == 0)
				{
					cur       = hl.first;
					singleton = true;
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
				int32_t noHead = 0;
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
								uint32_t cond;
								if(hl.first == 0)
								{
									if(noHead == 0)
									{
										cond   = output()->symbol();
										noHead = cond;
									}
									else { cond = noHead; }
								}
								else { cond = output()->symbol(); }
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
				if(hasLower) { output()->printBasicRule(head, 1, val.symbol); }
				else         { output()->printBasicRule(output()->falseSymbol(), 2, val.symbol, head); }
			}
			else { output()->printChoiceRule(trivChoice, AtomVec(1, val.symbol), AtomVec()); }
		}
		if(!singleton)
		{
			if(!hasBound)
			{
				if(!val.head) { output()->printBasicRule(val.symbol, 0); }
			}
			else
			{
				// aggregate specific
				uint32_t l = 0, u = 0;
				LitVec condSyms;
				getCondSyms(conds, condVec, condSyms);
				if(hasLower)
				{
					assert(key.lower.type == Val::NUM);
					l = !hasUpper && !val.head ? val.symbol : output()->symbol();
					int64_t lower = key.lower.num + !key.lleq - fix;
					printSum(l, lower, condSyms, condVec);
				}
				if(hasUpper)
				{
					assert(key.upper.type == Val::NUM);
					u = output()->symbol();
					int64_t upper = key.upper.num - !key.uleq - fix + 1;
					printSum(u, upper, condSyms, condVec);
				}
				if(val.head)
				{
					if(l != 0) { output()->printBasicRule(output()->falseSymbol(), 2, val.symbol, -l); }
					if(u != 0) { output()->printBasicRule(output()->falseSymbol(), 2, val.symbol, u); }
				}
				else if(l != val.symbol) { output()->printBasicRule(val.symbol, 2, l, -u); }
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

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::AggrCondPrinter, AggrCond::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::SumAggrLitPrinter, AggrLit::Printer<SumAggrLit>, LparseConverter)
