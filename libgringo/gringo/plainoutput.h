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
#include <gringo/output.h>

class DelayedOutput
{
public:
	typedef boost::function2<void, std::ostream &, const std::string &> LineCallback;
	typedef std::pair<uint32_t, uint32_t>                               Offset;

private:
	struct Line
	{
		Line();
		void clear();

		StringVec    slices;
		LineCallback cb;
		uint32_t     finished;
	};

	typedef std::vector<Offset>             OffsetVec;
	typedef std::vector<Line>               LineVec;
	typedef std::vector<Offset::first_type> FreeVec;

public:
	DelayedOutput(std::ostream &out);
	Offset beginDelay();
	void contDelay(const Offset &idx);
	void endDelay(const Offset &idx);
	void setLineCallback(const LineCallback &cb);
	void endLine();
	std::ostream &buffer();
	std::ostream &output();

private:
	Offset::first_type curLine_;
	std::ostream      &out_;
	std::stringstream  buf_;
	LineVec            lines_;
	FreeVec            free_;
};

class PlainOutput : public Output
{
public:
	PlainOutput(std::ostream &out);

	std::ostream &out();
	DelayedOutput::Offset beginDelay();
	void contDelay(const DelayedOutput::Offset &idx);
	void endDelay(const DelayedOutput::Offset &idx);

	void beginRule();
	void endHead();
	void print();
	void endRule();
	void endStatement();

	void print(PredLitRep *l, std::ostream &out);
	void finalize();
	void doShow(bool s);
	void doShow(uint32_t nameId, uint32_t arity, bool s);
	void addCompute(PredLitRep *l);
	~PlainOutput();
private:
	bool              head_;
	bool              printedHead_;
	bool              printedBody_;
	DelayedOutput     out_;
	std::stringstream compute_;

};

