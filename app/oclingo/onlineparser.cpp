// Copyright (c) 2010, Torsten Grote <tgrote@uni-potsdam.de>
// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

#include "onlineparser.h"
#include "onlineparser_impl.h"
#include <gringo/rule.h>
#include <oclingo/oclaspoutput.h>
#include <gringo/storage.h>
#include <gringo/exceptions.h>
#include <gringo/streams.h>

void *onlineparserAlloc(void *(*mallocProc)(size_t));
void onlineparserFree(void *p, void (*freeProc)(void*));
void onlineparser(void *yyp, int yymajor, OnlineParser::Token yyminor, OnlineParser *oParser);

OnlineParser::OnlineParser(oClaspOutput *output, std::istream* in)
	: GroundProgramBuilder(output)
	, parser_(onlineparserAlloc(malloc))
	, error_(false)
	, terminated_(false)
{
	in_ = in;
}

OnlineParser::~OnlineParser()
{
	onlineparserFree(parser_, free);
}


void OnlineParser::parseError()
{
	error_ = true;
}

std::string OnlineParser::errorToken()
{
	if(eof()) return "<EOF>";
	else return string();
}

void OnlineParser::syntaxError()
{
	errors_.push_back(ErrorTok(token_.loc(), errorToken()));
	error_ = true;
}

void OnlineParser::parse(std::istream &in)
{
	token_.file = storage()->index("[controller socket]");
	reset(&in);
	int token;
	do
	{
		token = lex();
		onlineparser(parser_, token, token_, this);
	}
	//while(token != 0);
	// stop at ENDSTEP because socket stream will not return EOI
	while(token != OPARSER_ENDSTEP);
}

void OnlineParser::parse()
{
	parse(*in_);
	if(error_)
	{
		ParseException ex;
		foreach(ErrorTok &tok, errors_) {
			std::cerr << "THROW ERROR!! ";
			std::cerr << "lits_.size(): " << lits_.size();
			std::cerr << " vals_.size(): " << vals_.size();
			ex.add(StrLoc(storage(), tok.first), tok.second);
		}
		throw ex;
	}
}

void OnlineParser::addSigned(uint32_t index, bool sign)
{
	if(sign) { index = storage()->index(std::string("-") + storage()->string(index)); }
	addVal(Val::create(Val::ID, index));
}


void OnlineParser::add(Type type, uint32_t n)
{
	switch(type)
	{
		case LIT:
		{
			lits_.push_back(Lit::create(type, vals_.size() - n - 1, n));
			break;
		}
		case TERM:
		{
			if(n > 0)
			{
				ValVec vals;
				std::copy(vals_.end() - n, vals_.end(), std::back_inserter(vals));
				vals_.resize(vals_.size() - n);
				uint32_t name = vals_.back().index;
				vals_.back()  = Val::create(Val::FUNC, storage()->index(Func(storage(), name, vals)));
			}
			break;
		}
		case STM_RULE:
		case STM_CONSTRAINT:
		{
			// TODO add Printer
			/*Rule::Printer *printer = output_->printer<Rule::Printer>();
			printer->begin();
			if(type == STM_RULE)             { printLit(printer, lits_.size() - n - 1, true); }
			printer->endHead();
			for(uint32_t i = n; i >= 1; i--) { printLit(printer, lits_.size() - i, false); }
			printer->end();
			*/GroundProgramBuilder::pop(n + (type == STM_RULE));
			break;
		}
		default:
			break;
	}
}


void OnlineParser::setStep(int step) { (void) step; }
void OnlineParser::forget(int step) { (void) step; }
void OnlineParser::terminate() {
	terminated_ = true;
}
void OnlineParser::setCumulative() { }
void OnlineParser::setVolatile() { }

bool OnlineParser::isTerminated() {
	return terminated_;
}

