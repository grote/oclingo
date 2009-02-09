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
#ifndef CLASP_LPARSE_READER_H_INCLUDED
#define CLASP_LPARSE_READER_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/include/program_builder.h>
#include <istream>
#include <stdexcept>

/*!
 * \file 
 * Defines functions and classes for program input.
 */
namespace Clasp {


//! Reads a CNF in DIMACS-format
/*!
 * \ingroup problem
 * \param prg The stream containing the CNF in DIMACS-format
 * \param s The Solver to use for solving the problem 
 * \param assertPure If true, pure literals are asserted
 */
bool parseDimacs(std::istream& prg, Solver& s, bool assertPure);


//! Wrapps an std::istream and provides basic functions for extracting numbers and strings.
class StreamSource {
public:
	explicit StreamSource(std::istream& is) : in_(is), pos_(0), line_(1) {
		underflow();
	}
	//! returns the character at the current reading-position
	char operator*() {
		if (buffer_[pos_] == 0) {
			underflow();
		}
		return buffer_[pos_];
	}
	//! advances the current reading-position
	StreamSource& operator++() {
		if (buffer_[pos_++] == '\n') {++line_;}
		**this;
		return *this;
	}
	//! returns the line number of the current reading-position
	unsigned line() const { return line_; }
	
	//! reads a base-10 integer
	/*!
	 * \pre system uses ASCII
	 */
	bool parseInt( int& val); 
	//! skips horizontal white-space
	void skipWhite();
	//! works like std::getline
	bool readLine( char* buf, unsigned maxSize );
private:
	StreamSource(const std::istream&);
	StreamSource& operator=(const StreamSource&);
	void underflow() {    
		pos_ = 0;
		buffer_[0] = 0;
		if (!in_) return;
		in_.read( buffer_, sizeof(buffer_)-1 );
		buffer_[in_.gcount()] = 0;
	}
	char buffer_[2048];
	std::istream& in_;
	uint32 pos_;
	uint32 line_;
};

//! skips horizontal and vertical white-space
inline bool skipAllWhite(StreamSource& in) {
	do {
		in.skipWhite();
	} while (*in == '\n' && *++in != 0);
	return true;
}

//! skips the current line
inline void skipLine(StreamSource& in) {
	while (*in && *in != '\n') ++in;
	if (*in) ++in;
}

//! consumes next character if equal to c
/*!
 * \param in StreamSource from which characters should be read
 * \param c character to match
 * \param sw skip leading white-space
 * \return
 *  - true if character c was consumed
 *  - false otherwise
 *  .
 */
inline bool match(StreamSource& in, char c, bool sw) {
	if (sw) in.skipWhite();
	if (*in == c) {
		++in;
		return true;
	}
	return false;
}

//! consumes string str 
/*!
 * \param in StreamSource from which characters should be read
 * \param str string to match
 * \param sw skip leading white-space
 * \return
 *  - true if string str was consumed
 *  - false otherwise
 *  .
 */
inline bool match(StreamSource& in, const char* str, bool sw) {
	if (sw) in.skipWhite();
	for (; *str && *in && *str == *in; ++str, ++in);
	return *str == 0;
}

//! Reads a logic program in lparse-format.
/*!
 * \ingroup problem
 * Reads a logic program in lparse-format and creates an internal represenation of the
 * program using a ProgramBuilder object.
 */
class LparseReader {
public:
	LparseReader();
	~LparseReader();
	
	//! parses the logic program given in prg.
	/*!
	 * Parses the logic program given in prg and creates an internal representation using
	 * the supplied ProgramBuilder.
	 * \param prg The logic program in lparse-format
	 * \param api The ProgramBuilder object to use for program creation.
	 * \pre api.startProgram() was called
	 */
	bool parse(std::istream& prg, ProgramBuilder& api);
private:
	LparseReader(const LparseReader&);
	LparseReader& operator=(const LparseReader&);
	void  clear();
	Var   parseAtom();
	bool  readRules();
	bool  readSymbolTable();
	bool  readComputeStatement();
	bool  readModels();
	bool  endParse();
	bool  readRule(int);
	bool  readBody(uint32 lits, uint32 neg, bool weights);
	PrgRule         rule_;
	StreamSource*   source_;
	ProgramBuilder* api_;
};

//! Instances of this class are thrown if a logic program contains errors.
struct ReadError : public std::runtime_error {
	ReadError(unsigned line, const char* msg) : std::runtime_error(msg), line_(line) {}
	unsigned line_;
};
}
#endif
