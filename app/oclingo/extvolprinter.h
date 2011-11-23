// Copyright (c) 2011, Torsten Grote <tgrote@uni-potsdam.de>
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

#include <gringo/printer.h>
#include <gringo/lparseconverter.h>
#include <gringo/rule.h>
#include "src/output/lparserule_impl.h"

class ExtBasePrinter : public Printer
{
public:
	using ::Printer::print;
	virtual void print() { }
	virtual void printTimeDecay(int window) { (void) window;}
	virtual void printAssert(Val term) { (void) term; }
	virtual ~ExtBasePrinter() { }
};

class ExtVolPrinter : public ExtBasePrinter
{
public:
	ExtVolPrinter(LparseConverter *output) : output_(output) {  }
	void print(PredLitRep *l) { (void) l; }
	void print();
	void printTimeDecay(int window);
	void printAssert(Val term);
	Output *output() const { return output_; }
private:
	LparseConverter *output_;
};
