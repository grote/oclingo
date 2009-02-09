//
// Copyright(c) 2006-2007, Benjamin Kaufmann
//
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/
//
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version.
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
#include "timer.h"
#include "program_opts/program_options.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <numeric>
#include <signal.h>
#include <sstream>

#include <lparseparser.h>
#include <lparseconverter.h>
#include <grounder.h>
#include <smodelsoutput.h>
#include <lparseoutput.h>
#include <pilsoutput.h>
#include <gringoexception.h>
#include <domain.h>
#ifdef WITH_CLASP
#	include <claspoutput.h>
#	include <clasp/include/lparse_reader.h>
#	include <clasp/include/satelite.h>
#	include <clasp/include/unfounded_check.h>
#	include <clasp/include/cb_enumerator.h>
#	include <clasp/include/model_enumerators.h>
#	include <clasp/include/smodels_constraints.h>
#endif

using namespace std;
#ifdef WITH_CLASP
using namespace Clasp;
#endif
using namespace NS_GRINGO;
using namespace NS_OUTPUT;

struct MainApp
{
	Options   options;
	Timer    t_pre;
	Timer    t_read;
	Timer    t_ground;
	Timer    t_solve;
	Timer    t_all;
#ifdef WITH_ICLASP
	int       steps;
#endif
#ifdef WITH_CLASP
	Solver    solver;
	uint32    problemSize;

	enum Mode { ASP_MODE, SAT_MODE } mode;
#endif
	int run(int argc, char **argv);
private:
	enum State { start_read, start_ground, start_pre, start_solve, end_read, end_ground, end_pre, end_solve };
	void setState(State s);
	void ground(Output &output);
	void printGrounderStats(Output& output) const;
#ifdef WITH_CLASP
	void printSatStats() const;
	void printLpStats() const;
	void printAspStats(bool more) const;
	bool runClasp();

	static void printSatEliteProgress(uint32 i, SatElite::SatElite::Options::Action a)
	{
		/*
		static const char pro[] = { '/', '-', '\\', '|' };
		if(a == SatElite::SatElite::Options::pre_start)
			cerr << '|';

		else if(a == SatElite::SatElite::Options::iter_start)
			cerr << '\b' << pro[i % 4];

		else if(a == SatElite::SatElite::Options::pre_done)
			cerr << '\b' << "Done";

		else if(a == SatElite::SatElite::Options::pre_stopped)
			cerr << '\b' << "Stop";
		*/
	}
	std::istream &getStream()
	{
		if(options.files.size() > 0)
		{
			if(inFile_.is_open())
				return inFile_;
			inFile_.open(options.files[0].c_str());
			if(!inFile_)
			{
				throw std::runtime_error("Can not read from '" + options.files[0] + "'");
			}
			return inFile_;
		}
		return std::cin;
	}
	bool solve();
#	ifdef WITH_ICLASP
	bool solveIncremental();
#	endif
	bool parseLparse();
	bool readSat();
	std::auto_ptr < PreproStats > preStats_;
	std::auto_ptr<AtomIndex>      atomIndex_;
	Enumerator *enum_;
	std::ifstream inFile_;
#endif
} clasp_g;

#ifdef WITH_CLASP
class StdOutPrinter : public ModelPrinter, public CBConsequences::CBPrinter {
public:
	explicit StdOutPrinter(bool quiet, bool report = true) : quiet_(quiet), report_(report) {}
	void printModel(const Solver& s, const ModelEnumerator& e) {
		if (quiet_) return;
		AtomIndex* index = e.getIndex();
		if (index) {
			std::cerr << "Answer: " << s.stats.models << "\n";
			for (AtomIndex::size_type i = 0; i != index->size(); ++i) {
				if (s.value((*index)[i].lit.var()) == trueValue((*index)[i].lit) && !(*index)[i].name.empty()) {
					std::cerr << (*index)[i].name << " ";
				}
			}
			std::cerr << std::endl;
		}
		else {
			const uint32 numVarPerLine = 10;
			std::cerr << "c Model: " << s.stats.models << "\n";
			std::cerr << "v ";
			for (Var v = 1, cnt=0; v <= s.numVars(); ++v) {
				if (s.value(v) == value_false) std::cerr << "-";
				std::cerr << v;
				if (++cnt == numVarPerLine && v+1 <= s.numVars()) { cnt = 0; std::cerr << "\nv"; }
				std::cerr << " ";
			}
			std::cerr << "0 \n" << std::flush;
		}
		printMini(e.getMini(), index?"Optimization":"c Optimization");
	}
	void printReport(const Solver& s, const ModelEnumerator& e) {
		if(!report_)
			return;
		const char* modString = e.getIndex() ? "Models" : "c Models";
		uint64 enumerated = s.stats.models;
		uint64 models     = e.getMini() ? e.getMini()->models() : enumerated;
		std::cerr << "\n";
		std::cerr << std::left << std::setw(12) << modString << ": ";
		if (s.decisionLevel() != s.rootLevel()) {
			char buf[64];
			int wr    = sprintf(buf, "%llu", models);
			buf[wr]   = '+';
			buf[wr+1] = 0;
			std::cerr << std::setw(6) << buf;
		}
		else {
			std::cerr << std::setw(6) << models;
		}
		if (enumerated != models) {
			std::cerr << " (Enumerated: " << enumerated << ")";
		}
		std::cerr   << "\n";
		if (quiet_) {
			printMini(e.getMini(), e.getIndex()?"Optimization":"c Optimization");
		}
	}
	void printConsequences(const Solver&, const CBConsequences& cb) {
		if (!quiet_) {
			std::cerr << (cb.cbType() == CBConsequences::cautious_consequences ? "Cautious" : "Brave") << " consequences: \n";
			const AtomIndex& index = cb.index();
			for (AtomIndex::size_type i = 0; i != index.size(); ++i) {
				if (index[i].lit.watched()) {
					std::cerr << index[i].name << " ";
				}
			}
			std::cerr << std::endl;
			printMini(cb.mini(), "Optimization");
		}
	}
	void printReport(const Solver& s, const CBConsequences& cb) {
		if(!report_)
			return;
		if (quiet_ || s.stats.models == 0) {
			quiet_ = false;
			printConsequences(s, cb);
		}
		std::cerr << "\n";
		std::cerr << std::left << std::setw(12) << "Models" << ": ";
		std::cerr << std::setw(6) << 1;
		if (s.stats.models > 1) {
			std::cerr << " (Enumerated: " << s.stats.models << ")";
		}
		std::cerr << std::endl;
		if (quiet_) printMini(cb.mini(), "Optimization");
	}
	void printMini(MinimizeConstraint* c, const char* prefix) {
		if (!c || c->numRules() == 0) return;
		std::cerr << std::left << std::setw(12) << prefix << ":";
		for (uint32 i = 0; i < c->numRules(); ++i) {
			std::cerr << " " << c->getOptimum(i);
		}
		std::cerr << std::endl;
	}
	void enableReport()
	{
		report_ = true;
	}
private:
	bool quiet_;
	bool report_;
};
#endif

ostream& operator<<(ostream& os, const Timer& t) {
	streamsize old = os.precision(3);
	os << fixed << t.elapsed();
	os.precision(old);
	return os;
}

// return values
const int S_UNKNOWN = 0;
const int S_SATISFIABLE = 10;
const int S_UNSATISFIABLE = 20;
const int S_ERROR = EXIT_FAILURE;
const int S_MEMORY = 107;

static void sigHandler(int)
{
	// ignore further signals
	signal(SIGINT, SIG_IGN);  // Ctrl + C
	signal(SIGTERM, SIG_IGN); // kill(but not kill -9)
#ifdef WITH_CLASP
	bool satM = clasp_g.mode == MainApp::SAT_MODE;
	printf("\n%s*** INTERRUPTED! ***\n",(satM ? "c " : ""));
	clasp_g.t_all.stop();
	printf("%sTime      : %.3f\n",(satM ? "c " : ""), clasp_g.t_all.elapsed());
	printf("%sModels    : %u\n",(satM ? "c " : ""), (uint32) clasp_g.solver.stats.models);
	printf("%sChoices   : %u\n",(satM ? "c " : ""), (uint32) clasp_g.solver.stats.choices);
	printf("%sConflicts : %u\n",(satM ? "c " : ""), (uint32) clasp_g.solver.stats.conflicts);
	printf("%sRestarts  : %u\n",(satM ? "c " : ""), (uint32) clasp_g.solver.stats.restarts);
#else
	printf("\n*** INTERRUPTED! ***\n");
#endif
	_exit(S_UNKNOWN);
}

void MainApp::setState(State s)
{
	switch(s)
	{
	case start_read:
		t_read.start();
		break;
	case end_read:
		t_read.stop();
		break;
	case start_ground:
		t_ground.start();
		break;
	case end_ground:
		t_ground.stop();
		break;
	case start_pre:
		t_pre.start();
		break;
	case end_pre:
		t_pre.stop();
		break;
	case start_solve:
		t_solve.start();
		break;
	case end_solve:
		t_solve.stop();
		break;
	default:;
	}
}

int MainApp::run(int argc, char **argv)
{
	ProgramOptions::OptionValues values;
	if(!options.parse(argc, argv, std::cout, values))
	{
		throw std::runtime_error(options.getError());
	}
#ifdef WITH_CLASP
	if(!options.initSolver(solver, values))
	{
		throw std::runtime_error(options.getError());
	}
#endif
	if(!options.getWarning().empty())
	{
		cerr << options.getWarning() << endl;
	}
	if(options.help || options.version || options.syntax)
	{
		return EXIT_SUCCESS;
	}

#ifdef WITH_CLASP
	if(options.verbose)
		cerr << (mode == ASP_MODE ? "" : "c ") << EXECUTABLE << " version " << GRINGO_VERSION << " (clasp " << CLASP_VERSION << ")" << endl;
	getStream();
	if(options.files.size() > 0)
	{
		vector<string>::iterator i = options.files.begin();
		if(options.verbose)
		{
			cerr << (mode == ASP_MODE ? "" : "c ") << "Reading from " << *i;
			for(i++; i != options.files.end(); i++)
				cerr << ", " << *i;
			cerr << "..." << endl;
		}
		if(options.claspMode && options.files.size() > 1)
			cerr << "Warning: only the first file will be used" << endl;
	}
	else if(options.verbose)
		cerr << (mode == ASP_MODE ? "" : "c ") << "Reading from stdin" << endl;
#else
	if(options.verbose)
	{
		cerr << EXECUTABLE << " version " << GRINGO_VERSION << endl;
		if(options.files.size() > 0)
		{
			vector<string>::iterator i = options.files.begin();
			cerr << "Reading from " << *i;
			for(i++; i != options.files.end(); i++)
				cerr << ", " << *i;
			cerr << "\n";
		}
		else
			cerr << "Reading from stdin" << endl;
	}
#endif

#ifdef WITH_CLASP
	if(options.claspMode)
		return runClasp();
#endif

	switch(options.outf)
	{
		case Options::SMODELS_OUT:
		{
			SmodelsOutput output(&std::cout, options.shift);
			ground(output);
			if (options.stats)
				printGrounderStats(output);
			break;
		}
		case Options::GRINGO_OUT:
		{
			PilsOutput output(&std::cout, options.aspilsOut);
			ground(output);
			if (options.stats)
				printGrounderStats(output);
			break;
		}
		case Options::TEXT_OUT:
		{
			LparseOutput output(&std::cout);
			ground(output);
			if (options.stats)
				printGrounderStats(output);
			break;
		}
#ifdef WITH_CLASP
		case Options::CLASP_OUT:
		case Options::ICLASP_OUT:
			return runClasp();
#endif
		default:;
	}
	return EXIT_SUCCESS;
}

namespace
{
	// bäh want a ptr_vector_owner or sth like that
	struct Streams
	{
		vector<istream *> streams;
		~Streams()
		{
			for(vector<istream *>::iterator i = streams.begin(); i != streams.end(); i++)
				delete *i;
		}
	};

	void getStreams(Options &options, Streams &s)
	{
		std::stringstream *ss = new std::stringstream();
		s.streams.push_back(ss);
		for(vector<string>::iterator i = options.consts.begin(); i != options.consts.end(); i++)
			*ss << "#const " << *i << ".\n";

		if(options.files.size() > 0)
		{
			for(vector<string>::iterator i = options.files.begin(); i != options.files.end(); i++)
			{
				s.streams.push_back(new std::ifstream(i->c_str()));
				if(s.streams.back()->fail())
					throw GrinGoException(std::string("Error: could not open file: ") + *i);
			}
		}
		else
			s.streams.push_back(new std::istream(std::cin.rdbuf()));
	}
}

void MainApp::ground(Output &output)
{
	Streams s;
	getStreams(options, s);
	if(options.convert)
	{
		setState(start_read);
		LparseConverter parser(s.streams);
		if(!parser.parse(&output))
			throw NS_GRINGO::GrinGoException("Error: Parsing failed.");
		setState(end_read);
		// just an approximation
		if (options.outf == Options::TEXT_OUT)
		{
			output.stats_.atoms = 0;
			//output.stats_.atoms = grounder.getDomains()->size();
			const DomainVector* d = parser.getDomains();
			for (DomainVector::const_iterator i = d->begin(); i != d->end(); ++i)
			{
				output.stats_.atoms += (*i)->getDomain().size();
			}
		}
	}
	else
	{
		setState(start_read);
		Grounder grounder(options.grounderOptions);
		LparseParser parser(&grounder, s.streams);
		if(!parser.parse(&output))
			throw NS_GRINGO::GrinGoException("Error: Parsing failed.");
		grounder.prepare(false);
		setState(end_read);
		setState(start_ground);
		if(options.verbose)
			cerr << "Grounding..." << endl;
		grounder.ground();
		setState(end_ground);
		// just an approximation
		if (options.outf == Options::TEXT_OUT)
		{
			output.stats_.atoms = 0;
			//output.stats_.atoms = grounder.getDomains()->size();
			const DomainVector* d = grounder.getDomains();
			for (DomainVector::const_iterator i = d->begin(); i != d->end(); ++i)
			{
				output.stats_.atoms += (*i)->getDomain().size();
			}
		}
	}
}


void MainApp::printGrounderStats(Output& output) const
{
	cerr << "=== Grounder Statistics ===" << std::endl;
	cerr << "#rules      : " << output.stats_.rules << std::endl;
	if (output.stats_.language == Output::Stats::TEXT)
	{
	cerr << "#headatoms  : " << output.stats_.atoms << std::endl;
	}
	else
	{
	cerr << "#atoms      : " << output.stats_.atoms << std::endl;
	if (output.stats_.language == Output::Stats::SMODELS)
	cerr << "#aux. atoms : " << output.stats_.auxAtoms << std::endl;
	}
	cerr << "#count      : " << output.stats_.count << std::endl;
	cerr << "#sum        : " << output.stats_.sum << std::endl;
	cerr << "#min        : " << output.stats_.min << std::endl;
	cerr << "#max        : " << output.stats_.max << std::endl;
	cerr << "#compute    : " << output.stats_.compute << std::endl;
	cerr << "#optimize   : " << output.stats_.optimize << std::endl;
}


#ifdef WITH_CLASP
void MainApp::printSatStats() const
{
	const SolverStatistics &st = solver.stats;
	cerr << "c Models       : " << st.models << "\n";
	cerr << "c Time         : " << t_all << "s(Solve: " << t_solve << "s)\n";
	if(!options.stats)
		return;
	cerr << "c Choices      : " << st.choices << "\n";
	cerr << "c Conflicts    : " << st.conflicts << "\n";
	cerr << "c Restarts     : " << st.restarts << "\n";
	cerr << "c Variables    : " << solver.numVars() << "(Eliminated: " << solver.numEliminatedVars() << ")\n";
	cerr << "c Clauses      : " << st.native[0] << "\n";
	cerr << "c   Binary     : " << st.native[1] << "\n";
	cerr << "c   Ternary    : " << st.native[2] << "\n";
	cerr << "c Lemmas       : " << st.learnt[0] << "\n";
	cerr << "c   Binary     : " << st.learnt[1] << "\n";
	cerr << "c   Ternary    : " << st.learnt[2] << "\n";
}
static double percent(uint32 y, uint32 from)
{
	if(from == 0)
		return 0;
	return(static_cast < double >(y) / from) *100.0;
}

static double percent(const uint32 * arr, uint32 idx)
{
	return percent(arr[idx], arr[0]);
}

static double compAverage(uint64 x, uint64 y)
{
	if(!x || !y)
		return 0.0;
	return static_cast < double >(x) / static_cast < double >(y);
}

void MainApp::printLpStats() const
{
	PreproStats& ps = *preStats_;
	if (ps.trStats) {
	  ps.rules[BASICRULE] -= ps.trStats->rules[0];
	  for (uint32 rt = BASICRULE+1; rt <= OPTIMIZERULE; ++rt) {
			ps.rules[0] -= ps.trStats->rules[rt];
		}
	}
	const Options& o = options;
	cerr << left << setw(12) << "Atoms" << ": " << setw(6) << ps.atoms;
	if (ps.trStats) {
		cerr << " (Original: " << ps.atoms-ps.trStats->auxAtoms << " Auxiliary: " << ps.trStats->auxAtoms << ")";
	}
	cerr << "\n";
	cerr << left << setw(12) << "Rules" << ": " << setw(6) << ps.rules[0] << " (";
	for (int i = 1; i <= OPTIMIZERULE; ++i) {
		if (ps.rules[i] != 0) {
			cerr << i << ": " << ps.rules[i];
			if (ps.trStats && ps.trStats->rules[0] != 0) {
				if (i == BASICRULE) {
					cerr << "/" << ps.rules[BASICRULE]+ps.trStats->rules[0];
				}
				else {
					cerr << "/" << ps.rules[i]-ps.trStats->rules[i];
				}
			}
			cerr << " ";
		}
	}
	cerr << "\b)" << "\n";
	cerr << left << setw(12) << "Bodies" << ": " << ps.bodies << "\n";
	if (ps.numEqs() != 0) {
		cerr << left << setw(12) << "Equivalences" << ": " << setw(6) << ps.numEqs() << " (Atom=Atom: " << ps.numEqs(Var_t::atom_var)
				 << " Body=Body: "   << ps.numEqs(Var_t::body_var)
				 << " Other: " << (ps.numEqs() - ps.numEqs(Var_t::body_var) - ps.numEqs(Var_t::atom_var))
				 << ")" << "\n";
	}
	cerr << left << setw(12) << "Tight" << ": ";
	if (!o.suppModels) {
		if (ps.sccs == 0) {
			cerr << "Yes";
		}
		else {
			cerr << setw(6) << "No"  << " (SCCs: " << ps.sccs << " Nodes: "
					 << ps.ufsNodes << ")";
		}
	}
	else {
		cerr << "N/A";
	}
	cerr << "\n";
}

void MainApp::printAspStats(bool more) const
{
	cerr.precision(1);
	cerr.setf(ios_base::fixed, ios_base::floatfield);
	const SolverStatistics &st = solver.stats;
#ifdef WITH_ICLASP
	if(!options.claspMode && options.outf == Options::ICLASP_OUT && (options.verbose || options.istats))
		cerr << "=============== Summary ===============" << endl;
#endif
	cerr << "\n";
#ifdef WITH_ICLASP
	if(!options.claspMode && options.outf == Options::ICLASP_OUT)
		cerr << "Total Steps : " << steps << std::endl;
#endif
	cerr << left << setw(12) << "Time" << ": " << setw(6) << t_all << "\n";
	if(!options.stats)
		return;
	cerr << "  Reading   : " << t_read << "\n";
	if(!options.claspMode && !options.convert)
		cerr << "  Grounding : " << t_ground << "\n";
	cerr << "  Prepro.   : " << t_pre << "\n";
	cerr << "  Solving   : " << t_solve << "\n";
	cerr << left << setw(12) << "Choices" << ": " << st.choices << "\n";
	cerr << left << setw(12) << "Conflicts" << ": " << st.conflicts << "\n";
	cerr << left << setw(12) << "Restarts" << ": " << st.restarts << "\n";
	cerr << "\n";
	printLpStats();
	cerr << "\n";
	uint32 other = st.native[0] - st.native[1] - st.native[2];
	cerr << left << setw(12)
		<< "Variables" << ": " << setw(6) << solver.numVars()
		<< "(Eliminated: " << right << setw(4) << solver.numEliminatedVars() << ")" << "\n";
	cerr << left << setw(12)
		<< "Constraints" << ": " << setw(6) << st.native[0]
		<< "(Binary: " << right << setw(4) << percent(st.native, 1) << "% "
		<< "Ternary: " << right << setw(4) << percent(st.native, 2) << "% "
		<< "Other: " << right << setw(4) << percent(other, st.native[0]) << "%)"  <<"\n";
	other = st.learnt[0] - st.learnt[1] - st.learnt[2];
	cerr << left << setw(12)
		<< "Lemmas" << ": " << setw(6) << st.learnt[0]
		<< "(Binary: " << right << setw(4) << percent(st.learnt, 1) << "% "
		<< "Ternary: " << right << setw(4) << percent(st.learnt, 2) << "% "
		<< "Other: " << right << setw(4) << percent(other, st.learnt[0]) << "%)"  <<"\n";
	cerr << left << setw(12)
		<< "  Conflicts" << ": " << setw(6) << st.learnt[0] - st.loops
		<<"(Average Length: " << compAverage(st.lits[0], st.learnt[0] - st.loops) << ")\n";
	cerr << left << setw(12)
		<< "  Loops" << ": " << setw(6) << st.loops
		<< "(Average Length: " << compAverage(st.lits[1], st.loops) << ")\n";

#if MAINTAIN_JUMP_STATS == 1
	cerr << "\n";
	cerr << "Backtracks          : " << st.conflicts - st.jumps << "\n";
	cerr << "Backjumps           : " << st.jumps << "( Bounded: " << st.bJumps << " )\n";
	cerr << "Skippable Levels    : " << st.jumpSum << "\n";
	cerr << "Levels skipped      : " << st.jumpSum - st.boundSum << "(" << 100 *((st.jumpSum - st.boundSum) / std::max(1.0, (double)st.jumpSum)) << "%)" << "\n";
	cerr << "Max Jump Length     : " << st. maxJump << "( Executed: " << st.maxJumpEx << " )\n";
	cerr << "Max Bound Length    : " << st.maxBound << "\n";
	cerr << "Average Jump Length : " << st.jumpSum / std::max(1.0, (double)st.jumps)
		<< "( Executed: " <<(st.jumpSum - st.boundSum) / std::max(1.0, (double)st.jumps) << " )\n";
	cerr << "Average Bound Length: " <<(st.boundSum) / std::max(1.0, (double)st.bJumps) << "\n";
	cerr << "Average Model Length: " <<(st.modLits) / std::max(1.0, (double)st.models) << "\n";
#endif
	cerr << endl;
}

bool MainApp::runClasp()
{
	if(options.seed != -1)
	{
		Clasp::srand(options.seed);
	}

	mode = options.dimacs ? SAT_MODE : ASP_MODE;
	if(options.satPreParams[0] != 0)
	{
		// enable and configure the sat preprocessor
		SatElite::SatElite * pre = new SatElite::SatElite(solver);
		pre->options.maxIters = options.satPreParams[0];
		pre->options.maxOcc = options.satPreParams[1];
		pre->options.maxTime = options.satPreParams[2];
		pre->options.elimPure = options.numModels == 1;
		pre->options.verbose = options.quiet ? 0 : printSatEliteProgress;
		solver.strategies().satPrePro.reset(pre);
	}
	t_all.start();

	bool more;
#	ifdef WITH_ICLASP
	if(options.outf == Options::ICLASP_OUT && !options.claspMode)
		more = solveIncremental();
	else
		more = solve();
#	else
	more = solve();
#	endif
	t_all.stop();
	if(mode == ASP_MODE)
	{
		printAspStats(more);
	}
	else
	{
		cerr << "s " <<(solver.stats.models != 0 ? "SATISFIABLE" : "UNSATISFIABLE") << "\n";
		printSatStats();
	}
	return solver.stats.models != 0 ? S_SATISFIABLE : S_UNSATISFIABLE;
}

#	ifdef WITH_ICLASP
bool MainApp::solveIncremental()
{
	setState(start_read);
	Streams s;
	getStreams(options, s);

	ProgramBuilder api;
	enum_ = 0;
	IClaspOutput output(&api, options.shift);
	Grounder     grounder(options.grounderOptions);
	LparseParser parser(&grounder, s.streams);

	preStats_.reset(new PreproStats());
	atomIndex_.reset(new AtomIndex());
  api.setExtendedRuleMode(ProgramBuilder::ExtendedRuleMode(options.transExt));
	api.startProgram(*atomIndex_, options.suppModels
		? 0
		: new DefaultUnfoundedCheck(DefaultUnfoundedCheck::ReasonStrategy(options.loopRep))
		, (uint32)options.eqIters
	);

	if(!parser.parse(&output))
		throw NS_GRINGO::GrinGoException("Error: Parsing failed.");

	bool more = false;
	bool ret  = false;
	steps = 0;
	solver.strategies().heuristic->reinit(options.keepHeuristic);
	grounder.prepare(true);
	setState(end_read);

	struct SStats
	{
		uint64 restarts;
		uint64 choices;
		uint64 conflicts;
		SStats() : restarts(0), choices(0), conflicts(0) {}
	} sstats;

	struct IStats
	{
		uint32 models;
		double ttotal;
		double tground;
		double tpre;
		double tsolve;
		uint32 rules;
		IStats() : models(0), ttotal(0), tground(0), tpre(0), tsolve(0), rules(0) {}
		void print(uint32 models, double ttotal, double tground, double tpre, double tsolve, uint32 rules, uint64 choices, uint64 conflicts)
		{
			cerr << endl;
			cerr << "Models   : " << models - this->models << endl;
			cerr << fixed << setprecision(3) << "Time     : " << (ttotal - this->ttotal)
				<< " (g: " << (tground - this->tground)
				<< ", p: " << (tpre - this->tpre)
				<< ", s: " << (tsolve - this->tsolve) << ")" << endl;
			cerr << "Rules    : " << (rules - this->rules) << endl;
			cerr << "Choices  : " << choices << endl;
			cerr << "Conflicts: " << conflicts << endl;
			this->models    = models;
			this->ttotal    = ttotal;
			this->tground   = tground;
			this->tpre      = tpre;
			this->tsolve    = tsolve;
			this->rules     = rules;
		}
	} istats;
	Timer all;
	StdOutPrinter *printer = 0;
	do
	{

		all.start();
		setState(start_ground);
		if(options.verbose || options.istats)
			cerr << "=============== step " << (steps + 1) << " ===============" << endl;
		if(options.verbose)
			cerr << "Grounding..." << endl;
		if(!options.keepLearnts)
			solver.reduceLearnts(1.0f);
		api.updateProgram();
		grounder.ground();
		setState(end_ground);
		setState(start_pre);
		if(options.verbose)
			cerr << "Preprocessing..." << endl;
		// Last param must be false so that Solver::endAddConstraints() is not called.
		// The enumerator may want to exclude some variables from preprocessing so we have to init it first
		ret = api.endProgram(solver, options.initialLookahead, false);
		if (enum_ == 0) {
			if (!options.cons.empty()) {
				cerr << "Warning: Computing cautious/brave consequences not supported in incremental setting!\n" << endl;
			}
			ModelEnumerator* e = !options.recordSol
				? (ModelEnumerator*)new BacktrackEnumerator(printer = new StdOutPrinter(options.quiet, false), options.projectConfig)
				: (ModelEnumerator*)new RecordEnumerator(printer = new StdOutPrinter(options.quiet, false), options.modelRestart);
			enum_ = e;
			enum_->setNumModels(options.numModels);
			solver.add(enum_);
			options.solveParams.setEnumerator( *enum_ );
		}
		static_cast<ModelEnumerator*>(enum_)->startSearch(solver, atomIndex_.get(), options.project, 0);
		// Now that enumerator is configured, finalize solver
		ret = ret && solver.endAddConstraints(options.initialLookahead);
		double r = solver.numVars() / double(solver.numConstraints());
		if (r < 0.1 || r > 10.0) {
			problemSize = std::max(solver.numVars(), solver.numConstraints());
		}
		else {
			problemSize = std::min(solver.numVars(), solver.numConstraints());
		}

		setState(end_pre);
		if(ret)
		{
			if (options.redFract == 0.0) {
				options.solveParams.setReduceParams((uint32)-1, 1.0, (uint32)-1, options.redOnRestart);
			}
			else {
				options.solveParams.setReduceParams(static_cast<uint32>(problemSize/options.redFract)
					, options.redInc
					, static_cast<uint32>(problemSize*options.redMax)
					, options.redOnRestart
				);
			}
			setState(start_solve);
			if(options.verbose)
				cerr << "Solving..." << endl;
			uint64 models = solver.stats.models;
			LitVec assumptions;
			assumptions.push_back((*atomIndex_.get())[output.getIncUid()]. lit);
			more = Clasp::solve(solver, assumptions, options.solveParams);
			ret = solver.stats.models - models > 0;
			setState(end_solve);
		}
		else
		{
			cerr << "Found top level conflict while preprocessing: program has no answer sets." << endl;
			break;
		}
		steps++;
		all.stop();
		if(options.istats)
		{
			istats.print(solver.stats.models, all, t_ground, t_pre, t_solve, api.stats.rules[0], solver.stats.choices, solver.stats.conflicts);
		}
		sstats.restarts += solver.stats.restarts;
		sstats.choices  += solver.stats.choices;
		sstats.conflicts+= solver.stats.conflicts;
		solver.stats.restarts = solver.stats.choices = solver.stats.conflicts = 0;
		if(options.numModels)
			enum_->setNumModels(options.numModels + solver.stats.models);
	}
	while(options.imax-- > 1 && (options.imin-- > 1 || ret == options.iunsat));
	printer->enableReport();
	printer->printReport(solver, *static_cast<ModelEnumerator*>(enum_));
	// for the summary
	api.stats.moveTo(*preStats_);
	solver.stats.restarts  = sstats.restarts;
	solver.stats.choices   = sstats.choices;
	solver.stats.conflicts = sstats.conflicts;
	return more;
}
#	endif

bool MainApp::solve() {
	bool res = mode == ASP_MODE ? parseLparse() : readSat();
	assert(enum_);
	enum_->setNumModels(options.numModels);
	solver.add(enum_);
	options.solveParams.setEnumerator( *enum_ );
	if (res) {
		if (options.redFract == 0.0) {
			options.solveParams.setReduceParams((uint32)-1, 1.0, (uint32)-1, options.redOnRestart);
		}
		else {
			options.solveParams.setReduceParams(static_cast<uint32>(problemSize/options.redFract)
				, options.redInc
				, static_cast<uint32>(problemSize*options.redMax)
				, options.redOnRestart
			);
		}
		setState(start_solve);
		if(options.verbose)
			cerr << (mode == ASP_MODE ? "" : "c ") << "Solving..." << endl;
		res = Clasp::solve(solver, options.solveParams);
		setState(end_solve);
	}
	else {
		enum_->endSearch(solver);
	}
	return res;
}


bool MainApp::readSat() {
	std::istream& in = getStream();
	enum_ = options.recordSol
		? (Enumerator*)new RecordEnumerator(new StdOutPrinter(options.quiet), options.modelRestart)
		: (Enumerator*)new BacktrackEnumerator(new StdOutPrinter(options.quiet), options.projectConfig);
	setState(start_read);
	bool res = parseDimacs(in, solver, options.numModels == 1);
	setState(end_read);
	if (res) {
		setState(start_pre);
		if(options.verbose)
			cerr << "c Preprocessing..." << endl;
		res = solver.endAddConstraints(options.initialLookahead);
		setState(end_pre);
	}
	problemSize = solver.numConstraints();
	return res;
}

bool MainApp::parseLparse()
{
	std::istream &in = getStream();
	preStats_.reset(new PreproStats());
	atomIndex_.reset(new AtomIndex());
	ProgramBuilder api;
	api.setExtendedRuleMode(ProgramBuilder::ExtendedRuleMode(options.transExt));
	api.startProgram(*atomIndex_, options.suppModels
		? 0
		: new DefaultUnfoundedCheck(DefaultUnfoundedCheck::ReasonStrategy(options.loopRep))
		, (uint32)options.eqIters
	);
	bool ret = true;
	if(!options.claspMode)
	{
		ClaspOutput output(&api, options.shift);
		ground(output);
	}
	else
	{
		setState(start_read);
		LparseReader reader;
		ret = reader.parse(in, api);
		setState(end_read);
	}
	if ( (api.hasMinimize() || !options.cons.empty()) && options.numModels != 0) {
		options.cons.empty()
			? cerr << "Warning: Option '--number' is ignored when computing optimal solutions!" << endl
			: cerr << "Warning: Option '--number' is ignored when computing consequences!" << endl;
	}
	if (api.hasMinimize() && !options.cons.empty()) {
		cerr << "Warning: Minimize statements: Consequences may depend on enumeration order!" << endl;
	}
	MinimizeConstraint* c = 0;
	if (ret) {
		setState(start_pre);
		if(options.verbose)
			cerr << "Preprocessing..." << endl;
		ret = api.endProgram(solver, options.initialLookahead, false);
		api.stats.moveTo(*preStats_);
		if (ret && api.hasMinimize()) {
			c = api.createMinimize(solver);
			c->setMode( options.optAll ? MinimizeConstraint::compare_less_equal : MinimizeConstraint::compare_less );
			if (!options.optVals.empty()) {
				uint32 m = std::min((uint32)options.optVals.size(), c->numRules());
				for (uint32 x = 0; x != m; ++x) {
					c->setOptimum(x, options.optVals[x]);
				}
			}
		}
	}
	if (!options.cons.empty()) {
		enum_ = new CBConsequences(solver, atomIndex_.get(),
			options.cons == "brave" ? CBConsequences::brave_consequences : CBConsequences::cautious_consequences, c,
			new StdOutPrinter(options.quiet), options.modelRestart
		);
	}
	else {
		ModelEnumerator* e = !options.recordSol
			? (ModelEnumerator*)new BacktrackEnumerator(new StdOutPrinter(options.quiet), options.projectConfig)
			: (ModelEnumerator*)new RecordEnumerator(new StdOutPrinter(options.quiet), options.modelRestart);
		e->startSearch(solver, atomIndex_.get(), options.project, c);
		enum_ = e;
	}
	ret = ret && solver.endAddConstraints(options.initialLookahead);
	double r = solver.numVars() / double(solver.numConstraints());
	if (r < 0.1 || r > 10.0) {
		problemSize = std::max(solver.numVars(), solver.numConstraints());
	}
	else {
		problemSize = std::min(solver.numVars(), solver.numConstraints());
	}
	setState(end_pre);
	return ret;
}

#endif

int main(int argc, char **argv)
{
	try
	{
		signal(SIGINT, sigHandler);	// Ctrl + C
		signal(SIGTERM, sigHandler);	// kill(but not kill -9)
		return clasp_g.run(argc, argv);
	}
#ifdef WITH_CLASP
	catch(const ReadError &e)
	{
		cerr << "\nError(" << e.line_ << "): " << e.
			what() << endl;
		return S_ERROR;
	}
#endif
	catch(const GrinGoException &e)
	{
		cerr << "\ngringo " << e.what() << endl;
		return S_ERROR;
	}
	catch(const std::bad_alloc &)
	{
		cerr << "\nclasp ERROR: out of memory" << endl;
		return S_MEMORY;
	}
	catch(const std::exception &e)
	{
		cerr << "\nclasp ERROR: " << e.what() << endl;
		return S_ERROR;
	}
	return S_UNKNOWN;
}

