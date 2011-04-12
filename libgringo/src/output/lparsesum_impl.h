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

class RulePrinter;

class AggrCondPrinter : public AggrCond::Printer
{
public:
	struct Cond
	{
		LparseConverter::AtomVec head;
		LparseConverter::AtomVec pos;
		LparseConverter::AtomVec neg;

		bool operator<(const Cond &c) const;
		bool operator==(const Cond &c) const;
	};
	typedef boost::unordered_map<ValVec, std::vector<Cond> > CondMap;
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
	Output *output() const;
	std::ostream &out() const;
private:
	LparseConverter *output_;
	Cond            *current_;
	StateMap         stateMap_;
	bool             isHead_;
};

class SumAggrLitPrinter : public AggrLit::Printer<SumAggrLit>, public DelayedPrinter
{
	typedef std::pair<State, std::pair<int32_t, int32_t> > TodoKey;
	typedef boost::tuples::tuple<uint32_t, bool, bool> TodoVal;
	typedef boost::unordered_map<TodoKey, TodoVal> TodoMap;
public:
	SumAggrLitPrinter(LparseConverter *output);
	void begin(State state, bool head, bool sign, bool complete);
	void lower(const Val &l, bool leq);
	void upper(const Val &u, bool leq);
	void end();
	Output *output() const;
	std::ostream &out() const;
	void finish();
private:
	TodoMap            todo_;
	LparseConverter   *output_;
	State              state_;
	int32_t            lower_;
	int32_t            upper_;
	bool               head_;
	bool               sign_;
};

//////////////////////////////// AggrCondPrinter::Cond ////////////////////////////////

inline bool AggrCondPrinter::Cond::operator<(const Cond &c) const
{
	if(head != c.head) { return head < c.head; }
	if(pos != c.pos)   { return pos < c.pos; }
	if(neg != c.neg)   { return neg < c.neg; }
	return false;
}

inline bool AggrCondPrinter::Cond::operator==(const Cond &c) const
{
	return head == c.head && pos == c.pos && neg == c.neg;
}

//////////////////////////////// AggrCondPrinter ////////////////////////////////

inline AggrCondPrinter::AggrCondPrinter(LparseConverter *output) : output_(output) { }
inline void AggrCondPrinter::trueLit() { }
inline Output *AggrCondPrinter::output() const { return output_; }
inline std::ostream &AggrCondPrinter::out() const { return output_->out(); }
inline void AggrCondPrinter::end() { }


//////////////////////////////// SumAggrLitPrinter ////////////////////////////////

inline void SumAggrLitPrinter::lower(const Val &l, bool leq) { lower_ = leq ? l.num : l.num + 1; }
inline void SumAggrLitPrinter::upper(const Val &u, bool leq) { upper_ = leq ? u.num : u.num - 1; }
inline Output *SumAggrLitPrinter::output() const { return output_; }
inline std::ostream &SumAggrLitPrinter::out() const { return output_->out(); }


}
