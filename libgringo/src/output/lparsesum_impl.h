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

class CondLitPrinter : public CondLit::Printer
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
	typedef boost::unordered_map<uint32_t, CondMap> StateMap;
public:
	CondLitPrinter(LparseConverter *output);
	void begin(uint32_t state, const ValVec &set);
	CondMap &state(uint32_t state);
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

class SumAggrLitPrinter : public SumAggrLit::Printer, public DelayedPrinter
{
	typedef std::pair<uint32_t, std::pair<int32_t, int32_t> > TodoKey;
	typedef boost::tuples::tuple<uint32_t, bool, bool> TodoVal;
	typedef boost::unordered_map<TodoKey, TodoVal> TodoMap;
public:
	SumAggrLitPrinter(LparseConverter *output);
	void begin(uint32_t state, bool head, bool sign, bool complete);
	void lower(int32_t l);
	void upper(int32_t u);
	void end();
	Output *output() const;
	std::ostream &out() const;
	void finish();
private:
	static bool todoCmp(const TodoMap::value_type *a, const TodoMap::value_type *b);
private:
	TodoMap            todo_;
	LparseConverter   *output_;
	uint32_t           state_;
	int32_t            lower_;
	int32_t            upper_;
	bool               head_;
	bool               sign_;
};

//////////////////////////////// CondLitPrinter::Cond ////////////////////////////////

inline bool CondLitPrinter::Cond::operator<(const Cond &c) const
{
	if(head != c.head) { return head < c.head; }
	if(pos != c.pos)   { return pos < c.pos; }
	if(neg != c.neg)   { return neg < c.neg; }
	return false;
}

inline bool CondLitPrinter::Cond::operator==(const Cond &c) const
{
	return head == c.head && pos == c.pos && neg == c.neg;
}

//////////////////////////////// CondLitPrinter ////////////////////////////////

inline CondLitPrinter::CondLitPrinter(LparseConverter *output) : output_(output) { }
inline void CondLitPrinter::CondLitPrinter::trueLit() { }
inline Output *CondLitPrinter::output() const { return output_; }
inline std::ostream &CondLitPrinter::out() const { return output_->out(); }
inline void CondLitPrinter::end() { }


//////////////////////////////// SumAggrLitPrinter ////////////////////////////////

inline void SumAggrLitPrinter::lower(int32_t l) { lower_ = l; }
inline void SumAggrLitPrinter::upper(int32_t u) { upper_ = u; }
inline Output *SumAggrLitPrinter::output() const { return output_; }
inline std::ostream &SumAggrLitPrinter::out() const { return output_->out(); }


}
