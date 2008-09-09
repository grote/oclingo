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
	grounder         = true;
	satPreParams.assign(3, 0);
	heuristic        = "berkmin";
	cons             = "";
	numModels        = 1;
	seed             = -1;
	loopRep          = DefaultUnfoundedCheck::common_reason;
	lookahead        = -1;
	eqIters          = 5;
	transExt         = LparseReader::transform_no;
	quiet            = false;
	initialLookahead = false;
	suppModels       = false;
	dimacs           = false;
	optimize         = 0;
	ccmExp           = false;
#endif
}

void Options::initOptions(ProgramOptions::OptionGroup& allOpts, ProgramOptions::OptionGroup& hidden) {
	OptionGroup common("Common-Options:\n");
	common.addOptions()
		("help,h"   , bool_switch(&help),   "Print help and exit")
		("version,v", bool_switch(&version),"Print version and exit")
		("syntax",    bool_switch(&syntax),"Print syntax and exit")
		("verbose,V", bool_switch(&verbose), "Print additional information")
		("stats"    , bool_switch(&stats),  "Print extended statistics")
#ifdef WITH_CLASP
		("solveonly"     , value<bool>(&grounder)->parser(mapMode), "Set the working mode\n"
			"\tDefault: No\n"
			"\t  No  : Standard mode\n"
			"\t  Yes : Pass input directly to clasp (lparse format required)")
#endif
	;
	allOpts.addOptions(common);
	OptionGroup gringo("\n\nGrinGo-Options:\n");
	gringo.addOptions()
		("ifixed"      , value<int>(&grounderOptions.ifixed)  , "Fixed number of incremental steps", "<num>")
		("ground,g", bool_switch(&convert), "Parse plain text")
		("const,c"         , value<vector<string> >(&consts)->setComposing(), "Set constant c to value v", "c=v")
		("lparse,l"        , bool_switch(&smodelsOut), "Print lparse output format")
		("text,t"          , bool_switch(&textOut), "Print plain text format")
#ifdef WITH_CLASP
		("clasp,C"         , bool_switch(&claspOut), "Use internal clasp interface")
#endif
		("aspils,a"        , value<int>(&aspilsOut)->parser(mapASPils), "Print experimental ASPils output in normalform 1-7", "1-7")
		("bindersplitting" , value<bool>(&grounderOptions.binderSplit), "Enable or disable bindersplitting\n"
			"\tDefault: yes\n"
			"\t  yes : Enable bindersplitting\n"
			"\t  no  : Disable bindersplitting")
	;
	allOpts.addOptions(gringo);
#ifdef WITH_ICLASP
	OptionGroup incremental("\n\nOptions for incremental solving:\n");
	incremental.addOptions()
		("istats"      , bool_switch(&istats) , "Print stats for each incremental step")
		("imin"        , value<int>(&imin)    , "Minimum number of incremental steps", "<num>")
		("imax"        , value<int>(&imax)    , "Maximum number of incremental steps", "<num>")
		("incremental,i"   , bool_switch(&iclaspOut), "Use incremental clasp interface")
		("iquery"      , value<int>(&grounderOptions.iquery)  , "Start solving beginning with step num", "<num>")
		("istop"      , value<bool>(&iunsat)->parser(mapStop), "Stop condition during incremental solving\n"
			"\tDefault: SAT\n"
			"\t  UNSAT : Stop if no solution found\n"
			"\t  SAT   : Stop if solution found")
		("ilearnt" , value<bool>(&keepLearnts)->parser(mapKeepForget), "How to handle learnt nogoods during incremental solving\n"
			"\tDefault: forget\n"
			"\t  keep   : Keep learnt nogoods\n"
			"\t  forget : Forget learnt nogoods")
		("iheuristic", value<bool>(&keepHeuristic)->parser(mapKeepForget), "How to handle heuristic information during incremental solving\n"
			"\tDefault: keep\n"
			"\t  keep   : Keep heuristic information\n"
			"\t  forget : Forget heuristic information")
	;
	allOpts.addOptions(incremental);
#endif
#ifdef WITH_CLASP
	OptionGroup clasp("\n\nClasp-Options - Not related to the search:\n");
	clasp.addOptions()
		("quiet,q"  , bool_switch(&quiet),  "Do not print answer sets")
		("dimacs"   , bool_switch(&dimacs), "Read DIMACS- instead of Lparse-Format")
		("seed"     , value<int>(&seed),    "Use <num> as seed for the RNG.", "<num>")
		("trans-ext", value<int>(&transExt)->parser(mapTransExt),
			"How to handle extended rules?\n"
			"\tDefault: no\n"
			"\t  all   : Transform all extended rules to basic rules\n"
			"\t  choice: Transform choice rules but keep weight- and constraint rules\n"
			"\t  weight: Transform weight- and constraint rules but keep choice rules\n"
			"\t  no    : Don't transform extended rules; handle them natively")
		("eq", value<int>(&eqIters), 
			"Run Eq-Preprocessor for at most <n> passes.\n"
			"\tDefault: -1 (run to fixpoint); 0 = disable Eq-Preprocessor\n", "<n>")
		("sat-prepro", value<vector<int> >(&satPreParams)->parser(mapSatElite),
			"Enable SatElite-like preprocessor\n"
			"\tDefault: no (yes if --dimacs), Valid: yes/no or <n1,[n2,n3]>\n"
			"\t  n1 : Max iterations      (-1 means run to fixpoint)\n"
			"\t  n2 : Heuristic cutoff    (-1 means no cutoff) \n"
			"\t  n3 : Max time in seconds (-1 means no time limit)\n")
		("opt-all"  , bool_switch(), "When optimizing compute all optimal solutions")
		("opt-rand" ,bool_switch(), "When computing one optimal solution randomize search")
		("opt-value", value<std::vector<int> >(&optVals),"Initialize the optimization function")
		("brave"    , bool_switch(), "compute the set of brave consequences\n")
		("cautious" , bool_switch(), "compute the set of cautious consequences\n")
	;
	allOpts.addOptions(clasp);
	OptionGroup basic("\nBasic-Options - Configure the search strategy:\n");
	basic.addOptions()
		("number,n",	value<int>(&numModels), "Compute at most <num> models (0 for all)", "<num>")
		("supp-models",bool_switch(&suppModels), "Compute supported models instead of stable models")
		("lookback"		,value<bool>()->defaultValue(true), 
			"Use lookback strategies\n"
			"\tDefault: yes\n"
			"\t  yes: Enable lookback strategies (learning, backjumping and restarts)\n"
			"\t  no : Disable lookback strategies")
		("lookahead"	, value<int>(&lookahead)->parser(mapLookahead),
			"Extend unit propagation with one-step lookahead\n"
			"\tDefault: disabled if lookback=yes, otherwise auto\n"
			"\t  atom  : Restrict lookahead to atoms\n"
			"\t  body  : Restrict lookahead to bodies\n"
			"\t  hybrid: Lookahead atoms and bodies but only in one (opposed) phase\n"
			"\t  auto  : Let clasp decide which literals to test\n"
			"\t  no    : Do not use lookahead")
		("initial-lookahead", bool_switch(&initialLookahead), "Use lookahead when simplifying initial problem")
		("heuristic", value<string>(&heuristic)->parser(mapHeuristic), 
			"Use <heu> as decision heuristic.\n"
			"\tDefault: Berkmin if lookback=yes, otherwise Unit\n"
			"\t  Berkmin: An adaption of the heuristic used in the BerkMin SAT-solver\n"
			"\t  Vmtf   : An adaption of the heuristic used in the Siege SAT-solver\n"
			"\t  Vsids  : An adaption of the heuristic used in the Chaff SAT-solver\n"
			"\t  Unit   : Counts assignments made during Lookahead\n"
			"\t  None   : Select the first free variable","<heu>")
		("rand-prop", value<double>()->defaultValue(0.0), 
			"Choose randomly with a probability equal to <n>\n"
			"\tDefault: 0.0, i.e. always use the selected decision heuristic\n"
			"\t  Valid: [0.0, 1.0]", "<n>")
		("randomize", value<std::pair<int, int> >()->defaultValue(std::pair<int, int>(0,0))->parser(mapRandomize),
			"Run some random passes before starting real search\n"
			"\tDefault: no, Valid: yes/no or 0 <= n1; 1 <= n2\n"
			"\t  yes   : Run 50 passes of 20 conflicts each using a random heuristic\n"
			"\t  no    : Disable randomization\n"
			"\t  n1,n2 : Run n1 passes of n2 conflicts each using a random heuristic\n", "n1,n2")
		("rand-watches", value<bool>()->defaultValue(true), 
			"Place watches randomly when creating nogoods\n"
			"\tDefault: yes\n"
			"\t  no : Initially watch first and last literal in a nogood\n")
	;
	allOpts.addOptions(basic);
	OptionGroup lookback("\nLookback-Options - Require lookback=yes:\n");
	lookback.addOptions()
		("restarts,r", value<vector<double> >()->defaultValue(restartDefault())->parser(mapVec), 
			"Restart policy to use during search\n"
			"\tDefault: 100,1.5\n"
			"\tValid: no or <n1[,n2,n3]>\n"
			"\t  no,n1==0  : Disable restarts\n"
			"\t  n1>0      : Luby et al. run-length sequence with n1 = unit run\n"
			"\t  n1>0,n2>=1: Restart after n1*(n2^k) conflicts (k >= 0)\n"
			"\t  n1,n2,n3>0: Repeat sequence once n3 is reached, then n3 *= n2\n")
		("save-progress",	bool_switch(), "Enable RSat-like progress saving")
		("bounded-restarts", bool_switch(), "Allow (bounded) restarts after solution was found")
		("local-restarts", bool_switch(), "Enable local restarts")
		("shuffle,s", value<std::pair<int,int> >()->defaultValue(std::pair<int,int>(0,0)),
			"Shuffle program after a number of restarts\n"
			"\tDefault: 0,0, Valid: 0 <= n1; 1 <= n2\n"
			"\t  n1,n2 : Shuffle after n1 restarts. Re-Shuffle every n2 restarts\n"
			"\t  n1=0  : Do not shuffle program", "n1,n2")
		("deletion,d", value<vector<double> >()->defaultValue(delDefault())->parser(mapVec), 
			"Set size and grow factor of learnt database\n"
			"\tDefault: 3,1.1,3.0\n"
			"\tValid: no or <n1[,n2,n3]> with 0 <= n1 <= n3; 1.0 <= n2\n"
			"\t  n1,n2,n3 : Size = problem size/n1. MaxSize = Size*n3\n"
			"\t             Size = min(Size*n2,MaxSize) after restart\n"
			"\t  no       : Never delete learnt nogoods")
		("reduce-on-restart", bool_switch(), "Delete some learnt nogoods after each restart")
		("minimize,m", value<int>()->defaultValue(SolverStrategies::all_antes)->parser(mapCflMinimize),
			"Minimize learnt conflict-nogoods\n"
			"\tDefault: all\n"
			"\t  bin  : Check only binary antecedents\n"
			"\t  tern : Check binary and ternary antecedents\n"
			"\t  all  : Check all antecedents\n"
			"\t  no   : Do not minimize conflict-nogoods")
		("expensiveccm", bool_switch(&ccmExp), "enhanced conflict clause minimization\n")
		("contraction", value<int>()->defaultValue(250),
			"Temporarily omit literals from long learnt nogoods\n"
			"\tDefault: 250 | Valid: [0...maxInt), 0 means disabled")
		("loops", value<int>(&loopRep)->parser(mapLoops),
			"Use <arg> as strategy when creating loop formuals\n"
			"\tDefault: common | Valid: common, shared, distinct, no\n"
			"\t For an unfounded set U compute:\n"
			"\t  common  : common reason for U, one nogood for each atom in U\n"
			"\t  shared  : common reason for U, one shared Loop-Formula for atoms in U\n"
			"\t  distinct: compute reason and nogood individually for each atom in U\n"
			"\t  no      : do not learn loop formulas\n")
	;
	allOpts.addOptions(lookback);
#endif
	hidden.addOptions()
#ifdef WITH_CLASP
		("hParams", value<vector<int> >()->defaultValue(vector<int>()), "Additional parameters for heuristic\n")
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
	if (vm.count("opt-all"))	optimize += 1;
	if (vm.count("opt-rand")) optimize += 2;
	if (optimize == 3) {
		warning_ += "Warning: 'opt-all' and 'opt-rand' are mutually exclusive!\n";
		optimize = 1;
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
		if (vm.count("lookahead") == 0)	{ lookahead = Lookahead::auto_lookahead; }
		if (heuristic != "unit" && heuristic != "none") {
			error_ = "Error: selected heuristic requires lookback strategy!\n";
			return false;
		}
		if (!vm["contraction"].isDefaulted()||!vm["minimize"].isDefaulted()) {
			warning_ += "Warning: lookback-options ignored because lookback strategy is not used!\n";
		}
		s.strategies().cflMinAntes = SolverStrategies::no_antes;
		s.strategies().setCompressionStrategy(0);
		s.strategies().saveProgress = false;
		loopRep = DefaultUnfoundedCheck::no_reason;
	}
	else {
		s.strategies().cflMinAntes	= (SolverStrategies::CflMinAntes)value_cast<int>(vm["minimize"]);
		s.strategies().cflMin				= ccmExp ? SolverStrategies::een_minimization : SolverStrategies::beame_minimization;
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
	solveParams.setRandomPropability( value_cast<double>(vm["rand-prop"]) );
	if (s.strategies().search == SolverStrategies::use_learning) {
		std::vector<double> rp = value_cast<vector<double> >(vm["restarts"]);
		rp.resize(3, 0.0);
		bool br = vm.count("bounded-restarts") != 0 && value_cast<bool>(vm["bounded-restarts"]);
		bool lr = vm.count("local-restarts") != 0 && value_cast<bool>(vm["local-restarts"]);
		solveParams.setRestartParams((uint32)rp[0], rp[1], (uint32)rp[2], lr, br);
		bool redOnRestart = vm.count("reduce-on-restart") != 0 && value_cast<bool>(vm["reduce-on-restart"]);
		vector<double> del = value_cast<vector<double> >(vm["deletion"]);
		del.resize(3, 1.0);
		solveParams.setReduceParams(del[0], del[1], del[2], redOnRestart);
		const std::pair<int, int>& rando = value_cast<std::pair<int, int> >(vm["randomize"]);
		solveParams.setRandomizeParams(rando.first, rando.second);
		const std::pair<int, int>& sh = value_cast<std::pair<int, int> >(vm["shuffle"]);
		solveParams.setShuffleParams(sh.first, sh.second);
	}
	else {
		solveParams.setRestartParams(0, 0, false);
		solveParams.setReduceParams(0.0, 0.0, 0.0, false);
		solveParams.setRandomizeParams(0,0);
		solveParams.setShuffleParams(0,0);
		if (!vm["restarts"].isDefaulted()||!vm["deletion"].isDefaulted()||!vm["randomize"].isDefaulted()||!vm["shuffle"].isDefaulted()) {
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
		bool loops	= !heuParams.empty() && heuParams[0] == 1;
		uint32 mtf	= heuParams.size() < 2 ? 8 : heuParams[1];
		heu = new ClaspVmtf( mtf, loops);
	}
	else if (heuristic == "vsids") {
		bool loops	= !heuParams.empty() && heuParams[0] == 1;
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
		<< "usage: " << EXECUTABLE << " [number] [options] [files]" << endl;
#else
	os << EXECUTABLE << " version " << GRINGO_VERSION << "\n"
		<< "usage: " << EXECUTABLE << " [options] [files]" << endl;
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
		<< "can be found at <http://potassco.sourceforge.net>." << std::endl;

}


void Options::printHelp(const OptionGroup& opts, std::ostream& os) const {
	string indent(strlen(EXECUTABLE) + 5, ' ');
#ifdef WITH_CLASP
	os << EXECUTABLE << " version " << GRINGO_VERSION << " (clasp " << CLASP_VERSION << ")\n"
		<< "usage: " << EXECUTABLE << " [number] [options] [files]" << endl;
#else
	os << EXECUTABLE << " version " << GRINGO_VERSION << "\n"
		<< "usage: " << EXECUTABLE << " [options] [files]" << endl;
#endif
	os << opts << endl;
	os << "Default commandline:\n"
		<< "  " << EXECUTABLE
#ifdef WITH_ICLASP
		<< " 1 --incremental --istop=SAT --ilearnt=forget --iheuristic=keep"
#elif defined WITH_CLASP
		<< " 1 --clasp --solveonly=No"
#else
		<< " --lparse --bindersplitting=Yes"
#endif
#ifdef WITH_CLASP
		<< "\n"
		<< indent << "--trans-ext=no --eq=5 \n"
		<< indent << "--lookahead=no --lookback=yes --heuristic=Berkmin\n"
		<< indent << "--rand-prop=0.0 --randomize=no --rand-watches=yes\n"
		<< indent << "--restarts=100,1.5 --deletion=3,1.1,3.0\n"
		<< indent << "--minimize=all --contraction=250 --loops=common"
#endif
		<< endl;
}

void Options::printVersion(std::ostream& os) const {
	os << "GrinGo " << GRINGO_VERSION << "\n";
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

