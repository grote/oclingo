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

#include <program_opts/app_options.h>
#include <gringo/output.h>
#include <gringo/inclit.h>
#include <gringo/converter.h>
#include <gringo/streams.h>
#include "clasp/clasp_options.h"
#include "clasp/clasp_output.h"
#include "clingcon/clingcon_output.h"
#include "clasp/smodels_constraints.h"
#include "gringo/gringo_app.h"
#include "clingcon/clingcon_options.h"
#include "oclingo/oclingo_options.h"
#include "clingcon/cspoutput.h"
#include "clingcon/timer.h"
#include <clingcon/cspparser.h>
#include <clingcon/gecodesolver.h>
#include <clingcon/propagator.h>
//#include "clingcon/lua_impl.h"
#include <iomanip>

// (i)Clingcon application, i.e. gringo+clasp+gecode
template <CSPMode M>
class ClingconApp : public GringoApp, public Clasp::ClaspFacade::Callback
{
private:
	class LuaImpl;
	typedef std::auto_ptr<LuaImpl> LuaImplPtr;
	LuaImplPtr luaImpl;

public:
	/** returns a singleton instance */
	static ClingconApp& instance();
        void luaInit(Grounder &g, CSPOutput &o);
	bool luaLocked();

protected:
        Clingcon::ClingconPropagator* cp_;       // the clingcon PostPropagator
	// AppOptions interface
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden)
	{
		config_.solver = &solver_;
		cmdOpts_.setConfig(&config_);
		cmdOpts_.initOptions(root, hidden);
		clingo.initOptions(root, hidden);
                //if(M == OCLINGCON) oclingo.initOptions(root, hidden);
		generic.verbose = 1;
		GringoApp::initOptions(root, hidden);
	}

	void addDefaults(std::string& defaults)
	{
		cmdOpts_.addDefaults(defaults);
		clingo.addDefaults(defaults);
                //if(M == OCLINGCON) oclingo.addDefaults(defaults);
		defaults += "  --verbose=1";
		GringoApp::addDefaults(defaults);
	}

	bool validateOptions(ProgramOptions::OptionValues& v, Messages& m)
	{
		if (cmdOpts_.basic.timeout != -1)
			m.warning.push_back("Time limit not supported");
		return cmdOpts_.validateOptions(v, m)
			&& GringoApp::validateOptions(v, m)
			&& clingo.validateOptions(v, GringoApp::gringo, m)
                        /*&& (M != OCLINGCON || oclingo.validateOptions(v, clingo.claspMode, clingo.clingoMode, clingo.mode, clingo.inc, m))*/;
	}
	// ---------------------------------------------------------------------------------------

	// Application interface
	void printVersion() const;
	std::string getVersion() const;
	std::string getUsage()   const { return "[number] [options] [files]"; }
	ProgramOptions::PosOption getPositionalParser() const { return &Clasp::parsePositional; }
	void handleSignal(int sig);
	int  doRun();
	// -------------------------------------------------------------------------------------------

	// ClaspFacade::Callback interface
	void state(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f);
	void event(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f);
        virtual void doEvent(Clasp::ClaspFacade::Event, Clasp::ClaspFacade&) { }
	void warning(const char* msg) { messages.warning.push_back(msg); }
	// -------------------------------------------------------------------------------------------

	enum ReasonEnd { reason_timeout, reason_interrupt, reason_end };
	enum { numStates = Clasp::ClaspFacade::num_states };
	void printResult(ReasonEnd re);
	void configureInOut(Streams& s);

	typedef std::auto_ptr<Clasp::OutputFormat> ClaspOutPtr;
        typedef std::auto_ptr<Clasp::Input> ClaspInPtr;

	Clasp::Solver          solver_;           // solver to use for search
	Clasp::SolveStats      stats_;            // accumulates clasp solve stats in incremental setting
	Clasp::ClaspConfig     config_;           // clasp configuration - from command-line
	Clasp::ClaspOptions    cmdOpts_;          // clasp basic options - from command-line
	Timer                  timer_[numStates]; // one timer for each state
	ClaspOutPtr            out_;              // printer for printing result of search
	ClaspInPtr             in_;               // input for clasp
	Clasp::ClaspFacade*    facade_;           // interface to clasp lib
        Clingcon::CSPSolver*   cspsolver_;
public:
	ClingconOptions<M> clingo;                  // (i)clingo options   - from command-line
//	oClingoOptions<M> oclingo;                 // oclingo options     - from command-line
};

template <CSPMode M>
struct CSPFromGringo : public Clasp::Input
{
	typedef std::auto_ptr<Grounder>    GrounderPtr;
	typedef std::auto_ptr<Storage>     StoragePtr;
	typedef std::auto_ptr<CSPParser>   ParserPtr;
	typedef std::auto_ptr<Converter>   ConverterPtr;
        typedef std::auto_ptr<CSPOutput> OutputPtr;
	typedef Clasp::MinimizeConstraint* MinConPtr;

        CSPFromGringo(ClingconApp<M> &app, Streams& str, Clingcon::CSPSolver*);
        void otherOutput();
	Format format() const { return Clasp::Input::SMODELS; }
	MinConPtr getMinimize(Clasp::Solver& s, Clasp::ProgramBuilder* api, bool heu) { return api ? api->createMinimize(s, heu) : 0; }
	void getAssumptions(Clasp::LitVec& a);
	bool read(Clasp::Solver& s, Clasp::ProgramBuilder* api, int);
	void release();

	ClingconApp<M>           &app;
	GrounderPtr            grounder;
	StoragePtr             storage;
	ParserPtr              parser;
	ConverterPtr           converter;
	OutputPtr              out;
	IncConfig              config;
	Clasp::Solver*         solver;
        Clingcon::CSPSolver*   cspsolver_;
};

#ifdef WITH_LUA
#	include "clingcon/lua_impl.h"
#else

template <CSPMode M>
class ClingconApp<M>::LuaImpl
{
public:
        LuaImpl(Grounder *, Clasp::Solver *, CSPOutput *) { }
	bool locked() { return false; }
	void onModel() { }
	void onBeginStep() { }
	void onEndStep() { }
};

#endif

/////////////////////////////////////////////////////////////////////////////////////////
// FromGringo
/////////////////////////////////////////////////////////////////////////////////////////

template <CSPMode M>
void CSPFromGringo<M>::otherOutput()
{
	assert(false);
}


template <CSPMode M>
CSPFromGringo<M>::CSPFromGringo(ClingconApp<M> &a, Streams& str, Clingcon::CSPSolver* cspsolver)
	: app(a)
{
	assert(app.clingo.mode == CLINGCON || app.clingo.mode == ICLINGCON || app.clingo.mode == OCLINGCON);
	if (app.clingo.mode == CLINGCON)
	{
                out.reset(new CSPOutput(app.gringo.disjShift, cspsolver));
	}
	else if(app.clingo.mode == ICLINGCON)
	{
                out.reset(new iCSPOutput(app.gringo.disjShift, cspsolver));
	}
	else { otherOutput(); }
	if(app.clingo.mode == CLINGCON && app.gringo.groundInput)
	{
		storage.reset(new Storage(out.get()));
		converter.reset(new Converter(out.get(), str));
	}
	else
	{
		bool inc = app.clingo.mode != CLINGCON || app.gringo.ifixed > 0;
		grounder.reset(new Grounder(out.get(), app.generic.verbose > 2, app.gringo.termExpansion(config), app.gringo.heuristics.heuristic));
		a.createModules(*grounder);
		parser.reset(new CSPParser(grounder.get(), a.base_, a.cumulative_, a.volatile_, config, str, app.gringo.compat, inc));
	}
}

template <CSPMode M>
void CSPFromGringo<M>::getAssumptions(Clasp::LitVec& a)
{
	if(M == ICLINGCON && app.clingo.mode == ICLINGCON)
	{
		const Clasp::AtomIndex& i = *solver->strategies().symTab.get();
		a.push_back(i.find(out->getIncAtom())->lit);
	}
}

template <CSPMode M>
bool CSPFromGringo<M>::read(Clasp::Solver& s, Clasp::ProgramBuilder* api, int)
{
	assert(out.get());
	out->setProgramBuilder(api);
	solver = &s;
	out->initialize();
	if(converter.get())
	{
		converter->parse();
		converter.reset(0);
	}
	else
	{
		if (parser.get())
		{
			parser->parse();

			if(app.gringo.magic) grounder->addMagic();
			grounder->analyze(app.gringo.depGraph, app.gringo.stats);
			parser.reset(0);
			app.luaInit(*grounder, *out);
			app.groundBase(*grounder, config, 1, app.clingo.mode == CLINGCON ? app.gringo.ifixed : 1, app.clingo.mode == CLINGCON ? app.gringo.ifixed : app.clingo.inc.iQuery);
		}
		else
		{
			config.incStep++;
			app.groundStep(*grounder, config, config.incStep, app.clingo.inc.iQuery);
		}
	}
	out->finalize();
	release();
	return true;
}

template <CSPMode M>
void CSPFromGringo<M>::release()
{
	if (app.clingo.mode == CLINGCON && !app.luaLocked())
	{
		grounder.reset(0);
		out.reset(0);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// ClingconApp
/////////////////////////////////////////////////////////////////////////////////////////

template <CSPMode M>
ClingconApp<M> &ClingconApp<M>::instance()
{
	static ClingconApp<M> app;
	return app;
}

template <CSPMode M>
void ClingconApp<M>::printVersion() const
{
	using namespace std;
	GringoApp::printVersion();
	cout << endl;
	cout << "clasp " << CLASP_VERSION << "\n";
	cout << "Copyright (C) Benjamin Kaufmann" << "\n";
	cout << "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n";
	cout << "clasp is free software: you are free to change and redistribute it.\n";
	cout << "There is NO WARRANTY, to the extent permitted by law." << endl;
}

template <CSPMode M>
std::string ClingconApp<M>::getVersion() const {
	std::string r(GRINGO_VERSION);
	r += " (clasp ";
	r += CLASP_VERSION;
	r += ")";
	return r;
}

template <CSPMode M>
void ClingconApp<M>::handleSignal(int) {
	for(int i = 0; i != sizeof(timer_)/sizeof(Timer); ++i)
		timer_[i].stop();
	fprintf(stderr, "\n*** INTERRUPTED! ***\n");
	if(facade_ && facade_->state() != Clasp::ClaspFacade::state_not_started)
		printResult(ClingconApp::reason_interrupt);
	_exit(solver_.stats.solve.models != 0 ?  S_SATISFIABLE : S_UNKNOWN);
}

template <CSPMode M>
void ClingconApp<M>::configureInOut(Streams& s)
{
	using namespace Clasp;
	in_.reset(0);
	facade_ = 0;
	if(clingo.mode == CSPCLASP)
	{
		s.open(generic.input);
		if (generic.input.size() > 1) { messages.warning.push_back("Only first file will be used"); }
		in_.reset(new StreamInput(s.currentStream(), detectFormat(s.currentStream())));
	}
	else
	{

		s.open(generic.input, constStream());
                cspsolver_ = new Clingcon::GecodeSolver(true,false,clingo.numAS.second,clingo.numAS.first,clingo.cspICL,clingo.cspBranchVar,clingo.cspBranchVal);
                in_.reset(new CSPFromGringo<M>(*this, s,cspsolver_));

                                                                           /*clingcon_.cspLazyLearn,
                                                                           clingcon_.cspCDG,
                                                                           clingcon_.cspWeakAS,
                                                                           clingcon_.cspNumAS,
                                                                           clingcon_.cspICL,
                                                                           clingcon_.cspBranchVar,
                                                                           clingcon_.cspBranchVal
                                                                           );*/
                //dynamic_cast<FromGringo*>(in_.get())->grounder->setCSPSolver(cspsolver_);

	}
	if(config_.onlyPre)
	{
		if(clingo.mode == CSPCLASP || clingo.mode == CLINGCON) { generic.verbose = 0; }
		else { warning("Option '--pre' is ignored in incremental setting!"); config_.onlyPre = false; }
	}
	if(in_->format() == Input::SMODELS)
	{
                out_.reset(new Clingcon::ClingconOutput(cmdOpts_.basic.asp09,cspsolver_));
		if(cmdOpts_.basic.asp09) { generic.verbose = 0; }
	}
	else if(in_->format() == Input::DIMACS) { out_.reset(new SatOutput()); }
	else if(in_->format() == Input::OPB)    { out_.reset(new PbOutput(generic.verbose > 1));  }
}

template <CSPMode M>
void ClingconApp<M>::luaInit(Grounder &g, CSPOutput &o)
{
	luaImpl.reset(new LuaImpl(&g, &solver_, &o));
}

template <CSPMode M>
bool ClingconApp<M>::luaLocked()
{
	return luaImpl.get() ? luaImpl->locked() : false;
}

template <CSPMode M>
int ClingconApp<M>::doRun()
{
	using namespace Clasp;
        if (gringo.groundOnly)
        {
            std::auto_ptr<Output> o(output());
            Streams  inputStreams(generic.input, constStream());
            if(gringo.groundInput)
            {
    #pragma message "reimplement me!!!"
                    /*
                    Storage   s(o.get());
                    Converter c(o.get(), inputStreams);

                    (void)s;
                    o->initialize();
                    c.parse();
                    o->finalize();
                    */
            }
            else
            {
                    IncConfig config;
                    Grounder  g(o.get(), generic.verbose > 2, gringo.termExpansion(config), gringo.heuristics.heuristic);
                    createModules(g);
                    CSPParser    p(&g, base_, cumulative_, volatile_, config, inputStreams, gringo.compat, gringo.ifixed > 0);

                    o->initialize();
                    p.parse();
                    if(gringo.magic) g.addMagic();
                    g.analyze(gringo.depGraph, gringo.stats);
                    groundBase(g, config, 1, gringo.ifixed, gringo.ifixed);
                    o->finalize();
            }

            return EXIT_SUCCESS;
        }

	if (cmdOpts_.basic.stats > 1) { 
		solver_.stats.solve.enableJumpStats(); 
		stats_.enableJumpStats();
	}
	Streams s;
	configureInOut(s);
	ClaspFacade clasp;
	facade_ = &clasp;
	timer_[0].start();
	if (clingo.mode == CSPCLASP || clingo.mode == CLINGCON)
	{
		clingo.iStats = false;
		clasp.solve(*in_, config_, this);
	}
	else { clasp.solveIncremental(*in_, config_, clingo.inc, this); }
	timer_[0].stop();
	printResult(reason_end);
	if      (clasp.result() == ClaspFacade::result_unsat) { return S_UNSATISFIABLE; }
	else if (clasp.result() == ClaspFacade::result_sat)   { return S_SATISFIABLE; }
	else                                                  { return S_UNKNOWN; }
}

#define STATUS(v1,x) if (generic.verbose<v1);else (x)

template <CSPMode M>
void ClingconApp<M>::state(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f) {
	using namespace Clasp;
	using namespace std;
	if (e == ClaspFacade::event_state_enter)
	{
		MainApp::printWarnings();
		if (f.state() == ClaspFacade::state_read)
		{
			if (f.step() == 0)
			{
				STATUS(2, cout << getExecutable() << " version " << getVersion() << "\n");
			}
			if (clingo.iStats)
			{
				cout << "=============== step " << f.step()+1 << " ===============" << endl;
			}
		}
		else if (f.state() == ClaspFacade::state_solve)
		{
			STATUS(2, cout << "Solving...\n");
			if(luaImpl.get()) { luaImpl->onBeginStep(); }
		}
		cout << flush;
		timer_[f.state()].start();

	}
	else if (e == ClaspFacade::event_state_exit)
	{
		timer_[f.state()].stop();
		if (generic.verbose > 1)
		{
			if(f.state() == ClaspFacade::state_read)
			{
				STATUS(2, cout << "Reading      : " << fixed << setprecision(3) << timer_[f.state()].elapsed() << endl);
			}
			else if(f.state() == ClaspFacade::state_preprocess)
			{
				STATUS(2, cout << "Preprocessing: " << fixed << setprecision(3) << timer_[f.state()].elapsed() << endl);
			}
		}
		if (f.state() == ClaspFacade::state_solve)
		{
			stats_.accu(solver_.stats.solve);
			if (clingo.iStats)
			{
				timer_[0].lap();
				cout << "\nModels   : " << solver_.stats.solve.models << "\n"
						 << "Time     : " << fixed << setprecision(3) << timer_[0].elapsed() << " (g: " << timer_[ClaspFacade::state_read].elapsed()
						 << ", p: " << timer_[ClaspFacade::state_preprocess].elapsed() << ", s: " << timer_[ClaspFacade::state_solve].elapsed() << ")\n"
						 << "Rules    : " << f.api()->stats.rules[0] << "\n"
						 << "Choices  : " << solver_.stats.solve.choices   << "\n"
						 << "Conflicts: " << solver_.stats.solve.conflicts << "\n";
			}
			if(luaImpl.get()) { luaImpl->onEndStep(); }
			solver_.stats.solve.reset();
		}
	}
}

template <CSPMode M>
void ClingconApp<M>::event(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f)
{
	using namespace std;
	using namespace Clasp;
	if (e == ClaspFacade::event_model)
	{
		if (!cmdOpts_.basic.quiet)
		{
			if ( !(config_.enumerate.consequences()) )
			{
				STATUS(1, cout << "Answer: " << solver_.stats.solve.models << endl);
				out_->printModel(solver_, *config_.solve.enumerator());
			}
			else
			{
				STATUS(1, cout << config_.enumerate.cbType() << " consequences:" << endl);
				out_->printConsequences(solver_, *config_.solve.enumerator());
			}
			if (config_.solve.enumerator()->minimize())
			{
				out_->printOptimize(*config_.solve.enumerator()->minimize());
			}
		}
		if(luaImpl.get()) { luaImpl->onModel(); }
	}
	else if (e == ClaspFacade::event_p_prepared)
	{
		if (config_.onlyPre)
		{
			if (f.api()) f.releaseApi(); // keep api so that we can later print the program
			else { STATUS(0, cout << "Vars: " << solver_.numVars() << " Constraints: " <<  solver_.numConstraints()<<endl); }
			AtomIndex* x = solver_.strategies().symTab.release();
			solver_.reset(); // release constraints and strategies - no longer needed
			solver_.strategies().symTab.reset(x);
		}
                else
                {
                    cspsolver_->setSolver(&solver_);
                    cp_ = new Clingcon::ClingconPropagator(cspsolver_);
                    solver_.addPost(cp_);
                    //cspsolver_->setDomain((dynamic_cast<FromGringo*>(in_.get()))->grounder->getCSPDomain().first,
                    //                         (dynamic_cast<FromGringo*>(in_.get()))->grounder->getCSPDomain().second);
                    //initialize cspsolver
                    cspsolver_->initialize();
                    //in_.release();
                    out_->initSolve(solver_, f.api(), f.config()->solve.enumerator());
                }
        }

	doEvent(e, f);
}

template <CSPMode M>
void ClingconApp<M>::printResult(ReasonEnd end)
{
	using namespace std;
	using namespace Clasp;
	if (config_.onlyPre)
	{
		if (end != reason_end) { return; }
		if (facade_->api())
		{
			facade_->result() == ClaspFacade::result_unsat
				? (void)(std::cout << "0\n0\nB+\n1\n0\nB-\n1\n0\n0\n")
				: facade_->api()->writeProgram(std::cout);
			delete facade_->releaseApi();
		}
		else
		{
			if (facade_->result() != ClaspFacade::result_unsat)
			{
				STATUS(0, cout << "Search not started because of option '--pre'!" << endl);
			}
			cout << "S UNKNWON" << endl;
		}
		return;
	}
	bool complete        = end == reason_end && !facade_->more();
	Solver& s            = solver_;
	s.stats.solve.accu(stats_);
	const Enumerator& en = *config_.solve.enumerator();
	if (clingo.iStats) { cout << "=============== Summary ===============" << endl; }
	out_->printSolution(s, en, complete);
	if (cmdOpts_.basic.quiet && config_.enumerate.consequences() && s.stats.solve.models != 0)
	{
		STATUS(1, cout << config_.enumerate.cbType() << " consequences:\n");
		out_->printConsequences(s, en);
	}
	if (generic.verbose > 0) 
	{
		const char* c= out_->format[OutputFormat::comment];
		const int   w= 12-(int)strlen(c);
		if      (end == reason_timeout)  { cout << "\n" << c << "TIME LIMIT  : 1\n"; }
		else if (end == reason_interrupt){ cout << "\n" << c << "INTERRUPTED : 1\n"; }
		uint64 enumerated = s.stats.solve.models;
		uint64 models     = enumerated;
		if      (config_.enumerate.consequences() && enumerated > 1) { models = 1; }
		else if (en.minimize())                                      { models = en.minimize()->models(); }
		cout << "\n" << c << left << setw(w) << "Models" << ": ";
		if (!complete)
		{
			char buf[64];
			int wr    = sprintf(buf, "%"PRIu64, models);
			buf[wr]   = '+';
			buf[wr+1] = 0;
			cout << setw(6) << buf << "\n";
		}
		else { cout << setw(6) << models << "\n"; }
		if (enumerated)
		{
			if (enumerated != models)
			{
				cout << c << setw(w) << "  Enumerated" << ": " << enumerated << "\n";
			}
			if (config_.enumerate.consequences())
			{
				cout << c <<"  " <<  setw(w-2) << config_.enumerate.cbType() << ": " << (complete?"yes":"unknown") << "\n";
			}
			if (en.minimize())
			{
				cout << c << setw(w) << "  Optimum" << ": " << (complete?"yes":"unknown") << "\n";
				cout << c << setw(w) << "Optimization" << ": ";
				out_->printOptimizeValues(*en.minimize());
				cout << "\n";
			}
		}
		if (facade_->step() > 0)
		{
			cout << c << setw(w) << "Total Steps" <<": " << facade_->step()+1 << endl;
		}
		cout << c << setw(w) << "Time" << ": " << fixed << setprecision(3) << timer_[0].total() << endl;
		cout << c << setw(w) << "  Prepare" << ": " << fixed << setprecision(3) << timer_[ClaspFacade::state_read].total() << endl;
		cout << c << setw(w) << "  Prepro." << ": " << fixed << setprecision(3) << timer_[ClaspFacade::state_preprocess].total() << endl;
		cout << c << setw(w) << "  Solving" << ": " << fixed << setprecision(3) << timer_[ClaspFacade::state_solve].total() << endl;
	}
	if (cmdOpts_.basic.stats) { out_->printStats(s.stats, en); }
}

#undef STATUS
