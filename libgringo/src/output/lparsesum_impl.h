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
	std::ostream &out() const;
private:
	LparseConverter *output_;
	Cond            *current_;
	StateMap         stateMap_;
};

class SumAggrLitPrinter : public AggrLit::Printer<SumAggrLit>, public DelayedPrinter
{
public:
	struct TodoKey
	{
		TodoKey();
		TodoKey(State state);
		bool operator==(const TodoKey &k) const;

		State state;
		Val   lower;
		Val   upper;
		bool  lleq;
		bool  uleq;
	};

	struct TodoVal
	{
		TodoVal();
		TodoVal(uint32_t symbol, bool head);

		uint32_t symbol;
		bool     head;
	};

	typedef LparseConverter::AtomVec AtomVec;
	typedef std::vector<AggrCondPrinter::Cond*> CondVec;
	typedef boost::unordered_map<TodoKey, TodoVal> TodoMap;
	typedef std::vector<std::pair<ValVec, AggrCondPrinter::Cond*> > SetCondVec;
	typedef AggrCondPrinter::Cond Cond;
public:
	SumAggrLitPrinter(LparseConverter *output);
	void begin(State state, bool head, bool sign, bool complete);
	void lower(const Val &l, bool leq);
	void upper(const Val &u, bool leq);
	void end();
	static void combine(SetCondVec::value_type &a, const SetCondVec::value_type &b);
	static bool analyze(const SetCondVec::value_type &a, int64_t &min, int64_t &max, int64_t &fix);
	void printAggr(const TodoKey &key, const TodoVal &val, SetCondVec &condVec);
	void printSum(uint32_t sym, int64_t bound, LitVec &conds, SetCondVec &condVec);
	void printCond(bool &single, int32_t &c, int32_t cond, uint32_t head);
	void getCondSyms(LitVec &conds, SetCondVec &condVec, LitVec &condSyms);
	LparseConverter *output() const;
	std::ostream &out() const;
	int32_t finishCond(int32_t aggrSym, CondVec &conds);
	void finish();

private:
	LparseConverter   *output_;
	TodoMap            todo_;
	TodoKey            key_;
	TodoVal            val_;
};

size_t hash_value(const SumAggrLitPrinter::TodoKey &v);

//////////////////////////////// AggrCondPrinter ////////////////////////////////

inline AggrCondPrinter::AggrCondPrinter(LparseConverter *output) : output_(output) { }
inline void AggrCondPrinter::trueLit() { }
inline LparseConverter *AggrCondPrinter::output() const { return output_; }
inline std::ostream &AggrCondPrinter::out() const { return output_->out(); }
inline void AggrCondPrinter::end() { }


//////////////////////////////// SumAggrLitPrinter ////////////////////////////////

inline LparseConverter *SumAggrLitPrinter::output() const { return output_; }
inline std::ostream &SumAggrLitPrinter::out() const { return output_->out(); }


}
