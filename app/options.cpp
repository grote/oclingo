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
#include "options.h"
#include "program_opts/value.h"

#include <ostream>
#include <algorithm>
#include <cctype>
#include <iostream>

#ifdef WITH_CLASP
#	include <clasp/include/unfounded_check.h>
#	include <clasp/include/heuristics.h>
#	include <clasp/include/lparse_reader.h>
#endif

#include <gringoexception.h>

using namespace NS_GRINGO;
using namespace ProgramOptions;
using namespace std;
#ifdef WITH_CLASP
using namespace Clasp;
#endif

namespace {

std::string toLower(const std::string& s) {
	std::string ret(s);
	for (std::string::size_type i = 0; i != ret.size(); ++i) {
		ret[i] = (char)std::tolower((unsigned char)s[i]);
	}
	return ret;
}
#ifdef WITH_CLASP
bool mapTransExt(const std::string& s, int& i, int*) {
	std::string temp = toLower(s);
	bool b = temp == "all";
	if (b || parseValue(s, b, 1)) {
		i = b ? LparseReader::transform_all : LparseReader::transform_no;
		return true;
	}
	else if (temp == "choice")	{ i = LparseReader::transform_choice; return true; }
	else if (temp == "weight")	{ i = LparseReader::transform_weight; return true; }
	return false;
}
bool mapLookahead(const std::string& s, int& i, int*) {
	std::string temp = toLower(s);
	bool b = temp == "auto";
	if (b || parseValue(s, b, 1)) {
		i = b ? Lookahead::auto_lookahead : -1;
		return true;
	}
	else if	(temp == "atom")			{ i = Lookahead::atom_lookahead; return true; }
	else if (temp == "body")			{ i = Lookahead::body_lookahead; return true; }
	else if (temp == "hybrid")		{ i = Lookahead::hybrid_lookahead; return true; }
	return false;
}
bool mapHeuristic(const std::string& s, std::string& out, std::string*) {
	std::string temp = toLower(s);
	if			(temp == "berkmin")		{ out = temp; return true; }
	else if (temp == "vmtf")			{ out = temp; return true; }
	else if (temp == "vsids")			{ out = temp; return true; }
	else if (temp == "unit")			{ out = temp; return true; }
	else if (temp == "none")			{ out = temp; return true; }
	return false;
}
bool mapCflMinimize(const std::string& s, int& i, int*) {
	std::string temp = toLower(s);
	bool b = temp == "all";
	if (b || parseValue(s, b, 1)) {
		i = b ? SolverStrategies::all_antes : SolverStrategies::no_antes;
		return true;
	}
	else if	(temp == "bin")		{ i = SolverStrategies::binary_antes; return true; }
	else if (temp == "tern")	{ i = SolverStrategies::binary_ternary_antes; return true; }
	return false;
}
bool mapLoops(const std::string& s, int& i, int*) {
	std::string temp = toLower(s);
	bool b = temp == "common";
	if (b || parseValue(s, b, 1)) {
		i = b ? DefaultUnfoundedCheck::common_reason : DefaultUnfoundedCheck::only_reason;
		return true;
	}
	else if (temp == "shared")		{ i = DefaultUnfoundedCheck::shared_reason; return true; }
	else if (temp == "distinct")	{ i = DefaultUnfoundedCheck::distinct_reason; return true; }
	return false;
}
bool mapVec(const std::string& s, std::vector<double>& v, std::vector<double>* def) {
	assert(def);
	v.clear();
	bool b;
	if (parseValue(s, b, 1)) {
		if (b)	{ v = *def; }
		else		{ v.resize(def->size(), 0.0); }
		return true;
	}
	return parseValue(s, v, 1);
}
bool mapRandomize(const std::string& s, std::pair<int, int>& r, std::pair<int, int>*) {
	bool b;
	if (parseValue(s, b, 1)) {
		r.first		= b ? 50 : 0;
		r.second	= b ? 20 : 0;
		return true;
	}
	return parseValue(s, r, 1);
}
bool mapSatElite(const std::string& s, std::vector<int>& v, std::vector<int>*) {
	bool b; v.clear();
	if (s != "1" && parseValue(s, b, 1)) {
		if		(b) { v.push_back(-1); v.push_back(-1); v.push_back(-1); }
		else			{ v.push_back(0); v.push_back(0); v.push_back(0); }
		return true;
	}
	if (parseValue(s, v, 1)) {
		if (v.empty())		v.push_back(-1);
		if (v.size()==1)	v.push_back(-1);
		if (v.size()==2)	v.push_back(-1);
	}
	return v.size() == 3;
}
#endif

bool mapKeepForget(const std::string& s, bool &out, bool*) {
	std::string temp = toLower(s);
	if (temp == "keep")   { out = true;  return true; }
	if (temp == "forget") { out = false; return true; }
	return false;
}

bool mapASPils(const std::string& s, int &out, int*) {
	return parseValue(s, out, 1) && out >= 1 && out <= 7;
}

bool mapMode(const std::string& s, bool &out, bool*) {
	std::string temp = toLower(s);
	if (temp == "no") { out = true;  return true; }
	if (temp == "yes")   { out = false; return true; }
	return false;
}

bool mapStop(const std::string& s, bool &out, bool*) {
	std::string temp = toLower(s);
	if (temp == "unsat") { out = true;  return true; }
	if (temp == "sat")   { out = false; return true; }
	return false;
}

bool parseInt(const std::string &s)
{
	char *endptr;
	strtol(s.c_str(), &endptr, 10);
	return (endptr != s.c_str() && !*endptr);
}

void optionCallback(OptionParser *p, const std::string &s)
{
	if(parseInt(s))
		p->addOptionValue("number", s);
	else
		p->addOptionValue("files", s);
}

}

Options::Options() { }

void Options::setDefaults() {
	files.clear();
	stats   = false;
	help    = false;	
	version = false;
	verbose = false;
	syntax  = false;
	shift   = false;

	grounderOptions = Grounder::Options();
	convert         = false;
	consts.clear();

	smodelsOut = false;
	aspilsOut  = -1;
#ifdef WITH_CLASP
	claspOut   = false;
#endif
	textOut    = false;
#ifdef WITH_ICLASP
	outf       = ICLASP_OUT;
#elif defined WITH_CLASP
	outf       = CLASP_OUT;
#else
	outf       = SMODELS_OUT;
#endif

#ifdef WITH_ICLASP
	keepLearnts     = true;
	keepHeuristic   = false;
	iclaspOut       = false;
	imin            = 1;
	imax            = std::numeric_limits<int>::max();
	iunsat          = false;
	istats          = false;
#endif

#ifdef WITH_CLASP
	claspMode       = false;
	satPreParams.assign(3, 0);
	heuristic        = "berkmin";
	cons             = "";
	redFract         = 3.0;
	redInc           = 1.1;
	redMax           = 3.0;
	numModels        = 1;
	seed             = -1;
	loopRep          = DefaultUnfoundedCheck::common_reason;
	projectConfig    = -1;
	lookahead        = -1;
	eqIters          = 5;
	transExt         = LparseReader::transform_no;
	quiet            = false;
	initialLookahead = false;
	suppModels       = false;
	dimacs           = false;
	optAll           = false;
	ccmExp           = false;
	redOnRestart     = false;
	modelRestart     = false;
	recordSol        = false;
	project          = false;
#endif
}

void Options::initOptions(ProgramOptions::OptionGroup& allOpts, ProgramOptions::OptionGroup& hidden) {
#ifdef WITH_CLASP
	OptionGroup clasp("\n\nClasp - General Options:\n");
	clasp.addOptions()
		("number,n", value<int>(&numModels), 
			"Compute at most <num> models (0 for all)\n"
			"      Default: 1", "<num>")
		("quiet,q" , bool_switch(&quiet),  "Do not print models")
		("seed"    , value<int>(&seed),    "Set random number generator's seed to <num>\n", "<num>")

                ("restart-on-model", bool_switch(&modelRestart), "Restart (instead of backtrack) after each model")
		("solution-recording", bool_switch(&recordSol), "Add conflicts for computed models")
		("project", bool_switch(&project), "Project models to named atoms in enumeration mode\n")

		("brave"    , bool_switch(), "Compute brave consequences")
		("cautious" , bool_switch(), "Compute cautious consequences\n")

		("opt-all"    , bool_switch(), "Compute all optimal models")
		("opt-value"  , value<std::vector<int> >(&optVals), 
			"Initialize objective function(s)\n"
			"      Valid:   <n1[,n2,n3,...]>\n")

		("supp-models",bool_switch(&suppModels), "Compute supported (instead of stable) models")
		("dimacs"   , bool_switch(&dimacs), "Read DIMACS (instead of Lparse) format\n")

		("trans-ext", value<int>(&transExt)->parser(mapTransExt),
			"Configure handling of Lparse-like extended rules\n"
			"      Default: no\n"
			"      Valid:   all, choice, weight, no\n"
			"        all   : Transform all extended rules to basic rules\n"
			"        choice: Transform choice rules, but keep cardinality and weight rules\n"
			"        weight: Transform cardinality and weight rules, but keep choice rules\n"
			"        no    : Do not transform extended rules\n")

		("eq", value<int>(&eqIters), 
			"Configure equivalence preprocessing\n"
			"      Default: 5\n"
			"      Valid:\n"
			"        -1 : Run to fixpoint\n"
			"        0  : Do not run equivalence preprocessing\n"
			"        > 0: Run for at most <n> iterations\n", "<n>")

		("sat-prepro", value<vector<int> >(&satPreParams)->parser(mapSatElite),
			"Configure SatElite-like preprocessing\n"
			"      Default: no (yes, if --dimacs)\n"
			"      Valid:   yes, no, <n1[,n2,n3]>\n"
			"        <n1>: Run for at most <n1> iterations           (-1=run to fixpoint)\n"
			"        <n2>: Run variable elimination with cutoff <n2> (-1=no cutoff)\n"
			"        <n3>: Run for at most <n3> seconds              (-1=no time limit)\n"
			"        yes : Run to fixpoint, no cutoff and no time limit\n")

		("rand-watches", value<bool>()->defaultValue(true), 
			"Configure watched literal initialization\n"
			"      Default: yes\n"
			"      Valid:   yes, no\n"
			"        yes: Randomly determine watched literals\n"
			"        no : Watch first and last literal in a nogood\n")
	;
	allOpts.addOptions(clasp);
	OptionGroup basic("\nClasp - Search Options:\n");
	basic.addOptions()
		("lookback"		,value<bool>()->defaultValue(true), 
			"Configure lookback strategies\n"
			"      Default: yes\n"
			"      Valid:   yes, no\n"
			"        yes: Enable lookback strategies (backjumping, learning, restarts)\n"
			"        no : Disable lookback strategies\n")

		("lookahead"	, value<int>(&lookahead)->parser(mapLookahead),
			"Configure failed-literal detection\n"
			"      Default: no (auto, if --lookback=no)\n"
			"      Valid:   atom, body, hybrid, auto, no\n"
			"        atom  : Apply failed-literal detection to atoms\n"
			"        body  : Apply failed-literal detection to bodies\n"
			"        hybrid: Apply Nomore++-like failed-literal detection\n"
			"        auto  : Let Clasp pick a failed-literal detection strategy\n"
			"        no    : Do not apply failed-literal detection")
		("initial-lookahead", bool_switch(&initialLookahead), "Apply failed-literal detection for preprocessing\n")

		("heuristic", value<string>(&heuristic)->parser(mapHeuristic), 
			"Configure decision heuristic\n"
			"      Default: Berkmin (Unit, if --lookback=no)\n"
			"      Valid:   Berkmin, Vmtf, Vsids, Unit, None\n"
			"        Berkmin: Apply BerkMin-like heuristic\n"
			"        Vmtf   : Apply Siege-like heuristic\n"
			"        Vsids  : Apply Chaff-like heuristic\n"
			"        Unit   : Apply Smodels-like heuristic\n"
			"        None   : Select the first free variable")
		("rand-freq", value<double>()->defaultValue(0.0), 
			"Make random decisions with probability <p>\n"
			"      Default: 0.0\n"
			"      Valid:   [0.0...1.0]\n", "<p>")

		("rand-prob", value<std::pair<int, int> >()->defaultValue(std::pair<int, int>(0,0))->parser(mapRandomize),
			"Configure random probing\n"
			"      Default: no\n"
			"      Valid:   yes, no, <n1,n2> (<n1> >= 0, <n2> > 0)\n"
			"        yes    : Run 50 random passes up to at most 20 conflicts each\n"
			"        no     : Do not run random probing\n"
			"        <n1,n2>: Run <n1> random passes up to at most <n2> conflicts each\n")
	;
	allOpts.addOptions(basic);
	OptionGroup lookback("\nClasp - Lookback Options (Require: lookback=yes):\n");
	lookback.addOptions()
		("restarts,r", value<vector<double> >()->defaultValue(restartDefault())->parser(mapVec), 
			"Configure restart policy\n"
			"      Default: 100,1.5\n"
			"      Valid:   <n1[,n2,n3]> (<n1> >= 0, <n2>,<n3> > 0), no\n"
			"        <n1>          : Run Luby et al.'s sequence with unit length <n1>\n"
			"        <n1>,<n2>     : Run geometric sequence of <n1>*(<n2>^i) conflicts\n"
			"        <n1>,<n2>,<n3>: Run Biere's inner-outer geometric sequence (<n3>=outer)\n"
			"        <n1> = 0, no  : Disable restarts")
		("local-restarts"  , bool_switch(), "Enable Ryvchin et al.'s local restarts")
		("bounded-restarts", bool_switch(), "Enable (bounded) restarts during model enumeration")
		("save-progress"   , bool_switch(), "Enable RSat-like progress saving\n")

		("shuffle,s", value<std::pair<int,int> >()->defaultValue(std::pair<int,int>(0,0)),
			"Configure shuffling after restarts\n"
			"      Default: 0,0\n"
			"      Valid:   <n1,n2> (<n1> >= 0, <n2> >= 0)\n"
			"        <n1> > 0: Shuffle problem after <n1> and re-shuffle every <n2> restarts\n"
			"        <n1> = 0: Do not shuffle problem after restarts\n"
			"        <n2> = 0: Do not re-shuffle problem\n", "<n1,n2>")

		("deletion,d", value<vector<double> >()->defaultValue(delDefault())->parser(mapVec), 
			"Configure size of learnt nogood database\n"
			"      Default: 3.0,1.1,3.0\n"
			"      Valid:   <n1[,n2,n3]> (<n3> >= <n1> >= 0, <n2> >= 1.0), no\n"
			"        <n1,n2,n3>: Store at most min(P/<n1>*(<n2>^i),P*<n3>) learnt nogoods,\n"
			"                    P and i being initial problem size and number of restarts\n"
			"        no        : Do not delete learnt nogoods")
		("reduce-on-restart", bool_switch(), "Delete some learnt nogoods after every restart\n")

		("strengthen", value<int>()->defaultValue(SolverStrategies::all_antes)->parser(mapCflMinimize),
			"Configure conflict nogood strengthening\n"
			"      Default: all\n"
			"      Valid:   bin, tern, all, no\n"
			"        bin : Check only binary antecedents for self-subsumption\n"
			"        tern: Check binary and ternary antecedents for self-subsumption\n"
			"        all : Check all antecedents for self-subsumption\n"
			"        no  : Do not check antecedents for self-subsumption")
		("recursive-str", bool_switch(&ccmExp), "Enable MiniSAT-like conflict nogood strengthening\n")

		("loops", value<int>(&loopRep)->parser(mapLoops),
			"Configure learning of loop formulas\n"
			"      Default: common\n"
			"      Valid:   common, distinct, shared, no\n"
			"        common  : Learn loop nogoods for atoms in an unfounded set\n"
			"        distinct: Learn loop nogood for one atom per unfounded set\n"
			"        shared  : Learn loop formula for a whole unfounded set\n"
			"        no      : Do not learn loop formulas\n")

		("contraction", value<int>()->defaultValue(250),
			"Configure (temporary) contraction of learnt nogoods\n"
			"      Default: 250\n"
			"      Valid:\n"
			"        0  : Do not contract learnt nogoods\n"
			"        > 0: Contract learnt nogoods containing more than <num> literals\n", "<num>")
	;
	allOpts.addOptions(lookback);
#endif
	OptionGroup gringo("\nGrinGo Options:\n");
	gringo.addOptions()
		("const,c"         , value<vector<string> >(&consts)->setComposing(), "Replace constant <c> by value <v>\n", "<c>=<v>")

		("text,t"          , bool_switch(&textOut), "Print plain text format")
		("lparse,l"        , bool_switch(&smodelsOut), "Print Lparse format")
		("aspils,a"        , value<int>(&aspilsOut)->parser(mapASPils), 
			"Print ASPils format in normal form <num>\n"
			"      Default: 7\n"
			"      Valid:   [1...7]\n"
			"        1: Print in normal form Simple\n"
			"        2: Print in normal form SimpleDLP\n"
			"        3: Print in normal form SModels\n"
			"        4: Print in normal form CModels\n"
			"        5: Print in normal form CModelsExtended\n"
			"        6: Print in normal form DLV\n"
			"        7: Print in normal form Conglomeration\n" , "<num>")

		("ground,g", bool_switch(&convert), "Enable lightweight mode for ground input\n")
		("shift"           , bool_switch(&shift), "Shift disjunctions into the body\n")

		("bindersplit" , value<bool>(&grounderOptions.binderSplit), 
		        "Configure binder splitting\n"
			"      Default: yes\n"
			"      Valid:   yes, no\n"
			"        yes: Enable binder splitting\n"
			"        no : Disable binder splitting\n")

		("ifixed", value<int>(&grounderOptions.ifixed)  , "Fix number of incremental steps to <num>", "<num>")
		("ibase",  bool_switch(&grounderOptions.ibase)  , "Process base program only\n")
	;
	allOpts.addOptions(gringo);
#ifdef WITH_ICLASP
	OptionGroup incremental("\nIncremental Computation Options:\n");
	incremental.addOptions()
		("istats"      , bool_switch(&istats) , "Print statistics for each incremental step\n")

		("imin"        , value<int>(&imin)    , "Perform at least <num> incremental steps", "<num>")
		("imax"        , value<int>(&imax)    , "Perform at most <num> incremental steps\n", "<num>")

		("istop"      , value<bool>(&iunsat)->parser(mapStop), 
			"Configure termination condition\n"
			"      Default: SAT\n"
			"      Valid:   SAT, UNSAT\n"
			"        SAT  : Terminate after first satisfiable subproblem\n"
			"        UNSAT: Terminate after first unsatisfiable subproblem\n")

		("iquery"      , value<int>(&grounderOptions.iquery), 
			"Start solving at step <num>\n"
			"      Default: 1\n", "<num>")

		("ilearnt" , value<bool>(&keepLearnts)->parser(mapKeepForget), 
			"Configure persistence of learnt nogoods\n"
			"      Default: keep\n"
			"      Valid:   keep, forget\n"
			"        keep  : Maintain learnt nogoods between incremental steps\n"
			"        forget: Drop learnt nogoods after every incremental step")
		("iheuristic", value<bool>(&keepHeuristic)->parser(mapKeepForget), 
			"Configure persistence of heuristic information\n"
			"      Default: forget\n"
			"      Valid:   keep, forget\n"
			"        keep  : Maintain heuristic values between incremental steps\n"
			"        forget: Drop heuristic values after every incremental step\n")
	;
	allOpts.addOptions(incremental);
#endif
	OptionGroup common("\nBasic Options:\n");
	common.addOptions()
		("help,h"   , bool_switch(&help),    "Print help information and exit")
		("version,v", bool_switch(&version), "Print version information and exit")
		("syntax",    bool_switch(&syntax),  "Print syntax information and exit\n")

		("stats"    , bool_switch(&stats),                 "Print extended statistics")
		("verbose,V", bool_switch(&verbose),               "Print additional information")
		("debug"    , bool_switch(&grounderOptions.debug), "Print internal representations of rules during grounding\n")
#ifdef WITH_ICLASP
		("clasp",  bool_switch(&claspMode), "Run in Clasp mode")
		("clingo", bool_switch(&claspOut), "Run in Clingo mode\n")
#elif defined WITH_CLASP
		("clasp",  bool_switch(&claspMode), "Run in Clasp mode\n")
#endif
	;
	allOpts.addOptions(common);
	hidden.addOptions()
#ifdef WITH_CLASP
		("hParams", value<vector<int> >()->defaultValue(vector<int>()), "Additional parameters for heuristic\n")
		("project-opt", value<int>(&projectConfig), "Additional options for projection as octal digit\n")
#endif
		("files",   value<vector<string> >(&files)->setComposing(), "The files which have to be parsed\n")
	;

}

bool Options::parse(int argc, char** argv, std::ostream& os, OptionValues& values) {
	setDefaults();
	error_.clear();
	try {
		OptionGroup allOpts, visible, hidden;
		initOptions(visible, hidden);
		allOpts.addOptions(visible).addOptions(hidden);
		warning_.clear();
		error_.clear();
		values.store(parseCommandLine(argc, argv, allOpts, false, optionCallback));
		if (help) { 
			printHelp(visible, os);
			return true;
		}
		if (version) {
			printVersion(os);
			return true;
		}
		if (syntax) {
			printSyntax(os);
			return true;
		}
		checkCommonOptions(values);
	}
	catch(const std::exception& e) {
		error_ = e.what();
		return false;
	}
	return true;
}

namespace
{
	int f(bool b)
	{
		return b ? 1 : 0;
	}
}

void Options::checkCommonOptions(const OptionValues& vm) {
#ifdef WITH_ICLASP
	if(f(smodelsOut) + f(aspilsOut > 0) + f(claspOut) + f(iclaspOut) + f(textOut) > 1)
#elif defined WITH_CLASP
	if(f(smodelsOut) + f(aspilsOut > 0) + f(claspOut) + f(textOut) > 1)
#else
	if(f(smodelsOut) + f(aspilsOut > 0) + f(textOut) > 1)
#endif
		throw(GrinGoException("multiple outputs defined"));
	
	if(smodelsOut)
		outf = SMODELS_OUT;
	if(aspilsOut > 0)
		outf = GRINGO_OUT;
#ifdef WITH_CLASP
	if(claspOut)
		outf = CLASP_OUT;
#endif
#ifdef WITH_ICLASP
	if(iclaspOut)
		outf = ICLASP_OUT;
#endif
	if(textOut)
		outf = TEXT_OUT;

	if(grounderOptions.ibase)
	{
		if(outf == ICLASP_OUT)
			outf = CLASP_OUT;
		grounderOptions.iquery = 1;
		grounderOptions.ifixed = -1;
	}

	grounderOptions.iquery = max(grounderOptions.iquery, 0);

	if(grounderOptions.ifixed >= 0 && outf == ICLASP_OUT)
	{
		grounderOptions.ifixed = -1;
		warning_ += "Warning: Option ifixed will be ignored!\n";
	}

#ifdef WITH_CLASP
	if (seed < 0 && seed != -1) {
		warning_ += "Warning: Invalid seed will be ignored!\n";
		seed = -1;
	}
	if (numModels < 0) {
		warning_ += "Warning: Invalid model-number. Forcing 1!\n";
		numModels = 1;
	}
	if (dimacs && vm.count("sat-prepro") == 0) {
		satPreParams.assign(3, -1);
	}
	if (suppModels == true && eqIters != 0) {
		if (vm.count("eq") != 0) {
			warning_ += "Warning: supp-Models requires --eq=no. Disabling eq-preprocessor!\n";
		}
		eqIters = 0;
	}
	bool bc = vm.count("brave") != 0;
	bool cc = vm.count("cautious") != 0;
	if (bc && cc) {
		warning_ += "Warning: 'brave' and 'cautious' are mutually exclusive!\n";
		bc = false;
	}
	if (bc || cc) {
		cons = bc ? "brave" : "cautious";
		if (dimacs) {
			warning_ += "Warning: '" + cons + "' and 'dimacs' are mutually exclusive!\n";
			cons = "";
		}
	}
        if (modelRestart) {
		recordSol = true;
	}
        if (vm.count("project") != 0 && projectConfig == -1) {
		projectConfig = 7;
	}
#endif
}

#ifdef WITH_CLASP
bool Options::initSolver(Solver& s, OptionValues &values)
{
	return setSolverStrategies(s, values) && setSolveParams(s, values);
}

bool Options::setSolverStrategies(Solver& s, const OptionValues& vm) {
	s.strategies().randomWatches = value_cast<bool>(vm["rand-watches"]);
	s.strategies().search = value_cast<bool>(vm["lookback"]) ? Clasp::SolverStrategies::use_learning : Clasp::SolverStrategies::no_learning;
	if (s.strategies().search == SolverStrategies::no_learning) {
		if (vm.count("heuristic") == 0) { heuristic = "unit"; }
		if (vm.count("lookahead") == 0) { lookahead = Lookahead::auto_lookahead; }
		if (heuristic != "unit" && heuristic != "none") {
			error_ = "Error: selected heuristic requires lookback strategy!\n";
			return false;
		}
		if (!vm["contraction"].isDefaulted()||!vm["strengthen"].isDefaulted()) {
			warning_ += "Warning: lookback-options ignored because lookback strategy is not used!\n";
		}
		s.strategies().cflMinAntes = SolverStrategies::no_antes;
		s.strategies().setCompressionStrategy(0);
		s.strategies().saveProgress = false;
		loopRep = DefaultUnfoundedCheck::no_reason;
	}
	else {
		s.strategies().cflMinAntes  = (SolverStrategies::CflMinAntes)value_cast<int>(vm["strengthen"]);
		s.strategies().cflMin       = ccmExp ? SolverStrategies::een_minimization : SolverStrategies::beame_minimization;
		s.strategies().setCompressionStrategy(value_cast<int>(vm["contraction"]));
		s.strategies().saveProgress = vm.count("save-progress") != 0 && value_cast<bool>(vm["save-progress"]);
	}
	if (heuristic == "unit" && lookahead == -1) {
		warning_ += "Warning: Unit-heuristic implies lookahead. Forcing auto-lookahead!\n";
		lookahead = Lookahead::auto_lookahead;
	}
	s.strategies().heuristic.reset( createHeuristic(value_cast<vector<int> >(vm["hParams"])) );
	return true;
}

bool Options::setSolveParams(Solver& s, const OptionValues& vm) {
	solveParams.setRandomProbability( value_cast<double>(vm["rand-freq"]) );
	if (s.strategies().search == SolverStrategies::use_learning) {
		std::vector<double> rp = value_cast<vector<double> >(vm["restarts"]);
		rp.resize(3, 0.0);
		bool br = false;
		if (vm.count("bounded-restarts") != 0) {
			br = value_cast<bool>(vm["bounded-restarts"]);
		}
		else {
			br = project;
		}
		bool lr = vm.count("local-restarts") != 0 && value_cast<bool>(vm["local-restarts"]);
		solveParams.setRestartParams((uint32)rp[0], rp[1], (uint32)rp[2], lr, br);
		vector<double> del = value_cast<vector<double> >(vm["deletion"]);
		del.resize(3, 1.0);
		redFract      = del[0];
		redInc        = del[1];
		redMax        = del[2];
		const std::pair<int, int>& rando = value_cast<std::pair<int, int> >(vm["rand-prob"]);
		solveParams.setRandomizeParams(rando.first, rando.second);
		const std::pair<int, int>& sh = value_cast<std::pair<int, int> >(vm["shuffle"]);
		solveParams.setShuffleParams(sh.first, sh.second);
	}
	else {
		solveParams.setRestartParams(0, 0, false);
		redFract = redInc = redMax = 1.0;
		solveParams.setRandomizeParams(0,0);
		solveParams.setShuffleParams(0,0);
		if (!vm["restarts"].isDefaulted()||!vm["deletion"].isDefaulted()||!vm["rand-prob"].isDefaulted()||!vm["shuffle"].isDefaulted()) {
			warning_ += "Warning: lookback-options ignored because lookback strategy is not used!\n";     
		}
	}
	return true;
}

DecisionHeuristic* Options::createHeuristic(const std::vector<int>& heuParams) const {
	DecisionHeuristic* heu = 0; 
	if (heuristic == "berkmin") {
		bool loops = heuParams.empty() || heuParams[0] == 1;
		uint32 maxB= heuParams.size() < 2 ? 0 : heuParams[1];
		heu = new ClaspBerkmin(maxB, loops);
	}
	else if (heuristic == "vmtf") {
		bool loops  = !heuParams.empty() && heuParams[0] == 1;
		uint32 mtf  = heuParams.size() < 2 ? 8 : heuParams[1];
		heu = new ClaspVmtf( mtf, loops);
	}
	else if (heuristic == "vsids") {
		bool loops  = !heuParams.empty() && heuParams[0] == 1;
		heu = new ClaspVsids(loops);
	}
	else if (heuristic == "none") {
		heu = new SelectFirst();
	}
	if (lookahead != -1) {
		return new Lookahead(Lookahead::Type(lookahead), heu);
	}
	return heu;
}

#endif

void Options::printSyntax(std::ostream& os) const
{
	string indent(strlen(EXECUTABLE) + 5, ' ');
#ifdef WITH_CLASP
	os << EXECUTABLE << " version " << GRINGO_VERSION << " (clasp " << CLASP_VERSION << ")\n"
		<< "Usage: " << EXECUTABLE << " [number] [options] [files]" << endl;
#else
	os << EXECUTABLE << " version " << GRINGO_VERSION << "\n"
		<< "Usage: " << EXECUTABLE << " [options] [files]" << endl;
#endif
	os << "The  input  language   supports  standard  logic  programming" << std::endl
		<< "syntax, including  function  symbols (' f(X,g(a,X)) '),  classical negation  ('-a')," << std::endl
		<< "disjunction ('a | b :- c.'), and  various types of aggregates e.g." << std::endl
		<< std::endl
		<< indent << "'count'" << std::endl
		<< indent << indent << "0 count {a, b, not c} 2." << std::endl
		<< indent << indent << "0       {a, b, not c} 2." << std::endl
		<< indent << "'sum'" << std::endl
		<< indent << indent << "1 sum   [a=1, b=3, not c=-2] 2." << std::endl
		<< indent << indent << "1       [a=1, b=3, not c=-2] 2." << std::endl
		<< indent << "'min'" << std::endl
		<< indent << indent << "0 max   [a=1, b=2, not c=3] 5." << std::endl
		<< indent << "'max'" << std::endl
		<< indent << indent << "1 min   [a=2, b=4, not c=1] 3." << std::endl
		<< "Further  details and  notes on compatibility  to lparse" << std::endl
		<< "can be found at <http://potassco.sourceforge.net/>." << std::endl;
}

void Options::printHelp(const OptionGroup& opts, std::ostream& os) const {
	// version + usage
#ifdef WITH_CLASP
	string indent(strlen(EXECUTABLE) + 5, ' ');
	os << EXECUTABLE << " version " << GRINGO_VERSION << " (clasp " << CLASP_VERSION << ")\n\n"
		<< "Usage: " << EXECUTABLE << " [number] [options] [files]" << endl;
#else
	os << EXECUTABLE << " version " << GRINGO_VERSION << "\n\n"
		<< "Usage: " << EXECUTABLE << " [options] [files]" << endl;
#endif
	// options
	os << opts << endl;
	// usage again :)
#ifdef WITH_CLASP
	os << "Usage: " << EXECUTABLE << " [number] [options] [files]" << endl << endl;
#else
	os << "Usage: " << EXECUTABLE << " [options] [files]" << endl << endl;
#endif
	// default commandline
	os << "Default commandline:\n"
		<< "  " << EXECUTABLE
#ifdef WITH_CLASP
		<< " 1 --trans-ext=no --eq=5 --sat-prepro=no --rand-watches=yes\n"
		<< indent << "--lookback=yes --lookahead=no --heuristic=Berkmin\n"
		<< indent << "--rand-freq=0.0 --rand-prob=no\n"
		<< indent << "--restarts=100,1.5 --shuffle=0,0 --deletion=3.0,1.1,3.0\n"
		<< indent << "--strengthen=all --loops=common --contraction=250\n"
		<< indent << "--bindersplit=yes\n"
#	ifdef WITH_ICLASP
		<< indent << "--istop=SAT --iquery=1 --ilearnt=keep --iheuristic=forget\n"
#	endif
#else
		<< " --lparse --bindersplit=yes\n"
#endif
		;
}

void Options::printVersion(std::ostream& os) const {
	os << EXECUTABLE << " " << GRINGO_VERSION << "\n";
	os << "Copyright (C) Roland Kaminski" << "\n";
	os << "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n";
	os << "GrinGo is free software: you are free to change and redistribute it.\n";
	os << "There is NO WARRANTY, to the extent permitted by law." << endl; 
#ifdef WITH_CLASP
	os << endl;
	os << "clasp " << CLASP_VERSION << "\n";
	os << "Copyright (C) Benjamin Kaufmann" << "\n";
	os << "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n";
	os << "clasp is free software: you are free to change and redistribute it.\n";
	os << "There is NO WARRANTY, to the extent permitted by law." << endl; 
#endif
}

