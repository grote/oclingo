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

#include <gringo/plainoutput.h>
#include <gringo/domain.h>
#include <gringo/storage.h>
#include <gringo/predlitrep.h>

namespace plainoutput_impl
{

GRINGO_EXPORT_PRINTER(DisplayPrinter)
GRINGO_EXPORT_PRINTER(ExternalPrinter)
GRINGO_EXPORT_PRINTER(RulePrinter)
GRINGO_EXPORT_PRINTER(SumAggrLitPrinter)
GRINGO_EXPORT_PRINTER(AggrCondPrinter)
GRINGO_EXPORT_PRINTER(MinAggrLitPrinter)
GRINGO_EXPORT_PRINTER(MaxAggrLitPrinter)
/*
GRINGO_EXPORT_PRINTER(AvgAggrLitPrinter)
GRINGO_EXPORT_PRINTER(MinMaxAggrLitPrinter)
GRINGO_EXPORT_PRINTER(ParityAggrLitPrinter)
*/
GRINGO_EXPORT_PRINTER(JunctionLitPrinter)
GRINGO_EXPORT_PRINTER(OptimizePrinter)
GRINGO_EXPORT_PRINTER(ComputePrinter)
GRINGO_EXPORT_PRINTER(IncPrinter)

}

////////////////////////////////// DelayedOutput::Line //////////////////////////////////

DelayedOutput::Line::Line()
	: finished(0)
{ }

void DelayedOutput::Line::clear()
{
	slices.clear();
	cb       = 0;
	finished = 0;
}

////////////////////////////////// DelayedOutput //////////////////////////////////

DelayedOutput::DelayedOutput(std::ostream &out)
	: curLine_(0)
	, out_(out)
	, lines_(1)
{
}

DelayedOutput::Offset DelayedOutput::beginDelay()
{
	Line &line = lines_[curLine_];
	uint32_t d = line.slices.size();
	line.slices.push_back(buf_.str());
	buf_.str("");
	return Offset(curLine_, d);
}

void DelayedOutput::contDelay(const Offset &idx)
{
	Line &line = lines_[idx.first];
	line.slices[idx.second] += buf_.str();
	buf_.str("");
}

void DelayedOutput::endDelay(const Offset &idx)
{
	Line &line = lines_[idx.first];
	line.finished++;
	if(line.finished == line.slices.size())
	{
		if(line.cb.empty())
		{
			foreach(std::string &s, line.slices) { out_ << s; }
		}
		else
		{
			std::string str;
			foreach(std::string &s, line.slices) { str+= s; }
			line.cb(str);
		}
		line.clear();
		free_.push_back(idx.first);
	}
}

void DelayedOutput::setLineCallback(const LineCallback &cb)
{
	lines_[curLine_].cb = cb;
}

void DelayedOutput::endLine()
{
	Line &line = lines_[curLine_];
	if(line.slices.empty())
	{
		if(line.cb.empty()) { out_ << buf_.str(); }
		else                { line.cb(buf_.str()); }
		line.clear();
	}
	else
	{
		line.finished++;
		line.slices.push_back(buf_.str());
		if(free_.empty())
		{
			curLine_ = lines_.size();
			lines_.push_back(Line());
		}
		else
		{
			curLine_ = free_.back();
			free_.pop_back();
		}
	}
	buf_.str("");
}

std::ostream &DelayedOutput::buffer() { return buf_; }
std::ostream &DelayedOutput::output() { return out_; }

////////////////////////////////// PlainOutput //////////////////////////////////

PlainOutput::PlainOutput(std::ostream &out)
	: out_(out)
	, optimize_(boost::bind(static_cast<int (Val::*)(const Val&, Storage *) const>(&Val::compare), _1, _2, boost::ref(s_)) < 0)
{
	initPrinters<PlainOutput>();
}

void PlainOutput::beginRule()
{
	head_        = true;
	printedHead_ = false;
}

void PlainOutput::endHead()
{
	head_        = false;
	printedBody_ = false;
}

#pragma message "something has to be done about that!"
void PlainOutput::print()
{
	if(head_)
	{
		if(printedHead_) { out() << "|"; }
		printedHead_ = true;
	}
	else
	{
		if(printedBody_) { out() << ","; }
		else             { out() << ":-"; }
		printedBody_ = true;
	}
}

std::ostream &PlainOutput::out()
{
	return out_.buffer();
}

DelayedOutput::Offset PlainOutput::beginDelay()
{
	return out_.beginDelay();
}

void PlainOutput::contDelay(const DelayedOutput::Offset &idx)
{
	out_.contDelay(idx);
}

void PlainOutput::endDelay(const DelayedOutput::Offset &idx)
{
	out_.endDelay(idx);
}

void PlainOutput::setLineCallback(const DelayedOutput::LineCallback &cb)
{
	out_.setLineCallback(cb);
}

void PlainOutput::endRule()
{
	if(!printedHead_ && !printedBody_) { out() << ":-"; }
	out() << ".\n";
	endStatement();
}

void PlainOutput::endStatement()
{
	out_.endLine();
}

void PlainOutput::addOptimize(const Val &optnum, bool maximize, bool multi, const std::string &s)
{
	std::string &str = optimize_.insert(OptimizeMap::value_type(optnum, OptimizeStr(maximize, multi, ""))).first->second.get<2>();
	if(!str.empty()) { str+= ","; }
	str+= s;
}

void PlainOutput::print(PredLitRep *l, std::ostream &out)
{
	if(l->sign()) {  out << "not "; }
	out << storage()->string(l->dom()->nameId());
	if(l->vals().size() > 0)
	{
		out << "(";
		l->vals().front().print(storage(), out);
		for(ValVec::const_iterator i = l->vals().begin() + 1; i != l->vals().end(); ++i)
		{
			out << ",";
			i->print(storage(), out);
		}
		out << ")";
	}
}

void PlainOutput::finalize()
{
	Output::finalize();
	foreach(OptimizeMap::value_type &ref, optimize_)
	{
		out_.output()
			<< (ref.second.get<0>() ? "#maximize" : "#minimize")
			<< (ref.second.get<1>() ? "[" : "{")
			<< ref.second.get<2>()
			<< (ref.second.get<1>() ? "]" : "}")
			<< ".\n";
	}
	foreach(Domain *dom, storage()->domains())
	{
		if (dom->external())
		{
			const std::string &name  = s_->string(dom->nameId());
			uint32_t           arity = dom->arity();
			out_.output() << "#external " << name << "/" << arity << ".\n";
		}
	}
	if(!compute_.str().empty())
	{
		out_.output() << "#compute{";
		out_.output() << compute_.str();
		out_.output() << "}.\n";
		compute_.str("");
	}
}

void PlainOutput::doHideAll()
{
	out_.output() << "#hide.\n";
}

void PlainOutput::doShow(uint32_t nameId, uint32_t arity, bool s)
{
	out_.output() << (s ? "#show " : "#hide ") << storage()->string(nameId) << "/" << arity << ".\n";
}

void PlainOutput::forgetStep(int step)
{
	out_.output() << "#forget " << step << ".\n";
}

void PlainOutput::addCompute(PredLitRep *l)
{
	if(!compute_.str().empty()) { compute_ << ","; }
	print(l, compute_);
}

PlainOutput::~PlainOutput()
{
}

