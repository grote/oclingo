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
#include "oclingo/externalknowledge.h"

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
	, part_(CUMULATIVE)
	, volatile_window_(0)
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
			ex.add(StrLoc(storage(), tok.first), tok.second);
		}
		throw ex;
	}
}

void OnlineParser::addSigned(uint32_t index, bool sign)
{
	if(sign) { index = storage()->index(std::string("-") + storage()->string(index)); }
	addVal(Val::id(index));
}

void OnlineParser::doAdd() {
	// special printing of volatile rules
	if(stack_->type == USER or stack_->type == USER+1) {
		Rule::Printer *printer = output_->printer<Rule::Printer>();
		printer->begin();
		if(stack_->type == USER) { printLit(printer, stack_->lits.size() - stack_->n - 1, true); }
		printer->endHead();
		for(uint32_t i = stack_->n; i >= 1; i--) { printLit(printer, stack_->lits.size() - i, false); }

		// add volatile atom with special Printer
		ExtBasePrinter *vol_printer = output_->printer<ExtBasePrinter>();
		if(volatile_window_ == 0) {
			// prolog like volatile that is discarded after each query
			vol_printer->print();
		} else {
			// step based volatile
			vol_printer->printWindow(volatile_window_);
		}

		printer->end();
		GroundProgramBuilder::pop(stack_->n + (stack_->type == USER));
	}
}

void OnlineParser::add(StackPtr stm) {
	output_->startExtInput();
	GroundProgramBuilder::add(stm);
	output_->stopExtInput();
}

void OnlineParser::add(Type type, uint32_t n) {
	if(type == STM_RULE) {
		if(part_ == VOLATILE) type = USER;	// set custom rule type
		StackPtr stack = get(type, n);

		if(output_->getExternalKnowledge().needsNewStep()) {
			// add rules in later step, so externals are declared
			output_->getExternalKnowledge().addStackPtr(stack);
			// remember status for volatile rules
			output_->getExternalKnowledge().savePrematureVol(part_ == VOLATILE, volatile_window_);
		} else {
			// add rules right away
			output_->startExtInput();
			GroundProgramBuilder::add(stack);
			output_->stopExtInput();
		}
	}
	else {
		if(type == STM_CONSTRAINT && part_ == VOLATILE) type = USER + 1;	// set custom rule type
		GroundProgramBuilder::add(type, n);
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
	part_ = CUMULATIVE;
}

void OnlineParser::setVolatile() {
	part_ = VOLATILE;
}

void OnlineParser::setVolatileWindow(int window) {
	setVolatile();

	volatile_window_ = window;
}

bool OnlineParser::isTerminated() {
	return terminated_;
}

