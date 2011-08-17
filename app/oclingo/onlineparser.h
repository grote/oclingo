// Copyright (c) 2010, Torsten Grote <tgrote@uni-potsdam.de>
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
#include <gringo/parser.h>
#include <gringo/groundprogrambuilder.h>
#include <gringo/lexer_impl.h>
#include <gringo/locateable.h>
#include "extvolprinter.h"

class oClaspOutput;

class OnlineParser : public LexerImpl, public GroundProgramBuilder
{
private:
	typedef std::pair<Loc,std::string> ErrorTok;
	typedef std::vector<ErrorTok> ErrorVec;
public:
	struct Token
	{
		Loc loc() const { return Loc(file, line, column); }
		uint32_t file;
		uint32_t line;
		uint32_t column;
		union
		{
			int      number;
			uint32_t index;
		};
	};

public:
	OnlineParser(oClaspOutput *output, std::istream* in);
	int lex();
	std::string errorToken();
	void syntaxError();
	void parseError();
	void parse();
	void addSigned(uint32_t index, bool sign);

	void add(StackPtr stm);
	void add(Type type, uint32_t n = 0);

	void setStep(int step);
	void forget(int step);
	void terminate();
	void setCumulative();
	void setVolatile();
	void setVolatileWindow(int window);

	bool isTerminated();

	~OnlineParser();

private:
	void parse(std::istream &sin);
	void doAdd();

private:
	std::istream* in_;
	void*         parser_;
	Token         token_;
	bool          error_;
	ErrorVec      errors_;
	Literal       lit_;

	oClaspOutput* output_;
	bool terminated_;
	bool got_step_;
	bool volatile_;
	int volatile_window_;
};

