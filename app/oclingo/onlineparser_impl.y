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
#include "onlineparser_impl.h"
#include "oclingo/onlineparser.h"
#include "gringo/converter.h"
#include "gringo/storage.h"
}

%name onlineparser
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

start ::= program.

program ::= .
program ::= program line DOT.

line ::= STEP NUMBER(num).   { onlineParser->setStep(num.number); }
line ::= rule.
line ::= CUMULATIVE.         { onlineParser->setCumulative(); }
line ::= VOLATILE.           { onlineParser->setVolatile(); }
line ::= FORGET NUMBER(num). { onlineParser->forget(num.number); }
line ::= ENDSTEP.
line ::= STOP.               { onlineParser->terminate(); }

id ::= IDENTIFIER(id). { onlineParser->addSigned(id.index, false); }

rule ::= predicate IF body(n). { onlineParser->add(Converter::STM_RULE, n); }
rule ::= IF body(n).           { onlineParser->add(Converter::STM_CONSTRAINT, n); }
rule ::= predicate.            { onlineParser->add(Converter::STM_RULE, 0); }

body(res) ::= .         { res = 0; }
body(res) ::= nbody(n). { res = n; }
nbody(res) ::= predlit.                { res = 1; }
nbody(res) ::= nbody(n) COMMA predlit. { res = n + 1; }

predlit ::= predicate.
predlit ::= NOT predicate. { onlineParser->addSign(); }

predicate ::= id LBRAC termlist(n) RBRAC. { onlineParser->add(Converter::LIT, n); }
predicate ::= id.                         { onlineParser->add(Converter::LIT, 0); }
predicate ::= MINUS IDENTIFIER(id) LBRAC termlist(n) RBRAC. { onlineParser->addSigned(id.index, true); onlineParser->add(Converter::LIT, n); }
predicate ::= MINUS IDENTIFIER(id).                         { onlineParser->addSigned(id.index, true); onlineParser->add(Converter::LIT, 0); }

term ::= id.                                       { onlineParser->add(Converter::TERM, 0); }
term ::= string.                                   { onlineParser->add(Converter::TERM, 0); }
term ::= id LBRAC termlist(n) RBRAC.               { onlineParser->add(Converter::TERM, n); }
term ::= numterm.

termlist(res) ::= term.                   { res = 1; }
termlist(res) ::= termlist(n) COMMA term. { res = n + 1; }

string    ::= STRING(tok).       { onlineParser->addVal(Val::id(tok.index)); }
numterm   ::= number.            { onlineParser->add(Converter::TERM, 0); }
number    ::= MINUS NUMBER(num). { onlineParser->addVal(Val::number(-num.number)); }
number    ::= posnumber.
posnumber ::= NUMBER(num).       { onlineParser->addVal(Val::number(num.number)); }
