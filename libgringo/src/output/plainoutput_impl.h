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

#include <gringo/plainoutput.h>
#include <gringo/rule.h>
#include <gringo/sumaggrlit.h>
#include <gringo/avgaggrlit.h>
#include <gringo/junctionlit.h>
#include <gringo/minmaxaggrlit.h>
#include <gringo/parityaggrlit.h>
#include <gringo/optimize.h>
#include <gringo/compute.h>
#include <gringo/show.h>
#include <gringo/external.h>
#include <gringo/inclit.h>

namespace plainoutput_impl
{
	class DisplayPrinter : public Display::Printer
	{
	public:
		DisplayPrinter(PlainOutput *output) : output_(output) { }
		void begin(const Val &head, Type type);
		void print(PredLitRep *l);
		void end();
		PlainOutput *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }

	private:
		PlainOutput *output_;
		bool         ignore_;
		bool         ignored_;
	};

	class ExternalPrinter : public External::Printer
	{
	public:
		ExternalPrinter(PlainOutput *output) : output_(output) { }
		void print(PredLitRep *l);
		PlainOutput *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
	};

	class RulePrinter : public Rule::Printer
	{
	public:
		RulePrinter(PlainOutput *output) : output_(output) { }
		void begin();
		void endHead();
		void print(PredLitRep *l);
		void end();
		PlainOutput *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
		bool         head_;
		bool         printed_;
	};

	class AggrCondPrinter : public AggrCond::Printer
	{
	public:
		typedef std::vector<boost::tuples::tuple<ValVec, AggrCond::Style, std::string> > CondVec;
	private:
		typedef boost::unordered_map<State, CondVec> StateMap;
	public:
		AggrCondPrinter(PlainOutput *output) : output_(output) { }
		void begin(AggrCond::Style style, State state, const ValVec &set);
		CondVec &state(State state);
		void endHead();
		void trueLit();
		void print(PredLitRep *l);
		void end();
		PlainOutput *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput       *output_;
		StateMap           stateMap_;
		CondVec           *currentState_;
	};

	template <class T, uint32_t Type = 0>
	class AggrLitPrinter : public AggrLit::Printer<T>, public DelayedPrinter
	{
		typedef AggrCond::Printer::State State;
		typedef boost::tuples::tuple<State, bool, bool, bool, bool, Val, Val, bool> TodoVal;
		typedef boost::unordered_map<DelayedOutput::Offset, TodoVal> TodoMap;
	public:
		AggrLitPrinter(PlainOutput *output);
		void begin(State state, bool head, bool sign, bool complete, bool set);
		void lower(const Val &l, bool leq);
		void upper(const Val &u, bool leq);
		void end();
		PlainOutput *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
		void finish();
		virtual bool simplify(const Val &weight) = 0;
		virtual void func() = 0;
		virtual ~AggrLitPrinter();
	protected:
		TodoMap            todo_;
		PlainOutput       *output_;
		TodoVal            current_;
		bool               complete_;
	};

	class SumAggrLitPrinter : public AggrLitPrinter<SumAggrLit>
	{
	public:
		SumAggrLitPrinter(PlainOutput *output);
		bool simplify(const Val &weight);
		void func();
	};

	template <uint32_t Type>
	class MinMaxAggrLitPrinter : public AggrLitPrinter<MinMaxAggrLit, Type>
	{
	public:
		MinMaxAggrLitPrinter(PlainOutput *output);
		bool simplify(const Val &weight);
		void func();
	};

	struct MinAggrLitPrinter : MinMaxAggrLitPrinter<MinMaxAggrLit::MINIMUM>
	{
		MinAggrLitPrinter(PlainOutput *output) : MinMaxAggrLitPrinter<MinMaxAggrLit::MINIMUM>(output) { }
	};

	struct MaxAggrLitPrinter : MinMaxAggrLitPrinter<MinMaxAggrLit::MAXIMUM>
	{
		MaxAggrLitPrinter(PlainOutput *output) : MinMaxAggrLitPrinter<MinMaxAggrLit::MAXIMUM>(output) { }
	};


	/*
	class AvgAggrLitPrinter : public AvgAggrLit::Printer
	{
	public:
		AvgAggrLitPrinter(PlainOutput *output) : output_(output) { }
		void begin(bool head, bool sign);
		void weight(const Val &v);
		void lower(int32_t l);
		void upper(int32_t u);
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput       *output_;
		int32_t            upper_;
		int32_t            lower_;
		bool               sign_;
		bool               count_;
		bool               hasUpper_;
		bool               hasLower_;
		bool               printedLit_;
		std::ostringstream aggr_;
	};

	class ParityAggrLitPrinter : public ParityAggrLit::Printer
	{
	public:
		ParityAggrLitPrinter(PlainOutput *output) : output_(output) { }
		void begin(bool head, bool sign, bool even, bool set);
		void print(PredLitRep *l);
		void weight(const Val &v);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput       *output_;
		bool               even_;
		bool               sign_;
		bool               set_;
		bool               printedLit_;
		std::ostringstream aggr_;
	};
	*/

	class JunctionLitPrinter : public JunctionLit::Printer, public DelayedPrinter
	{
		typedef std::pair<uint32_t, uint32_t> CondKey;
		typedef std::map<CondKey, std::string> CondMap;
		typedef std::pair<DelayedOutput::Offset, CondKey> TodoValue;
		typedef std::vector<TodoValue> TodoVec;
	public:
		JunctionLitPrinter(PlainOutput *output);
		void beginHead(bool disjunction, uint32_t uidJunc, uint32_t uidSubst);
		void beginBody();
		void printCond(bool bodyComplete);
		void printJunc(bool disjunction, uint32_t juncUid, uint32_t substUid);
		void print(PredLitRep *l);
		void finish();
		PlainOutput *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
		std::string *current_;
		CondMap      condMap_;
		TodoVec      todo_;
		bool         body_;
	};

	class OptimizePrinter : public Optimize::Printer
	{
	public:
		OptimizePrinter(PlainOutput *output) : output_(output) { }
		void begin(Optimize::Type type, bool maximize);
		void print(const ValVec &set);
		void print(PredLitRep *l);
		void end();
		PlainOutput *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput   *output_;
		Optimize::Type type_;
		bool           maximize_;
		bool           first_;
		Val            weight_;
		Val            prio_;
	};

	class ComputePrinter : public Compute::Printer
	{
	public:
		ComputePrinter(PlainOutput *output) : output_(output) { }
		void print(PredLitRep *l);
		PlainOutput *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
	};

	class IncPrinter : public IncLit::Printer
	{
	public:
		IncPrinter(PlainOutput *output) : output_(output) {  }
		void print(PredLitRep *l) { (void)l; }
		Output *output() const { return output_; }
	private:
		PlainOutput *output_;
	};

}
