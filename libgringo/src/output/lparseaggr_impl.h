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

#pragma once

#include <gringo/gringo.h>
#include <gringo/sumaggrlit.h>
#include <gringo/minmaxaggrlit.h>
#include <gringo/lparseconverter.h>

namespace lparseconverter_impl
{

typedef std::vector<int32_t>       LitVec;
typedef std::pair<int32_t,int32_t> WeightLit;
typedef std::vector<WeightLit>     WeightLitVec;

class RulePrinter;

class AggrCondPrinter : public AggrCond::Printer
{
public:
	struct Cond
	{
		void simplify();

		typedef std::pair<uint32_t, LitVec> HeadLits;
		typedef std::vector<HeadLits> HeadLitsVec;
		HeadLitsVec lits;
	};
	typedef std::map<ValVec, Cond, boost::function2<bool, ValVec, ValVec> > CondMap;
private:
	typedef boost::unordered_map<State, CondMap> StateMap;
public:
	AggrCondPrinter(LparseConverter *output);
	void begin(State state, const ValVec &set);
	CondMap *state(State state);
	void endHead();
	void trueLit();
	void print(PredLitRep *l);
	void end();
	LparseConverter *output() const;
	//std::ostream &out() const;
private:
	LparseConverter *output_;
	Cond            *current_;
	StateMap         stateMap_;
};

struct AggrTodoKey
{
	typedef AggrCond::Printer::State State;

	AggrTodoKey();
	AggrTodoKey(State state);
	bool operator==(const AggrTodoKey &k) const;

	State state;
	Val   lower;
	Val   upper;
	bool  lleq;
	bool  uleq;
};

struct AggrTodoVal
{
	typedef AggrCond::Printer::State State;

	AggrTodoVal();
	AggrTodoVal(uint32_t symbol, bool head);

	uint32_t symbol;
	bool     head;
};

size_t hash_value(const AggrTodoKey &v);

struct AggrBoundCheck
{
	AggrBoundCheck(bool isFalse, bool hasLower, bool hasUpper);

	bool isFalse;
	bool hasLower;
	bool hasUpper;
};

template <class T, uint32_t Type = 0>
class AggrLitPrinter : public AggrLit::Printer<T>, public DelayedPrinter
{
public:
	typedef AggrCond::Printer::State State;
	typedef LparseConverter::AtomVec AtomVec;
	typedef std::vector<AggrCondPrinter::Cond*> CondVec;
	typedef boost::unordered_map<AggrTodoKey, AggrTodoVal> TodoMap;
	typedef std::pair<ValVec, AggrCondPrinter::Cond*> SetCond;
	typedef std::vector<SetCond> SetCondVec;
	typedef AggrCondPrinter::Cond Cond;

public:
	AggrLitPrinter(LparseConverter *output);
	void begin(State state, bool head, bool sign, bool complete);
	void lower(const Val &l, bool leq);
	void upper(const Val &u, bool leq);
	void end();
	template <class W>
	AggrBoundCheck checkBounds(const AggrTodoKey &key, const W &min, const W &max);
	bool handleAggr(const AggrBoundCheck &b, const AggrTodoVal &val, SetCondVec &condVec, LitVec &conds);
	void handleCond(bool &single, int32_t &c, int32_t cond, uint32_t head);
	int32_t getCondSym(const SetCond &cond, LitVec::iterator &lit);
	void getCondSyms(LitVec &conds, SetCondVec &condVec, LitVec &condSyms);
	virtual void printAggr(const AggrTodoKey &key, const AggrTodoVal &val, SetCondVec &condVec) = 0;
	LparseConverter *output() const;
	std::ostream &out() const;
	void finish();

	virtual ~AggrLitPrinter();
private:
	LparseConverter *output_;
	TodoMap          todo_;
	AggrTodoKey      key_;
	AggrTodoVal      val_;
};

class SumAggrLitPrinter : public AggrLitPrinter<SumAggrLit>
{
public:
	SumAggrLitPrinter(LparseConverter *output);
	static void combine(SetCondVec::value_type &a, const SetCondVec::value_type &b);
	static bool analyze(const SetCondVec::value_type &a, int64_t &min, int64_t &max, int64_t &fix);
	void printSum(uint32_t sym, int64_t bound, LitVec &conds, SetCondVec &condVec);
	void printAggr(const AggrTodoKey &key, const AggrTodoVal &val, SetCondVec &condVec);
};

template <uint32_t Type>
class MinMaxAggrLitPrinter : public AggrLitPrinter<MinMaxAggrLit, Type>
{
	typedef typename AggrLitPrinter<MinMaxAggrLit, Type>::SetCond SetCond;
	typedef typename AggrLitPrinter<MinMaxAggrLit, Type>::SetCondVec SetCondVec;
public:
	MinMaxAggrLitPrinter(LparseConverter *output);
	static void combine(Storage *s, SetCond &a, const SetCond &b);
	static bool analyze(Storage *s, const SetCond &a, Val &min, Val &max, Val &fix);
	void printSum(uint32_t sym, int64_t bound, LitVec &conds, SetCondVec &condVec);
	void printAggr(const AggrTodoKey &key, const AggrTodoVal &val, SetCondVec &condVec);
};

struct MinAggrLitPrinter : MinMaxAggrLitPrinter<MinMaxAggrLit::MINIMUM>
{
	MinAggrLitPrinter(LparseConverter *output) : MinMaxAggrLitPrinter<MinMaxAggrLit::MINIMUM>(output) { }
};

struct MaxAggrLitPrinter : MinMaxAggrLitPrinter<MinMaxAggrLit::MAXIMUM>
{
	MaxAggrLitPrinter(LparseConverter *output) : MinMaxAggrLitPrinter<MinMaxAggrLit::MAXIMUM>(output) { }
};


//////////////////////////////// AggrCondPrinter ////////////////////////////////

inline AggrCondPrinter::AggrCondPrinter(LparseConverter *output) : output_(output) { }
inline void AggrCondPrinter::trueLit() { }
inline LparseConverter *AggrCondPrinter::output() const { return output_; }
//inline std::ostream &AggrCondPrinter::out() const { return output_->out(); }
inline void AggrCondPrinter::end() { }

//////////////////////////////// AggrBoundCheck ////////////////////////////////

AggrBoundCheck::AggrBoundCheck(bool isFalse, bool hasLower, bool hasUpper)
	: isFalse(isFalse)
	, hasLower(hasLower)
	, hasUpper(hasUpper)
{
}

}
