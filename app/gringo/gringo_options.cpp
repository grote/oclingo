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
#include <gringo/grounder.h>
#include <gringo/predlit.h>

using namespace ProgramOptions;
using namespace std;

namespace ProgramOptions
{
bool HeuristicOptions::mapHeuristic(const std::string& s, HeuristicOptions &options)
{
	std::string temp = toLower(s);
	if (temp == "basic")      { options.heuristic.reset(new BasicBodyOrderHeuristic()); return true; }
	else if (temp == "unify") { options.heuristic.reset(new UnifyBodyOrderHeuristic()); return true; }
	return false;
}

template<>
bool parseValue(const std::string&s, GringoOptions::IExpand& exp, double)
{
	std::string temp = toLower(s);
	if (temp == "all")        { exp = GringoOptions::IEXPAND_ALL;   return true; }
	else if (temp == "depth") { exp = GringoOptions::IEXPAND_DEPTH; return true; }
	return false;
}

bool parseValue(const std::string&, HeuristicOptions&, int) { return false; }
}

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
	, ifixed(std::numeric_limits<int>::min())
	, iinit(1)
	, groundInput(false)
	, disjShift(false)
	, compat(false)
	, stats(false)
	, magic(false)
	, iexpand(IEXPAND_ALL)
{
	heuristics.heuristic.reset(new BasicBodyOrderHeuristic());
}

TermExpansionPtr GringoOptions::termExpansion(IncConfig &config) const
{
	switch(iexpand)
	{
		case IEXPAND_ALL:   { return TermExpansionPtr(new TermExpansion()); }
		case IEXPAND_DEPTH: { return TermExpansionPtr(new TermDepthExpansion(config)); }
	}
	assert(false);
	return TermExpansionPtr(0);
}

void GringoOptions::initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden)
{
	(void)hidden;
	OptionGroup gringo("Gringo Options");

#pragma message "TODO: re-add options below"
	gringo.addOptions()
		("const,c"   , storeTo(consts)->setComposing(), "Replace constant <c> by value <v>\n", "<c>=<v>")
		("gstats"    , bool_switch(&stats),             "Print extended statistics")
		("dep-graph" , storeTo(depGraph),               "Dump program dependency graph to file", "<file>")

		("text,t"    , bool_switch(&textOut),           "Print plain text format")
		//("reify"     , bool_switch(&metaOut),           "Print reified text format")
		("lparse,l"  , bool_switch(&smodelsOut),        "Print Lparse format")

		("compat"    , bool_switch(&compat),            "Improve compatibility with lparse")
		//("ground,g"  , bool_switch(&groundInput),       "Enable lightweight mode for ground input")
		("shift"     , bool_switch(&disjShift),         "Shift disjunctions into the body")
		("body-order", storeTo(heuristics)->parser(&HeuristicOptions::mapHeuristic)->setImplicit(),
			"Configure body order heuristic\n"
			"      Default: basic\n"
			"      Valid:   basic, unify\n"
			"        basic: basic heuristic\n"
			"        unify: unify to estimate domain sizes")

		("magic"     , bool_switch(&magic),             "Enable magic set rewriting")

		("ifixed"    , storeTo(ifixed),                 "Fix number of incremental steps to <num>", "<num>")
		("iinit"     , storeTo(iinit),                  "Start to ground from step <num>", "<num>")
		("iexpand"   , storeTo(iexpand),
			"Limits the expansion of terms\n"
			"      Default: All\n"
			"      Valid:   All, Depth\n"
			"        All   : Do not limit term expansion\n"
			"        Depth : Limit according to term depth\n")
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
	def += "  --lparse --iinit=1\n";
}
