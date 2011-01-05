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
	, in_(in)
	, parser_(onlineparserAlloc(malloc))
	, error_(false)
	, output_(output)
	, terminated_(false)
	, got_step_(false)
{ }

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
	// stop at ENDSTEP because socket stream will not return EOI
	while(token != OPARSER_ENDSTEP && !terminated_);
}

void OnlineParser::parse()
{
	parse(*in_);
	if(error_)
	{
		ParseException ex;
		foreach(ErrorTok &tok, errors_) {
			std::cerr << "THROW ERROR!! ";
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

PredLitRep *OnlineParser::predLitRep(GroundProgramBuilder::StackPtr &stack, Lit &a)
{
	Domain *dom = storage()->domain(stack->vals[a.offset].index, a.n);
	lit_.dom_  = dom;
	lit_.sign_ = a.sign;
	lit_.vals_.resize(a.n);
	std::copy(stack->vals.begin() + a.offset + 1, stack->vals.begin() + a.offset + 1 + a.n, lit_.vals_.begin());
	return &lit_;
}

void OnlineParser::add(Type type, uint32_t n) {
	switch(type)
	{
		case STM_RULE:
		{
			GroundProgramBuilder::StackPtr stack = get(type, n);

			LitVec &litVec = stack->lits;
			Lit &lit = litVec.at(litVec.size() - n - 1);
			uint32_t head = dynamic_cast<LparseConverter*>(output_)->symbol(predLitRep(stack, lit));

			if(output_->getExternalKnowledge().checkHead(head)) {
				output_->getExternalKnowledge().addHead(head);
				GroundProgramBuilder::add(stack);
			} else {
				std::stringstream emsg;
				emsg << "Warning: Head of rule added in line " << token_.line;
				emsg << " has not been declared external. The entire rule will be ignored.";
				std::cerr << emsg.str() << std::endl;
				output_->getExternalKnowledge().sendToClient(emsg.str());
			}
			break;
		}
		default:
		{
			GroundProgramBuilder::add(type, n);
		}
	}
}

void OnlineParser::setStep(int step) {
	if(got_step_) {
		std::stringstream warning_msg;
		warning_msg << "Warning: New '#step " << step << ".' without prior '#endstep.' encountered. Ignoring....\n";

		std::cerr << warning_msg.str() << std::endl;
		output_->getExternalKnowledge().sendToClient(warning_msg.str());
	} else {
		got_step_ = true;
		output_->getExternalKnowledge().setControllerStep(step);
	}
}

void OnlineParser::forget(int step) { (void) step; }

void OnlineParser::terminate() {
	terminated_ = true;
}

void OnlineParser::setCumulative() { }
void OnlineParser::setVolatile() { }

bool OnlineParser::isTerminated() {
	return terminated_;
}

