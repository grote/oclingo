// Copyright (c) 2010, Arne König <arkoenig@uni-potsdam.de>
// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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

#include "gringo/gringo_options.h"
#include <program_opts/value.h>

using namespace ProgramOptions;
using namespace std;

bool parsePositional(const std::string& t, std::string& out)
{
	int num;
	if (ProgramOptions::parseValue(t, num, 1)) { out = "number"; }
	else                                       { out = "file";   }
	return true;
}

GringoOptions::GringoOptions()
	: smodelsOut(false)
	, textOut(false)
	, metaOut(false)
	, groundOnly(false)
	, ifixed(1)
	, ibase(false)
	, groundInput(false)
	, disjShift(false)
	, compat(false)
{ }

void GringoOptions::initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden)
{
	(void)hidden;
	OptionGroup gringo("Gringo Options");

	gringo.addOptions()
		("const,c"  , storeTo(consts)->setComposing(), "Replace constant <c> by value <v>\n", "<c>=<v>")
		("dep-graph", storeTo(depGraph),               "Dump program dependency graph to file", "<file>")
		("text,t"   , bool_switch(&textOut),           "Print plain text format")
		("reify"    , bool_switch(&metaOut),           "Print reified text format")
		("lparse,l" , bool_switch(&smodelsOut),        "Print Lparse format")
		("compat"   , bool_switch(&compat),            "Improve compatibility with lparse")
		("ground,g" , bool_switch(&groundInput),       "Enable lightweight mode for ground input")
		("shift"    , bool_switch(&disjShift),         "Shift disjunctions into the body")
		("ifixed"   , storeTo(ifixed),                 "Fix number of incremental steps to <num>", "<num>")
		("ibase"    , bool_switch(&ibase),             "Process base program only")
	;
	OptionGroup basic("Basic Options");
	root.addOptions(gringo);
	root.addOptions(basic, true);
}

bool GringoOptions::validateOptions(ProgramOptions::OptionValues& values, Messages& m)
{
	(void)values;
	int out = smodelsOut + textOut + metaOut;
	if (out > 1)
	{
		m.error = "multiple outputs defined";
		return false;
	}
	else if (out == 0)
	{
		groundOnly = false;
		smodelsOut = true;
	}
	else groundOnly = true;
	return true;
}

void GringoOptions::addDefaults(std::string& def)
{
	def += "  --lparse\n";
}
