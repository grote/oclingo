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

#include "output/lparseaggr_impl.h"
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

////////////////////////////////// AggrTodoKey //////////////////////////////////

AggrTodoKey::AggrTodoKey()
{
}

AggrTodoKey::AggrTodoKey(State state)
	: state(state)
	, lower(Val::inf())
	, upper(Val::sup())
	, lleq(true)
	, uleq(true)
{
}

bool AggrTodoKey::operator==(const AggrTodoKey &k) const
{
	return
		state == k.state &&
		lower == k.lower &&
		upper == k.upper &&
		lleq  == k.lleq  &&
		uleq  == k.uleq;
}

size_t hash_value(const AggrTodoKey &v)
{
	size_t hash = 0;
	boost::hash_combine(hash, v.state);
	boost::hash_combine(hash, v.lower);
	boost::hash_combine(hash, v.upper);
	boost::hash_combine(hash, v.lleq);
	boost::hash_combine(hash, v.uleq);
	return hash;
}

////////////////////////////////// AggrTodoVal //////////////////////////////////

AggrTodoVal::AggrTodoVal()
{
}

AggrTodoVal::AggrTodoVal(uint32_t symbol, bool head)
	: symbol(symbol)
	, head(head)
{
}

////////////////////////////////// SumAggrLitPrinter //////////////////////////////////

template <class T, uint32_t Type>
AggrLitPrinter<T, Type>::AggrLitPrinter(LparseConverter *output)
	: output_(output)
{
	output->regDelayedPrinter(this);
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::begin(State state, bool head, bool sign, bool)
{
	key_ = AggrTodoKey(state);
	val_ = AggrTodoVal(output_->symbol(), head);
	RulePrinter *printer = static_cast<RulePrinter*>(output()->template printer<Rule::Printer>());
	if(head)
	{
		assert(!sign);
		printer->setHead(val_.symbol);
	}
	else { printer->addBody(val_.symbol, sign); }
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::lower(const Val &l, bool leq)
{
	key_.lower = l;
	key_.lleq  = leq;
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::upper(const Val &u, bool leq)
{
	key_.upper = u;
	key_.uleq  = leq;
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::end()
{
	todo_.insert(TodoMap::value_type(key_, val_));
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::handleCond(bool &single, int32_t &c, int32_t cond, uint32_t head)
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
	if(complex || (head != 0 && int32_t(head) != c) || (cond != 0 && cond != c))
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

template <class T, uint32_t Type>
int32_t AggrLitPrinter<T, Type>::getCondSym(const SetCond &cond, LitVec::iterator &lit)
{
	bool    single  = true;
	int32_t c       = 0;
	foreach(Cond::HeadLits &hl, cond.second->lits) { handleCond(single, c, *lit++, hl.first); }
	return c;
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::getCondSyms(LitVec &conds, SetCondVec &condVec, LitVec &condSyms)
{
	LitVec::iterator it = conds.begin();
	foreach(SetCondVec::value_type &cond, condVec) { condSyms.push_back(getCondSym(cond, it)); }
}

template <class T, uint32_t Type>
template <class W>
AggrBoundCheck AggrLitPrinter<T, Type>::checkBounds(const AggrTodoKey &key, const W &min, const W &max)
{
	Storage *s = output()->storage();
	bool isFalse  = key.lower.compare(max, s) > -!key.lleq || key.upper.compare(min, s) < !key.uleq || key.upper.compare(key.lower, s) < !(key.lleq && key.uleq);
	bool hasLower = !isFalse && key.lower.compare(min, s) > -!key.lleq;
	bool hasUpper = !isFalse && key.upper.compare(max, s) <  !key.uleq;
	return AggrBoundCheck(isFalse, hasLower, hasUpper);
}

template <class T, uint32_t Type>
bool AggrLitPrinter<T, Type>::handleAggr(const AggrBoundCheck &b, const AggrTodoVal &val, SetCondVec &condVec, LitVec &conds)
{
	if(b.isFalse)
	{
		if(val.head) { output()->printBasicRule(output()->falseSymbol(), 1, val.symbol); }
	}
	else
	{
		bool hasBound  = b.hasLower || b.hasUpper;
		bool singleton = hasBound && condVec.size() == 1;
		if(singleton)
		{
			assert(b.hasUpper != b.hasLower);
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
		AtomVec trivChoice;
		foreach(SetCondVec::value_type &cond, condVec)
		{
			if(cond.second->lits.size() == 1 && cond.second->lits.back().first != 0 && cond.second->lits.back().second.empty())
			{
				trivChoice.push_back(cond.second->lits.back().first);
				if(b.hasLower || b.hasUpper) { conds.push_back(0); }
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
								if(b.hasLower) { output()->printBasicRule(hl.first, pos, neg); }
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
				if(b.hasLower) { output()->printBasicRule(head, 1, val.symbol); }
				else           { output()->printBasicRule(output()->falseSymbol(), 2, val.symbol, head); }
			}
			else { output()->printChoiceRule(trivChoice, AtomVec(1, val.symbol), AtomVec()); }
		}
		if(!singleton)
		{
			if(!hasBound)
			{
				if(!val.head) { output()->printBasicRule(val.symbol, 0); }
			}
			else { return true; }
		}
	}
	return false;
}

template <class T, uint32_t Type>
void AggrLitPrinter<T, Type>::finish()
{
	foreach(TodoMap::value_type &todo, todo_)
	{
		AggrCondPrinter::CondMap *conds = static_cast<AggrCondPrinter*>(output()->template printer<AggrCond::Printer>())->state(todo.first.state);
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

template <class T, uint32_t Type>
LparseConverter *AggrLitPrinter<T, Type>::output() const
{
	return output_;
}

/*
template <class T, uint32_t Type>
std::ostream &AggrLitPrinter<T, Type>::out() const
{
	return output_->out();
}
*/

template <class T, uint32_t Type>
AggrLitPrinter<T, Type>::~AggrLitPrinter()
{
}

////////////////////////////////// SumAggrLitPrinter //////////////////////////////////

SumAggrLitPrinter::SumAggrLitPrinter(LparseConverter *output)
	: AggrLitPrinter<SumAggrLit>(output)
{
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

void SumAggrLitPrinter::printAggr(const AggrTodoKey &key, const AggrTodoVal &val, SetCondVec &condVec)
{
	std::sort(condVec.begin(), condVec.end(), CondLess());
	condVec.erase(combine_adjacent(condVec.begin(), condVec.end(), CondEqual(), &SumAggrLitPrinter::combine), condVec.end());
	int64_t min = 0, max = 0, fix = 0;
	condVec.erase(std::remove_if(condVec.begin(), condVec.end(), boost::bind(&SumAggrLitPrinter::analyze, _1, boost::ref(min), boost::ref(max), boost::ref(fix))), condVec.end());

	AggrBoundCheck b = checkBounds(key, min, max);
	LitVec conds;

	if(handleAggr(b, val, condVec, conds))
	{
		uint32_t l = 0, u = 0;
		LitVec condSyms;
		getCondSyms(conds, condVec, condSyms);
		if(b.hasLower)
		{
			assert(key.lower.type == Val::NUM);
			l = !b.hasUpper && !val.head ? val.symbol : output()->symbol();
			int64_t lower = key.lower.num + !key.lleq - fix;
			printSum(l, lower, condSyms, condVec);
		}
		if(b.hasUpper)
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

////////////////////////////////// MinMaxAggrLitPrinter //////////////////////////////////

template <uint32_t T>
MinMaxAggrLitPrinter<T>::MinMaxAggrLitPrinter(LparseConverter *output)
	: AggrLitPrinter<MinMaxAggrLit, T>(output)
{
}

template <uint32_t T>
void MinMaxAggrLitPrinter<T>::combine(Storage *s, SetCond &a, const SetCond &b)
{
	int sign = (T == MinMaxAggrLit::MINIMUM ? 1 : -1);
	int cmp  = a.first[0].compare(b.first[0].num, s);
	if(sign * cmp > 0) { a.first[0] = b.first[0]; }
}

template <uint32_t T>
bool MinMaxAggrLitPrinter<T>::analyze(Storage *s, const SetCond &a, Val &min, Val &max, Val &fix)
{
	int sign = (T == MinMaxAggrLit::MINIMUM ? 1 : -1);
	const Val &w = a.first[0];
	if(min.compare(w, s) > 0) { min = w; }
	if(max.compare(w, s) < 0) { max = w; }
	if(a.second->lits.empty())
	{
		if(sign * fix.compare(w, s) > 0) { fix = w; }
		return true;
	}
	else { return false; }
}

template <uint32_t T>
void MinMaxAggrLitPrinter<T>::printAggr(const AggrTodoKey &key, const AggrTodoVal &val, SetCondVec &condVec)
{
	LparseConverter *o = this->template output();
	Storage *s = o->storage();

	std::sort(condVec.begin(), condVec.end(), CondLess());
	condVec.erase(combine_adjacent(condVec.begin(), condVec.end(), CondEqual(), boost::bind(&MinMaxAggrLitPrinter::combine, s, _1, _2)), condVec.end());
	Val min = Val::sup();
	Val max = Val::inf();
	Val fix = T == MinMaxAggrLit::MINIMUM ? Val::sup() : Val::inf();
	condVec.erase(std::remove_if(condVec.begin(), condVec.end(), boost::bind(&MinMaxAggrLitPrinter::analyze, s, _1, boost::ref(min), boost::ref(max), boost::ref(fix))), condVec.end());

	if(T == MinMaxAggrLit::MINIMUM && max.compare(fix, s) > 0) { max = fix; }
	if(T == MinMaxAggrLit::MAXIMUM && min.compare(fix, s) < 0) { min = fix; }

	AggrBoundCheck b = this->template checkBounds(key, min, max);
	LitVec conds;

	// NOTE: in theory some choices could be dropped, e.g., p(1) and p(2) in:
	//       2 < #min { <X>:p(X) : X=1..5 } < 5.
	//       this holds for aggregates in general if a literal would make the aggregate false
	//       no choices has to be generated (but the literal is still important for the check)
	if(handleAggr(b, val, condVec, conds))
	{
		uint32_t mp = 0, ap = 0;
		if(T == MinMaxAggrLit::MINIMUM ? b.hasLower : b.hasUpper) { ap = o->symbol(); }
		if(T == MinMaxAggrLit::MINIMUM ? b.hasUpper : b.hasLower) { mp = ap == 0 && !val.head ? val.symbol : o->symbol(); }
		/*
		monotone:
			Minimum || Maximum:
				mp :- lit. if lit.weight >= lower and lit.weight <= upper
				mp :- lit. if lit.weight >= lower and lit.weight <= upper
		antimonotone:
			Minimum:
				ap :- lit. lit.weight < lower.
			Maximum:
				ap :- lit. lit.weight > upper.
		*/

		LitVec::iterator lit = conds.begin();
		foreach(SetCond &cond, condVec)
		{
			const Val &w = cond.first[0];
			bool needsM =
				(mp != 0) && (
					(!b.hasLower || w.compare(key.lower, s) > -key.lleq) &&
					(!b.hasUpper || w.compare(key.upper, s) <  key.uleq) );

			bool needsA =
				(ap != 0) && (
					(T == MinMaxAggrLit::MINIMUM && w.compare(key.lower, s) <  !key.lleq) ||
					(T == MinMaxAggrLit::MAXIMUM && w.compare(key.upper, s) > -!key.uleq) );

			if(needsA || needsM)
			{
				uint32_t sym = getCondSym(cond, lit);
				if(needsM) { o->printBasicRule(mp, 1, sym); }
				if(needsA) { o->printBasicRule(ap, 1, sym); }
			}
		}

		if(val.head)
		{
			if(mp != 0) { o->printBasicRule(o->falseSymbol(), 2, val.symbol, -mp); }
			if(ap != 0) { o->printBasicRule(o->falseSymbol(), 2, val.symbol, ap); }
		}
		else if(mp != val.symbol) { o->printBasicRule(val.symbol, 2, mp, -ap); }
	}
}

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::AggrCondPrinter, AggrCond::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::SumAggrLitPrinter, AggrLit::Printer<SumAggrLit>, LparseConverter)
typedef AggrLit::Printer<MinMaxAggrLit, MinMaxAggrLit::MINIMUM> MinPrinterBase;
typedef AggrLit::Printer<MinMaxAggrLit, MinMaxAggrLit::MAXIMUM> MaxPrinterBase;
GRINGO_REGISTER_PRINTER(lparseconverter_impl::MinAggrLitPrinter, MinPrinterBase, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::MaxAggrLitPrinter, MaxPrinterBase, LparseConverter)
