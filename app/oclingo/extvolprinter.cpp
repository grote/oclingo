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

#include "extvolprinter.h"
#include "oclaspoutput.h"

using namespace lparseconverter_impl;

void ExtVolPrinter::print() {
	RulePrinter *printer = static_cast<RulePrinter *>(output_->printer<Rule::Printer>());
	int atom = dynamic_cast<oClaspOutput*>(output_)->getVolAtom();
	if(atom > 0) { printer->addBody(atom, false); }
}

void ExtVolPrinter::printWindow() {
	RulePrinter *printer = static_cast<RulePrinter *>(output_->printer<Rule::Printer>());
	int atom = dynamic_cast<oClaspOutput*>(output_)->getVolAtom();
	if(atom > 0) { printer->addBody(atom, false); }
}

GRINGO_REGISTER_PRINTER(ExtVolPrinter, ExtBasePrinter, LparseConverter)
