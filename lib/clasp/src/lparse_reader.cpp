// 
// Copyright (c) 2006-2007, Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#include <clasp/include/lparse_reader.h>
#include <clasp/include/clause.h>
#include <clasp/include/solver.h>
namespace Clasp { 

/////////////////////////////////////////////////////////////////////////////////////////
// StreamSource
/////////////////////////////////////////////////////////////////////////////////////////

// skips horizontal white-space
void StreamSource::skipWhite() {
	while ( **this == ' ' || **this == '\t' ) {
		++*this;
	}
}

// reads a base-10 integer
// Pre: system uses ASCII
bool StreamSource::parseInt( int& val) {
	val = 0;
	bool  pos = true;
	skipWhite();
	if (**this == '-') {
		pos = false;
		++*this;
	}
	if (**this == '+') {
		++*this;
	}
	bool ok = **this >= '0' && **this <= '9';
	while (**this >= '0' && **this <= '9') {
		val *= 10;
		val += **this - '0';
		++*this;
	}
	val = pos ? val : -val;
	return ok;
}

// works like std::getline
bool StreamSource::readLine( char* buf, unsigned maxSize ) {
	if (maxSize == 0) return false;
	for (;;) {
		char ch = **this;
		++*this;
		if ( ch == '\0' || ch == '\n' || --maxSize == 0) break;
		*buf++ = ch;
	}
	*buf = 0;
	return maxSize != 0;
}



LparseReader::LparseReader()
	: source_(0)
	, api_(0) {
}

LparseReader::~LparseReader() {
	clear();
}

void LparseReader::clear() {
	rule_.clear();
	api_  = 0;
}

bool LparseReader::parse(std::istream& prg, ProgramBuilder& api) {
	clear();
	api_ = &api;
	if (!prg) {
		throw ReadError(0, "Could not read from stream!");
	}
	StreamSource source(prg);
	source_ = &source;
	return readRules()
		&& readSymbolTable()
		&& readComputeStatement()
		&& readModels()
		&& endParse();
}

Var LparseReader::parseAtom() {
	int r = -1;
	if (!source_->parseInt(r) || r < 1 || r > (int)varMax) {
		throw ReadError(source_->line(), (r == -1 ? "Atom id expected!" : "Atom out of bounds"));
	}
	return static_cast<Var>(r);
}

bool LparseReader::readRules() {
	int rt = -1;
	while ( skipAllWhite(*source_) && source_->parseInt(rt) && rt != 0 && readRule(rt) ) ;
	if (rt != 0) {
		throw ReadError(source_->line(), "Rule type expected!");
	}
	if (!match(*source_, '\n', true)) {
		throw ReadError(source_->line(), "Symbol table expected!");
	}
	return skipAllWhite(*source_);
}

bool LparseReader::readRule(int rt) {
	int bound = -1;
	if (rt <= 0 || rt > 6 || rt == 4) {
		throw ReadError(source_->line(), "Unsupported rule type!");
	}
	RuleType type(static_cast<RuleType>(rt));
	rule_.setType(type);
	if ( type == BASICRULE || rt == CONSTRAINTRULE || rt == WEIGHTRULE) {
		rule_.addHead(parseAtom());
		if (rt == WEIGHTRULE && (!source_->parseInt(bound) || bound < 0)) {
			throw ReadError(source_->line(), "Weightrule: Positive weight expected!");
		}
	}
	else if (rt == CHOICERULE) {
		int heads;
		if (!source_->parseInt(heads) || heads < 1) {
			throw ReadError(source_->line(), "Choicerule: To few heads");
		}
		for (int i = 0; i < heads; ++i) {
			rule_.addHead(parseAtom());
		}
	}
	else {
		assert(rt == 6);
		int x;
		if (!source_->parseInt(x) || x != 0) {
			throw ReadError(source_->line(), "Minimize rule: 0 expected!");
		}
	}
	int lits, neg;
	if (!source_->parseInt(lits) || lits < 0) {
		throw ReadError(source_->line(), "Number of body literals expected!");
	}
	if (!source_->parseInt(neg) || neg < 0 || neg > lits) {
		throw ReadError(source_->line(), "Illegal negative body size!");
	}
	if (rt == CONSTRAINTRULE && (!source_->parseInt(bound) || bound < 0)) {
		throw ReadError(source_->line(), "Constraint rule: Positive bound expected!");
	}
	if (bound >= 0) {
		rule_.setBound(static_cast<uint32>(bound));
	}
	return readBody(static_cast<uint32>(lits), static_cast<uint32>(neg), rt >= 5);  
}

bool LparseReader::readBody(uint32 lits, uint32 neg, bool readWeights) {
	for (uint32 i = 0; i != lits; ++i) {
		rule_.addToBody(parseAtom(), i >= neg, 1);
	}
	if (readWeights) {
		for (uint32 i = 0; i < lits; ++i) {
			int w;
			if (!source_->parseInt(w) || w < 0) {
				throw ReadError(source_->line(), "Weight Rule: bad or missing weight!");
			}
			rule_.body[i].second = toWeight(w);
		}
	} 
	api_->addRule(rule_);
	rule_.clear();
	return match(*source_, '\n', true) ? true : throw ReadError(source_->line(), "Illformed rule body!");
}

bool LparseReader::readSymbolTable() {
	int a = -1;
	while (source_->parseInt(a) && a != 0) {
		if (a < 1) {
			throw ReadError(source_->line(), "Symbol Table: Atom id out of bounds!");
		}
		char buffer[1024];
		source_->skipWhite();
		if (!source_->readLine(buffer, 1024)) {
			throw ReadError(source_->line(), "Symbol Table: Atom name too long or end of file!");
		}
		api_->setAtomName(a, buffer);
		skipAllWhite(*source_);
	}
	if (a != 0) {
		throw ReadError(source_->line(), "Symbol Table: Atom id expected!");
	}
	if (!match(*source_, '\n', true)) {
		throw ReadError(source_->line(), "Compute Statement expected!");
	}
	return skipAllWhite(*source_);
}

bool LparseReader::readComputeStatement() {
	char pos[2] = { '+', '-' };
	for (int i = 0; i != 2; ++i) {
		char sec[3];
		skipAllWhite(*source_);
		if (!source_->readLine(sec, 3) || sec[0] != 'B' || sec[1] != pos[i]) {
			throw ReadError(source_->line(), (i == 0 ? "B+ expected!" : "B- expected!"));
		}
		skipAllWhite(*source_);
		int id = -1;
		while (source_->parseInt(id) && id != 0) {
			if (id < 1) throw ReadError(source_->line(), "Compute Statement: Atom out of bounds");  
			api_->setCompute(static_cast<Var>(id), pos[i] == '+');
			if (!match(*source_, '\n', true)) {
				throw ReadError(source_->line(), "Newline expected!");
			}
			skipAllWhite(*source_);
		}
		if (id != 0) {
			throw ReadError(source_->line(), "Compute Statement: Atom id or 0 expected!");
		}
		if (!match(*source_, '\n', true)) {
			throw ReadError(source_->line(), (i == 0 ? "B- expected!" : "Number of models expected!"));
		}
	}
	return skipAllWhite(*source_);
}

bool LparseReader::readModels() {
	int m;
	if (!source_->parseInt(m) || m < 0) {
		throw ReadError(source_->line(), "Number of models expected!");
	}
	return skipAllWhite(*source_);
}

bool LparseReader::endParse() {
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// parse dimacs
/////////////////////////////////////////////////////////////////////////////////////////
bool parseDimacs(std::istream& prg, Solver& s, bool assertPure) {
	LitVec currentClause;
	ClauseCreator nc(&s);
	SatPreprocessor* p = 0;
	int numVars = -1, numClauses = 0;
	bool ret = true;
	StreamSource in(prg);
	
	// For each var v: 0000p1p2c1c2
	// p1: set if v occurs negatively in any clause
	// p2: set if v occurs positively in any clause
	// c1: set if v occurs negatively in the current clause
	// c2: set if v occurs positively in the current clause
	PodVector<uint8>::type flags;
	for (;ret;) {
		skipAllWhite(in);
		if (*in == 0) {
			break;
		}
		else if (*in == 'p') {
			if (match(in, "p cnf", false)) {
				if (!in.parseInt(numVars) || !in.parseInt(numClauses) ) {
					throw ReadError(in.line(), "Bad parameters in the problem line!");
				}
				s.reserveVars(numVars+1);
				flags.resize(numVars+1);
				for (int v = 1; v <= numVars; ++v) {
					s.addVar(Var_t::atom_var);
				}
				// HACK: Don't waste time preprocessing a gigantic problem
				if (numClauses > 4000000) { p = s.strategies().satPrePro.release(); }
				s.startAddConstraints();
			}
			else {
				throw ReadError(in.line(), "'cnf'-format expected!");
			}
		} 
		else if (*in == 'c' || *in == 'p') {
			skipLine(in);
		}
		else if (numVars != -1) { // read clause
			int lit;
			Literal rLit;
			bool sat = false;
			nc.start();
			currentClause.clear();
			for (;;){
				if (!in.parseInt(lit)) {
					throw ReadError(in.line(), "Bad parameter in clause!");
				}
				else if (abs(lit) > numVars) {
					throw ReadError(in.line(), "Unrecognized format - variables must be numbered from 1 up to $VARS!");
				}
				skipAllWhite(in);
				rLit = lit >= 0 ? posLit(lit) : negLit(-lit);
				if (lit == 0) {
					for (LitVec::iterator it = currentClause.begin(); it != currentClause.end(); ++it) {
						flags[it->var()] &= ~3u; // clear "in clause"-flags
						if (!sat) { 
							// update "in problem"-flags
							flags[it->var()] |= ((1 + it->sign()) << 2);
						}
					}
					ret = sat || nc.end();
					break;
				}
				else if ( (flags[rLit.var()] & (1+rLit.sign())) == 0 ) {
					flags[rLit.var()] |= 1+rLit.sign();
					nc.add(rLit);
					currentClause.push_back(rLit);
					if ((flags[rLit.var()] & 3u) == 3u) sat = true;
				}
			}
		}
		else {
			throw ReadError(in.line(), "Unrecognized format - missing problem line!");
		}
	}
	if (p) s.strategies().satPrePro.reset(p);
	for (Var i = 1; ret && i <= s.numVars(); ++i) {
		uint8 d = (flags[i]>>2);
		if      (d == 0)                { ret = s.force(negLit(i), 0); }
		else if (d != 3 && assertPure)  { ret = s.force(Literal(i, d != 1), 0); }
	}
	return ret;
}
}
