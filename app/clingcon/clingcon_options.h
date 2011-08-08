// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
// Copyright (c) 2009, Benjamin Kaufmann
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

#include <string>
#include <utility>
#include <program_opts/app_options.h>
#include <gringo/grounder.h>
#include <clasp/clasp_facade.h>
#include <program_opts/value.h>
#include "gringo/gringo_options.h"
//#include "clingo/clingo_options.h"

enum CSPMode { CSPCLASP, CLINGCON, ICLINGCON, OCLINGCON };

struct iClingconConfig : public Clasp::IncrementalControl
{
	iClingconConfig()
		: minSteps(1)
		, maxSteps(~uint32(0))
		, iQuery(1)
		, stopUnsat(false)
		, keepLearnt(true)
		, keepHeuristic(false)
	{ }
	void initStep(Clasp::ClaspFacade& f);
	bool nextStep(Clasp::ClaspFacade& f);

	uint32 minSteps;      /**< Perform at least minSteps incremental steps */
	uint32 maxSteps;      /**< Perform at most maxSteps incremental steps */
	int    iQuery;
	bool   stopUnsat;     /**< Stop on first unsat problem? */
	bool   keepLearnt;    /**< Keep learnt nogoods between incremental steps? */
	bool   keepHeuristic; /**< Keep heuristic values between incremental steps? */
};

template <CSPMode M>
struct ClingconOptions
{
	ClingconOptions();
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden);
	bool validateOptions(ProgramOptions::OptionValues& values, GringoOptions& opts, Messages&);
	void addDefaults(std::string& def);

	bool claspMode;  // default: false
	bool clingconMode; // default: true for clingo, false for iclingo
        std::pair<uint32, bool> numAS; // number (default 0), weakAS is true/false
        std::string      cspICL; // Default: "default"
        std::string      cspBranchVar;     // Default: "SIZE_MIN"
        std::string      cspBranchVal;     // Default: "SPLIT_MIN"
        bool             cspLazyLearn;     // Default: true

	CSPMode mode;       // default: highest mode the current binary supports
	bool iStats;     // default: false
	iClingconConfig inc;
};

bool mapStop(const std::string& s, bool&);
bool mapKeepForget(const std::string& s, bool&);
bool mapNumAS(const std::string& s, std::pair<uint32, bool>&);


//////////////////////////// ClingoOptions ////////////////////////////////////

template <CSPMode M>
ClingconOptions<M>::ClingconOptions()
	: claspMode(false)
	, clingconMode(M == CLINGCON)
	, mode(M)
        , iStats(false)
        , cspLazyLearn(true)
{ }

template <CSPMode M>
void ClingconOptions<M>::initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden)
{
	(void)hidden;
	using namespace ProgramOptions;
	if(M == ICLINGCON || M == OCLINGCON)
	{
		clingconMode = false;
		OptionGroup incremental("Incremental Computation Options");
		incremental.addOptions()
			("istats"      , bool_switch(&iStats) , "Print statistics for each incremental step\n")

			("imin"        , storeTo(inc.minSteps), "Perform at least <num> incremental solve steps", "<num>")
			("imax"        , storeTo(inc.maxSteps), "Perform at most <num> incremental solve steps\n", "<num>")

			("istop"      , storeTo(inc.stopUnsat)->parser(mapStop),
				"Configure termination condition\n"
				"      Default: SAT\n"
				"      Valid:   SAT, UNSAT\n"
				"        SAT  : Terminate after first satisfiable subproblem\n"
				"        UNSAT: Terminate after first unsatisfiable subproblem\n")

			("iquery"      , storeTo(inc.iQuery)->defaultValue(1),
				"Start solving after <num> step\n"
				"      Default: 1\n", "<num>")

			("ilearnt" , storeTo(inc.keepLearnt)->parser(mapKeepForget),
				"Configure persistence of learnt nogoods\n"
				"      Default: keep\n"
				"      Valid:   keep, forget\n"
				"        keep  : Maintain learnt nogoods between incremental steps\n"
				"        forget: Drop learnt nogoods after every incremental step")
			("iheuristic", storeTo(inc.keepHeuristic)->parser(mapKeepForget),
				"Configure persistence of heuristic information\n"
				"      Default: forget\n"
				"      Valid:   keep, forget\n"
				"        keep  : Maintain heuristic values between incremental steps\n"
				"        forget: Drop heuristic values after every incremental step\n")
		;
		root.addOptions(incremental);
	}
	OptionGroup basic("Basic Options");
	basic.addOptions()("clasp",    bool_switch(&claspMode),  "Run in Clasp mode");
        if(M == ICLINGCON || M == OCLINGCON)
		basic.addOptions()("clingcon",    bool_switch(&clingconMode),  "Run in Clingcon mode");
	root.addOptions(basic,true);
        OptionGroup csp("CSP Options");
        csp.addOptions()("csp-num-as",    storeTo(numAS)->parser(mapNumAS)->defaultValue(std::make_pair(0,true)),
                        "Set number of constraint answers\n"
                        "      weak   : Compute weak answer sets\n"
                        "      0      : Compute all constraint answer sets\n"
                        "      <x>    : Compute at most x constraint solutions for each standard answer set\n");
        csp.addOptions()("csp-lazy-learn", bool_switch(&cspLazyLearn), "Enable lazy learning for csp reasons\n");
        csp.addOptions()("csp-icl", storeTo(cspICL)->defaultValue("default"),
                        "Set propagator by consistency level\n"
                        "      value  : Value propagation or consistency (naive)\n"
                        "      bound  : Bounds propagation or consistency\n"
                        "      domain : Domain propagation or consistency\n"
                        "      default: The default consistency for a constraint\n");
        csp.addOptions()("csp-branch-var", storeTo(cspBranchVar),
                         "Sets the integer branch variable\n"
                         "      NONE           : first unassigned\n"
                                 "      RND            : randomly\n"
                                 "      DEGREE_MIN     : smallest degree\n"
                                 "      DEGREE_MAX     : largest degree\n"
                                 "      AFC_MIN        : smallest accumulated failure count (AFC)\n"
                                 "      AFC_MAX        : largest accumulated failure count (AFC)\n"
                                 "      MIN_MIN        : smallest minimum value\n"
                                 "      MIN_MAX        : largest minimum value\n"
                                 "      MAX_MIN        : smallest maximum value\n"
                                 "      MAX_MAX        : largest maximum value\n"
                                 "      SIZE_MIN       : smallest domain size\n"
                                 "      SIZE_MAX       : largest domain size\n"
                                 "      SIZE_DEGREE_MIN: smallest domain size divided by degree\n"
                                 "      SIZE_DEGREE_MAX: largest domain size divided by degree\n"
                                 "      SIZE_AFC_MIN   : smallest domain size divided by AFC\n"
                                 "      SIZE_AFC_MAX   : largest domain size divided by AFC\n"
                                 "      REGRET_MIN_MIN : smallest minimum-regret\n"
                                 "      REGRET_MIN_MAX : largest minimum-regret\n"
                                 "      REGRET_MAX_MIN : smallest maximum-regret\n"
                                 "      REGRET_MAX_MAX : largest maximum-regret\n"
                        )
                        ("csp-branch-val", storeTo(cspBranchVal), "Sets the integer branch value\n"
                                 "      MIN          : smallest value\n"
                                 "      MED          : greatest value not greater than the median\n"
                                 "      MAX          : largest value\n"
                                 "      RND          : random value\n"
                                 "      SPLIT_MIN    : values not greater than mean of smallest and largest value\n"
                                 "      SPLIT_MAX    : values greater than mean of smallest and largest value\n"
                                 "      VAL_RANGE_MIN: values from smallest range, if domain has several ranges;\n"
                                 "                     otherwise, values not greater than mean of smallest\n"
                                 "                     and largest value\n"
                                 "      VAL_RANGE_MAX: values from largest range, if domain has several ranges;\n"
                                 "                     otherwise, values greater than mean of smallest\n"
                                 "                     and largest value\n"
                                 "      VALUES_MIN   : all values starting from smallest\n"
                                 "      VALUES_MAX   : all values starting from largest\n"
                        );


        root.addOptions(csp,true);

}

template <CSPMode M>
bool ClingconOptions<M>::validateOptions(ProgramOptions::OptionValues& values, GringoOptions& opts, Messages& m)
{
	(void)values;
	(void)opts;
	if(M == ICLINGCON || M == OCLINGCON)
	{
		if (claspMode && clingconMode)
		{
			m.error = "Options '--clingcon' and '--clasp' are mutually exclusive";
			return false;
		}
	}
	if(claspMode)       mode = CSPCLASP;
	else if(clingconMode) mode = CLINGCON;
	else                mode = ICLINGCON;
	return true;
}

template <CSPMode M>
void ClingconOptions<M>::addDefaults(std::string& def)
{
	if(M == ICLINGCON)
	{
		def += "  --istop=SAT --iquery=1 --ilearnt=keep --iheuristic=forget\n";
	}
}

//////////////////////////// iClingconConfig ////////////////////////////////////

void iClingconConfig::initStep(Clasp::ClaspFacade& f)
{
	if (f.step() == 0)
	{
		if (maxSteps == 0)
		{
			f.warning("Max incremental steps must be > 0!");
			maxSteps = 1;
		}
		f.config()->solver->strategies().heuristic->reinit(!keepHeuristic);
	}
	else if (!keepLearnt)
	{
		f.config()->solver->reduceLearnts(1.0f);
	}
}

bool iClingconConfig::nextStep(Clasp::ClaspFacade& f)
{
	using Clasp::ClaspFacade;
	ClaspFacade::Result stopRes = stopUnsat ? ClaspFacade::result_unsat : ClaspFacade::result_sat;
	return --maxSteps && ((minSteps > 0 && --minSteps) || f.result() != stopRes);
}
/*
bool mapStop(const std::string& s, bool& b)
{
	std::string temp = ProgramOptions::toLower(s);
	return (b=true,(temp=="unsat")) || (b=false,(temp=="sat"));
}

bool mapKeepForget(const std::string& s, bool& b)
{
	std::string temp = ProgramOptions::toLower(s);
	return (b=true,(temp == "keep")) || (b=false,(temp == "forget"));
}
*/


bool mapNumAS(const std::string& s, std::pair<uint32, bool>& numAS)
{
    std::string temp = ProgramOptions::toLower(s);
    if (temp=="weak")
    {
        numAS.second=true;
        numAS.first=0;
        return true;
    }
    numAS.second=false;
    return ProgramOptions::parseValue(temp,numAS.first,1);

}
