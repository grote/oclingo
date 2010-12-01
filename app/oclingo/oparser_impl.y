// Copyright (c) 2010, Torsten Grote <tgrote@uni-potsdam.de>
// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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

%include {
#include "gringo/gringo.h"
#include "oparser_impl.h"
#include "oclingo/olexer.h"
#include "gringo/converter.h"
#include "gringo/storage.h"

#define ONE   Val::create(Val::NUM, 1)
#define UNDEF Val::create()
#define PRIO  Val::create(Val::NUM, onlineParser->level())

}

%name onlineParser
%stack_size       0
%parse_failure    { onlineParser->parseError(); }
%syntax_error     { onlineParser->syntaxError(); }
%extra_argument   { OnlineParser *onlineParser }
%token_type       { OnlineParser::Token }
%token_destructor { (void)onlineParser; (void)$$; }
%token_prefix     OPARSER_
%start_symbol     start

%type body           { uint32_t }
%type nbody          { uint32_t }
%type termlist       { uint32_t }
%type nprio_list     { uint32_t }
%type prio_list      { uint32_t }
%type nprio_set      { uint32_t }
%type prio_set       { uint32_t }
%type head_ccondlist { uint32_t }
%type nweightlist    { uint32_t }
%type weightlist     { uint32_t }
%type nnumweightlist { uint32_t }
%type numweightlist  { uint32_t }
%type ncondlist      { uint32_t }
%type condlist       { uint32_t }

start ::= program.

program ::= .
program ::= program line DOT.

// line ::= fact.
line ::= rule.
line ::= STEP.// NUMBER(n).	//{ onlineParser->setStep(atol(n->c_str())); }
line ::= ENDSTEP DOT.
line ::= FORGET.// NUMBER(n).	//{ onlineParser->forget(atol(n->c_str())); }
line ::= STOP.				//{ onlineParser->terminate(); }
line ::= CUMULATIVE.		//{ onlineParser->setCumulative(); }
line ::= VOLATILE.			//{ onlineParser->setVolatile(); }

id ::= IDENTIFIER(id). { onlineParser->addSigned(id.index, false); }

// fact ::= predicate(head) IF.	{ pParser->addFact(head); }
// fact ::= predicate(head).		{ pParser->addFact(head); }
// rule ::= predicate(head) IF body(body).	{ Rule r(head, body); pParser->addRule(&r); }
// rule ::= IF body(body).					{ Integrity r(body);  pParser->addIntegrity(&r); }

rule ::= predicate IF body(n). { onlineParser->add(Converter::STM_RULE, n); }
rule ::= IF body(n).      { onlineParser->add(Converter::STM_CONSTRAINT, n); }
rule ::= predicate.            { onlineParser->add(Converter::STM_RULE, 0); }

nbody(res) ::= predlit.                { res = 1; }
nbody(res) ::= nbody(n) COMMA predlit. { res = n + 1; }
body(res) ::= .         { res = 0; }
body(res) ::= nbody(n). { res = n; }

predlit ::= predicate.
predlit ::= NOT predicate. { onlineParser->addSign(); }

predicate ::= id LBRAC termlist(n) RBRAC. { onlineParser->add(Converter::LIT, n); }
predicate ::= id.                         { onlineParser->add(Converter::LIT, 0); }
predicate ::= MINUS IDENTIFIER(id) LBRAC termlist(n) RBRAC. { onlineParser->addSigned(id.index, true); onlineParser->add(Converter::LIT, n); }
predicate ::= MINUS IDENTIFIER(id).                         { onlineParser->addSigned(id.index, true); onlineParser->add(Converter::LIT, 0); }

termlist(res) ::= term.                   { res = 1; }
termlist(res) ::= termlist(n) COMMA term. { res = n + 1; }

term ::= id.                                       { onlineParser->add(Converter::TERM, 0); }
term ::= string.                                   { onlineParser->add(Converter::TERM, 0); }
term ::= empty LBRAC termlist(n) COMMA term RBRAC. { onlineParser->add(Converter::TERM, n+1); }
term ::= id LBRAC termlist(n) RBRAC.               { onlineParser->add(Converter::TERM, n); }
term ::= numterm.

numterm ::= number.   { onlineParser->add(Converter::TERM, 0); }
string    ::= STRING(id).        { onlineParser->addVal(Val::create(Val::ID, id.index)); }
empty     ::= .                  { onlineParser->addVal(Val::create(Val::ID, onlineParser->storage()->index(""))); }
number    ::= MINUS NUMBER(num). { onlineParser->addVal(Val::create(Val::NUM, -num.number)); }
number    ::= posnumber.
posnumber ::= NUMBER(num).       { onlineParser->addVal(Val::create(Val::NUM, num.number)); }
