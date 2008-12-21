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

#	include "enumerator.h"
#	include <clasp/include/lparse_reader.h>
#	include <clasp/include/satelite.h>
#	include <clasp/include/unfounded_check.h>
#endif

#if defined(_MSC_VER) &&_MSC_VER >= 1200
//#define CHECK_HEAP 1;
#	include <crtdbg.h>
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
	CTimer    t_pre;
	CTimer    t_read;
	CTimer    t_ground;
	CTimer    t_solve;
	CTimer    t_all;
#ifdef WITH_ICLASP
	int       steps;
#endif
#ifdef WITH_CLASP
	Solver    solver;

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
	std::auto_ptr < LparseStats > lpStats_;
	std::auto_ptr < PreproStats > preStats_;
	std::auto_ptr < Enumerator > enum_;
	std::ifstream inFile_;
#endif
} clasp_g;

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
	clasp_g.t_all.Stop();
	printf("%sTime      : %s\n",(satM ? "c " : ""), clasp_g.t_all.Print().c_str());
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
		t_read.Start();
		break;
	case end_read:
		t_read.Stop();
		break;
	case start_ground:
		t_ground.Start();
		break;
	case end_ground:
		t_ground.Stop();
		break;
	case start_pre:
		t_pre.Start();
		break;
	case end_pre:
		t_pre.Stop();
		break;
	case start_solve:
		t_solve.Start();
		break;
	case end_solve:
		t_solve.Stop();
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
				s.streams.push_back(new std::fstream(i->c_str()));
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
	cerr << "c Time         : " << t_all.Print() << "s(Solve: " << t_solve.Print() << "s)\n";
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
	const PreproStats &ps = *preStats_;
	const LparseStats &lp = *lpStats_;
	const Options &o = options;
	uint32 r = lp.rules[0] == 0 
		? std::accumulate(lp.rules, lp.rules + OPTIMIZERULE + 1, 0) 
		: lp.rules[0] + lp.rules[1] + lp.rules[OPTIMIZERULE];
	uint32 a = lp.atoms[0] + lp.atoms[1];
	cerr << left << setw(12) << "Atoms" << ": " << setw(6) << a;
	if(lp.atoms[1] != 0)
	{
		cerr << "(Original: " << lp.atoms[0] << " Auxiliary: " << lp.atoms[1] << ")";
	}
	cerr << "\n";
	cerr << left << setw(12) << "Rules" << ": " << setw(6) << r << "(";
	for(int i = 1; i != OPTIMIZERULE; ++i)
	{
		if(lp.rules[i] != 0)
		{
			if(i != 1)
				cerr << " ";
			cerr << i << ": " << lp.rules[i];
			if(lp.rules[0] != 0)
			{
				cerr << "/";
				if(i == 1)
					cerr << lp.rules[1] + lp.rules[0];
				
				else if((i == 3 &&(o.transExt &LparseReader::transform_choice) != 0))
					cerr << 0;
				else if((i == 2 || i == 5) &&(o.transExt &LparseReader::transform_weight) != 0)
					cerr << 0;
				else
					cerr << lp.rules[i];
			}
		}
	}
	if(lp.rules[OPTIMIZERULE] != 0)
	{
		cerr << " " << OPTIMIZERULE << ": " << lp.rules[OPTIMIZERULE];
	}
	cerr << ")" << "\n";
	cerr << left << setw(12) << "Bodies" << ": " << ps.bodies << "\n";
	if(ps.numEqs() != 0)
	{
		cerr << left << setw(12) << "Equivalences" << ": " << setw(6) << ps.numEqs() 
			<< "(Atom=Atom: " << ps.numEqs(Var_t::atom_var)  
			<< " Body=Body: " << ps.numEqs(Var_t::body_var) 
			<< " Other: " << (ps.numEqs() - ps.numEqs(Var_t::body_var) - ps.numEqs(Var_t::atom_var)) <<")" << "\n";
	}
	cerr << left << setw(12) << "Tight" << ": ";
	if(!o.suppModels)
	{
		if(ps.sccs == 0)
		{
			cerr << "Yes";
		}
		else
		{
			cerr << setw(6) << "No" << "(SCCs: " << ps.sccs << " Nodes: " << ps.ufsNodes << ")";
		}
	}
	else
	{
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
	uint64 enumerated = st.models;
	uint64 models = enum_.get() != 0 ? enum_->numModels(solver) : enumerated;
	cerr << left << setw(12) << "Models" << ": ";
	if(more)
	{
		char buf[64];
		int wr = sprintf(buf, "%llu", models);
		buf[wr] = '+';
		buf[wr + 1] = 0;
		cerr << setw(6) << buf;
	}
	else
	{
		cerr << setw(6) << models;
	}
	if(enumerated != models &&options.stats)
	{
		cerr << "(Enumerated: " << enumerated << ")";
	}
	cerr << "\n";
#ifdef WITH_ICLASP
	if(!options.claspMode && options.outf == Options::ICLASP_OUT)
		cerr << "Total Steps : " << steps << std::endl;
#endif
	cerr << left << setw(12) << "Time" << ": " << setw(6) << t_all.Print() << "\n";
	if(!options.stats)
		return;
	cerr << "  Reading   : " << t_read.Print() << "\n";
	if(!options.claspMode && !options.convert)
		cerr << "  Grounding : " << t_ground.Print() << "\n";
	cerr << "  Prepro.   : " << t_pre.Print() << "\n";
	cerr << "  Solving   : " << t_solve.Print() << "\n";
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
	t_all.Start();

	bool more;
#	ifdef WITH_ICLASP
	if(options.outf == Options::ICLASP_OUT && !options.claspMode)
		more = solveIncremental();
	else
		more = solve();
#	else
	more = solve();
#	endif
	t_all.Stop();
	if(enum_.get())
	{
		enum_.get()->report(solver);
	}
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
	IClaspOutput output(&api, LparseReader::TransformMode(options. transExt), options.shift);
	Grounder     grounder(options.grounderOptions);
	LparseParser parser(&grounder, s.streams);

	lpStats_.reset(new LparseStats());
	preStats_.reset(new PreproStats());

	api.startProgram(options.suppModels 
		? 0 
		: new DefaultUnfoundedCheck (DefaultUnfoundedCheck:: ReasonStrategy(options.loopRep)), 
		(uint32) options.eqIters);
	
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

	StdOutPrinter printer;
	if(!options.quiet)
	{
		printer.index = &api.stats.index;
		options.solveParams.enumerator()->setPrinter(&printer);
	}

	CTimer all;
	do 
	{
		all.Start();
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
		ret = api.endProgram(solver, options.initialLookahead, true);
		setState(end_pre);
		if(ret) 
		{
			setState(start_solve);
			if(options.verbose)
				cerr << "Solving..." << endl;
			uint64 models = solver.stats.models;
			LitVec assumptions;
			assumptions.push_back(api.stats.index[output.getIncUid()]. lit);
			more = Clasp::solve(solver, assumptions, options.numModels, options.solveParams);
			ret = solver.stats.models - models > 0;
			setState(end_solve);
		}
		else
		{
			cerr << "Found top level conflict while preprocessing: program has no answer sets." << endl;
			break;
		}
		steps++;
		all.Stop();
		if(options.istats)
		{
			uint32 r = output.getStats().rules[0] == 0
		                ? std::accumulate(output.getStats().rules, output.getStats().rules + OPTIMIZERULE + 1, 0)
				: output.getStats().rules[0] + output.getStats().rules[1] + output.getStats().rules[OPTIMIZERULE];
			istats.print(solver.stats.models, all, t_ground, t_pre, t_solve, r, solver.stats.choices, solver.stats.conflicts);
		}
		sstats.restarts += solver.stats.restarts;
		sstats.choices  += solver.stats.choices;
		sstats.conflicts+= solver.stats.conflicts;
		solver.stats.restarts = solver.stats.choices = solver.stats.conflicts = 0;
	}
	while(options.imax-- > 1 && (options.imin-- > 1 || ret == options.iunsat));
	// for the summary
	*lpStats_ = output.getStats();
	api.stats.moveTo(*preStats_);
	solver.stats.restarts  = sstats.restarts;
	solver.stats.choices   = sstats.choices;
	solver.stats.conflicts = sstats.conflicts;
	return more;
}
#	endif

bool MainApp::solve()
{
	bool res = mode == ASP_MODE ? parseLparse() : readSat();
	if(res)
	{
		StdOutPrinter printer;
		if(!options.quiet)
		{
			if(mode == ASP_MODE)
				printer.index = &preStats_->index;
			options.solveParams.enumerator()->
				setPrinter(&printer);
		}
		setState(start_solve);
		if(options.verbose)
			cerr << (mode == ASP_MODE ? "" : "c ") << "Solving..." << endl;
		res = Clasp::solve(solver, options.numModels, options.solveParams);
		setState(end_solve);
	}
	return res;
}

bool MainApp::readSat()
{
	std::istream &in = getStream();
	setState(start_read);
	bool res = parseDimacs(in, solver, options.numModels == 1);
	setState(end_read);
	if(res)
	{
		setState(start_pre);
		if(options.verbose)
			cerr << "c Preprocessing..." << endl;
		res = solver.endAddConstraints(options.initialLookahead);
		setState(end_pre);
	}
	return res;
}

bool MainApp::parseLparse()
{
	std::istream &in = getStream();
	lpStats_.reset(new LparseStats());
	preStats_.reset(new PreproStats());
	
	ProgramBuilder api;
	api.startProgram(options.suppModels 
		? 0 
		: new DefaultUnfoundedCheck(DefaultUnfoundedCheck::ReasonStrategy(options.  loopRep)), 
		(uint32) options.eqIters );
	
	if(!options.claspMode)
	{
		ClaspOutput output(&api, LparseReader::TransformMode(options. transExt), options.shift);
		ground(output);
		*lpStats_ = output.getStats();
	}
	else
	{
		setState(start_read);
		LparseReader reader;
		reader.setTransformMode(LparseReader::TransformMode(options.transExt));
		if(!reader.parse(in, api))
			return false;
		*lpStats_ = reader.stats;
		setState(end_read);
	}

	if(api.hasMinimize() || !options.cons.empty())
	{
		if(api.hasMinimize() &&!options.cons.empty())
		{
			cerr << "Warning: Optimize statements are ignored because of '" << options.cons << "'!" << endl;
		}
		if(options.numModels != 0)
		{
			!options.cons.empty() 
				? cerr << "Warning: For computing consequences clasp must be called with '--number=0'!" << endl  
				: cerr << "Warning: For computing optimal solutions clasp must be called with '--number=0'!" << endl;
		}
	}

	setState(start_pre);
	if(options.verbose)
		cerr << "Preprocessing..." << endl;
	bool ret = api.endProgram(solver, options.initialLookahead, false);
	api.stats.moveTo(*preStats_);
	if(!options.cons.empty())
	{
		enum_.reset(new CBConsequences(solver, &preStats_->index,
			options.cons == "brave" ? CBConsequences::brave_consequences : CBConsequences::cautious_consequences, 
			options.quiet));
		options.solveParams.setEnumerator(*enum_);
	}
	
	else if(api.hasMinimize())
	{
		MinimizeConstraint * c = api.createMinimize(solver);
		c->setMode(MinimizeConstraint::Mode(options.optimize &1), options.optimize == 2);
		if(!options.optVals.empty())
		{
			uint32 m = std::min((uint32) options.optVals.size(), c->numRules());
			for(uint32 x = 0; x != m; ++x)
			{
				c->setOptimum(x, options.optVals[x]);
			}
		}
		enum_.reset(new MinimizeEnumerator(c));
		options.solveParams.setEnumerator(*enum_);
	}
	ret = ret && solver.endAddConstraints(options.initialLookahead);
	setState(end_pre);
	return ret;
}

#endif

int main(int argc, char **argv) 
{
#if defined(_MSC_VER) &&defined(CHECK_HEAP) &&_MSC_VER >= 1200 
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
	try
	{
		signal(SIGINT, sigHandler);	// Ctrl + C
		signal(SIGTERM, sigHandler);	// kill(but not kill -9)
		return clasp_g.run(argc, argv);
	}
#ifdef WITH_CLASP
	catch(const ReadError &e)
	{
		cerr << "Failed!\nError(" << e.line_ << "): " << e.
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

