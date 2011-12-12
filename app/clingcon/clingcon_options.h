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

struct ToLower {
        char operator()(char in) const { return (char)std::tolower(static_cast<unsigned char>(in)); }
};

std::string toLower(const std::string& s) {
        std::string ret; ret.reserve(s.size());
        std::transform(s.begin(), s.end(), std::back_inserter(ret), ToLower());
        return ret;
}

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
        static bool checkICL(const std::string& in, std::string& out)
        {
            std::string temp = toLower(in);
            if      (temp == "value")   { out = temp; return true; }
            else if (temp == "bound")   { out = temp; return true; }
            else if (temp == "domain")  { out = temp; return true; }
            else if (temp == "default") { out = temp; return true; }

            return false;
        }

        static bool checkBranchVar(const std::string& in, std::string& out)
        {
            std::string temp = toLower(in);
            if      (temp == "none")            { out = temp; return true; }
            else if (temp == "rnd")             { out = temp; return true; }
            else if (temp == "degree-min")      { out = temp; return true; }
            else if (temp == "degree-max")      { out = temp; return true; }
            else if (temp == "afc-min")         { out = temp; return true; }
            else if (temp == "afc-max")         { out = temp; return true; }
            else if (temp == "min-min")         { out = temp; return true; }
            else if (temp == "min-max")         { out = temp; return true; }
            else if (temp == "max-min")         { out = temp; return true; }
            else if (temp == "max-max")         { out = temp; return true; }
            else if (temp == "size-min")        { out = temp; return true; }
            else if (temp == "size-max")        { out = temp; return true; }
            else if (temp == "size-degree-min") { out = temp; return true; }
            else if (temp == "size-degree-max") { out = temp; return true; }
            else if (temp == "size-afc-min")    { out = temp; return true; }
            else if (temp == "size-afc-max")    { out = temp; return true; }
            else if (temp == "regret-min-min")  { out = temp; return true; }
            else if (temp == "regret-min-max")  { out = temp; return true; }
            else if (temp == "regret-max-min")  { out = temp; return true; }
            else if (temp == "regret-max-max")  { out = temp; return true; }
            return false;
        }

        static bool checkBranchVal(const std::string& in, std::string& out)
        {
            std::string temp = toLower(in);
            if      (temp == "min")            { out = temp; return true; }
            else if (temp == "med")            { out = temp; return true; }
            else if (temp == "max")            { out = temp; return true; }
            else if (temp == "rnd")            { out = temp; return true; }
            else if (temp == "split-min")      { out = temp; return true; }
            else if (temp == "split-max")      { out = temp; return true; }
            else if (temp == "val-range-min")  { out = temp; return true; }
            else if (temp == "val-range-max")  { out = temp; return true; }
            else if (temp == "values-min")     { out = temp; return true; }
            else if (temp == "values-max")     { out = temp; return true; }
            return false;
        }

        static bool checkReduceReason(const std::string& in, std::string& out)
        {
            std::string temp = toLower(in);
            if      (temp == "simple")            { out = temp; return true; }
            else if (temp == "linear")            { out = temp; return true; }
            else if (temp == "linear-fwd")        { out = temp; return true; }
            else if (temp == "scc")               { out = temp; return true; }
            else if (temp == "range")             { out = temp; return true; }
            else if (temp == "sccrange")          { out = temp; return true; }
            return false;

        }

        static bool checkReduceConflict(const std::string& in, std::string& out)
        {
            std::string temp = toLower(in);
            if      (temp == "simple")            { out = temp; return true; }
            else if (temp == "linear")            { out = temp; return true; }
            else if (temp == "linear-fwd")        { out = temp; return true; }
            else if (temp == "scc")               { out = temp; return true; }
            else if (temp == "range")             { out = temp; return true; }
            else if (temp == "sccrange")          { out = temp; return true; }
            return false;

        }

	bool claspMode;  // default: false
	bool clingconMode; // default: true for clingo, false for iclingo
        std::pair<uint32, bool> numAS; // number (default 0), weakAS is true/false
        std::string      cspICL; // Default: "default"
        std::string      cspBranchVar;     // Default: "SIZE_MIN"
        std::string      cspBranchVal;     // Default: "SPLIT_MIN"
        std::string      cspReason;        // Default: "simple"
        std::string      cspConflict;      // Default: "simple"
        bool             cspLazyLearn;     // Default: true
        std::vector<int> optValues;        // Default: empty
        bool             optAll;           // Default: false
        bool             initialLookahead; // Default: false
        int              cspPropDelay;     // Default: 1

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
        , numAS(std::make_pair(0,true))
        , cspICL("default")
        , cspBranchVar("size_min")
        , cspBranchVal("split_min")
        , cspReason("simple")
        , cspConflict("simple")
        , cspLazyLearn(true)
        , optAll(false)
        , initialLookahead(false)
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
                        "      <x>    : Compute at most x constraint solutions for each standard answer set\n")
        ("csp-lazy-learn", bool_switch(&cspLazyLearn), "Enable lazy learning for csp reasons\n")
        ("csp-icl", storeTo(cspICL)->defaultValue("default")->parser(ClingconOptions::checkICL),
                        "Set propagator by consistency level\n"
                        "      value  : Value propagation or consistency (naive)\n"
                        "      bound  : Bounds propagation or consistency\n"
                        "      domain : Domain propagation or consistency\n"
                        "      default: The default consistency for a constraint\n")
        ("csp-branch-var", storeTo(cspBranchVar)->defaultValue("size-min")->parser(ClingconOptions::checkBranchVar),
                         "Sets the integer branch variable\n"
                                 "      NONE           : first unassigned\n"
                                 "      RND            : randomly\n"
                                 "      DEGREE-MIN     : smallest degree\n"
                                 "      DEGREE-MAX     : largest degree\n"
                                 "      AFC-MIN        : smallest accumulated failure count (AFC)\n"
                                 "      AFC-MAX        : largest accumulated failure count (AFC)\n"
                                 "      MIN-MIN        : smallest minimum value\n"
                                 "      MIN-MAX        : largest minimum value\n"
                                 "      MAX-MIN        : smallest maximum value\n"
                                 "      MAX-MAX        : largest maximum value\n"
                                 "      SIZE-MIN       : smallest domain size(default)\n"
                                 "      SIZE-MAX       : largest domain size\n"
                                 "      SIZE-DEGREE-MIN: smallest domain size divided by degree\n"
                                 "      SIZE-DEGREE-MAX: largest domain size divided by degree\n"
                                 "      SIZE-AFC-MIN   : smallest domain size divided by AFC\n"
                                 "      SIZE-AFC-MAX   : largest domain size divided by AFC\n"
                                 "      REGRET-MIN-MIN : smallest minimum-regret\n"
                                 "      REGRET-MIN-MAX : largest minimum-regret\n"
                                 "      REGRET-MAX-MIN : smallest maximum-regret\n"
                                 "      REGRET-MAX-MAX : largest maximum-regret\n"
                        )
                ("csp-branch-val", storeTo(cspBranchVal)->defaultValue("min")->parser(ClingconOptions::checkBranchVal), "Sets the integer branch value\n"
                                 "      MIN          : smallest value(default)\n"
                                 "      MED          : greatest value not greater than the median\n"
                                 "      MAX          : largest value\n"
                                 "      RND          : random value\n"
                                 "      SPLIT-MIN    : values not greater than mean of smallest and largest value\n"
                                 "      SPLIT-MAX    : values greater than mean of smallest and largest value\n"
                                 "      VAL-RANGE-MIN: values from smallest range, if domain has several ranges;\n"
                                 "                     otherwise, values not greater than mean of smallest\n"
                                 "                     and largest value\n"
                                 "      VAL-RANGE-MAX: values from largest range, if domain has several ranges;\n"
                                 "                     otherwise, values greater than mean of smallest\n"
                                 "                     and largest value\n"
                                 "      VALUES-MIN   : all values starting from smallest\n"
                                 "      VALUES-MAX   : all values starting from largest\n"
                        )
                        ("csp-opt-value"  , ProgramOptions::value<std::vector<int> >(),
                                "Initialize csp objective function(s)\n"
                                "      Valid:   <n1[,n2,n3,...]>\n")
                        ("csp-opt-all"    , bool_switch(&optAll)->defaultValue(false), "Compute all optimal models")
                        ("csp-initial-lookahead"    , bool_switch(&initialLookahead)->defaultValue(false), "Do singular lookahead on initialization")
                 ("csp-prop-delay", storeTo(cspPropDelay)->defaultValue(-1), "Do CSP-Propagation every n requested decision levels\n"
                                 "     -1          : do CSP-Propagation may several times per decision level (default)\n"
                                 "      0          : only on possible model\n"
                                 "      n          : every n requested decision levels\n"
                 )
                ;

        csp.addOptions()
                ("csp-reduce-reason", storeTo(cspReason)->defaultValue("simple")->parser(ClingconOptions::checkReduceReason), "Determine the method to reduce the reasons.\n"
                         "      simple         : Do nothin (default)\n"
                         "      linear         : Do a linear IRS check backward\n"
                         "      linear-fwd     : Do a linear IRS check forward\n"
                         "      range          : Take the first range of fitting literals (backwards)\n"
                         "      scc            : Do an IRS check using the variable dependency tree\n"
                         "      sccrange       : Take the first range of fitting literals connected by variables (backwards)\n"
                )
                ("csp-reduce-conflict", storeTo(cspConflict)->defaultValue("simple")->parser(ClingconOptions::checkReduceConflict), "Determine the method to reduce the conflicts.\n"
                         "      simple         : Do nothin (default)\n"
                         "      linear         : Do a linear IIS check backward\n"
                         "      linear-fwd     : Do a linear IIS check forward\n"
                         "      range          : Take the first range of fitting literals (backwards)\n"
                         "      scc            : Do an IIS check using the variable dependency tree\n"
                         "      sccrange       : Take the first range of fitting literals connected by variables (backwards)\n"
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
        if (values.count("csp-opt-value") != 0) {
                const std::vector<int>& vals = ProgramOptions::value_cast<std::vector<int> >(values["csp-opt-value"]);
                optValues.assign(vals.begin(), vals.end());
        }
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
