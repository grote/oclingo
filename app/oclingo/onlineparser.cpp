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
	, volatile_(false)
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

void OnlineParser::doAdd() {
	if(stack_->type == EXT_VOLATILE_RULE or stack_->type == EXT_VOLATILE_CONSTRAINT) {
		std::cerr << "++++ ADD EXT VOL" << std::endl;
		// TODO change type to uint16_t and use enum Type USER+1 for new type

		Rule::Printer *printer = output_->printer<Rule::Printer>();
		printer->begin();
		if(stack_->type == EXT_VOLATILE_RULE)             { printLit(printer, stack_->lits.size() - stack_->n - 1, true); }
		printer->endHead();
		for(uint32_t i = stack_->n; i >= 1; i--) { printLit(printer, stack_->lits.size() - i, false); }
		// TODO add volatileAtom symbol() here
		printer->end();
		GroundProgramBuilder::pop(stack_->n + (stack_->type == STM_RULE));
	}
}

void OnlineParser::add(StackPtr stm) {
	std::cerr << "ADDING RULE FROM STACK" << std::endl;
	output_->startExtInput();
	GroundProgramBuilder::add(stm);
	output_->stopExtInput();
}

void OnlineParser::add(Type type, uint32_t n) {
	switch(type)
	{
		case STM_RULE:
		case EXT_VOLATILE_RULE:
		{
			StackPtr stack = get(type, n);

			ExternalKnowledge& ext = output_->getExternalKnowledge();

			if(ext.needsNewStep()) {
				std::cerr << "NEED NEW STEP BEFORE ADDING RULE. ADDING TO STACK" << std::endl;
				ext.addStackPtr(stack);
			} else {
				std::cerr << "ADDING RULE/HEAD!!!" << std::endl;
				output_->startExtInput();
				GroundProgramBuilder::add(stack);
				output_->stopExtInput();
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

void OnlineParser::forget(int step) {
	output_->getExternalKnowledge().forgetExternals(step);
}

void OnlineParser::terminate() {
	terminated_ = true;
}

void OnlineParser::setCumulative() {
	volatile_ = false;
}

void OnlineParser::setVolatile() {
	volatile_ = true;
}

bool OnlineParser::isTerminated() {
	return terminated_;
}

