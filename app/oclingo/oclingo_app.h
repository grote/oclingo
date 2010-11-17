// Copyright (c) 2010, Torsten Grote <tgrote@uni-potsdam.de>
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

#include "clingo/clingo_app.h"
#include "oclingo/oclaspoutput.h"
#include "oclingo/externalknowledge.h"

/////////////////////////////////////////////////////////////////////////////////////////
// FromGringo
/////////////////////////////////////////////////////////////////////////////////////////

template <>
FromGringo<OCLINGO>::FromGringo(ClingoApp<OCLINGO> &a, Streams& str)
	: app(a)
{
	config.incBase  = app.gringo.ibase;
	config.incBegin = 1;
	if (app.clingo.mode == CLINGO)
	{
		config.incEnd   = config.incBegin + app.gringo.ifixed;
		out.reset(new ClaspOutput(app.gringo.disjShift));
	}
	else if (app.clingo.mode == ICLINGO)
	{
		config.incEnd   = config.incBegin + app.clingo.inc.iQuery;
		out.reset(new iClaspOutput(app.gringo.disjShift));
	}
	// TODO move into clingo_app ?
	else
	{
		config.incEnd   = config.incBegin + app.clingo.inc.iQuery;
		out.reset(new oClaspOutput(grounder.get(), solver, app.gringo.disjShift));
	}
	if(app.clingo.mode == CLINGO && app.gringo.groundInput)
	{
		storage.reset(new Storage(out.get()));
		converter.reset(new Converter(out.get(), str));
	}
	else
	{
		grounder.reset(new Grounder(out.get(), app.generic.verbose > 2));
		parser.reset(new Parser(grounder.get(), config, str, app.gringo.compat));
	}
}

template <>
void FromGringo<OCLINGO>::getAssumptions(Clasp::LitVec& a)
{
	if(app.clingo.mode == ICLINGO || app.clingo.mode == OCLINGO)
	{
		const Clasp::AtomIndex& i = *solver->strategies().symTab.get();
		a.push_back(i.find(out->getIncAtom())->lit);
	}
}


template <>
void FromGringo<OCLINGO>::release()
{
	if (app.clingo.mode == CLINGO && !app.luaLocked())
	{
		grounder.reset(0);
		out.reset(0);
	}
}


template <>
bool FromGringo<OCLINGO>::read(Clasp::Solver& s, Clasp::ProgramBuilder* api, int)
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
			grounder->analyze(app.gringo.depGraph, app.gringo.stats);
			parser.reset(0);
			app.luaInit(*grounder, *out);
		}
		else
		{
			config.incBegin = config.incEnd;
			config.incEnd   = config.incEnd + 1;
		}

		ExternalKnowledge& ext = dynamic_cast<oClaspOutput*>(out.get())->getExternalKnowledge();

		if(app.clingo.mode == OCLINGO) {
			std::cerr << "read in OCLINGO mode" << std::endl;
			if(solver->hasConflict()) {
				ext.sendToClient("Error: The solver detected a conflict, so program is not satisfiable anymore.");
			}
			else {
				// add new facts and check for termination condition
				if(!ext.addInput())
					return false; // exit if received #stop.

				// do new step if there's no model or controller needs new step
				if(!ext.hasModel() || ext.needsNewStep()) {
					std::cerr << "preparing new step" << std::endl;
					grounder->ground();
					ext.endStep();
				} else {
					std::cerr << "preparing new iteration" << std::endl;
					ext.endIteration();
				}
			}
			std::cerr << "preparing get of new ext knowledge" << std::endl;
			ext.get();
		} else {
			grounder->ground();
		}
	}
	out->finalize();
	release();
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////
// ClingoApp
/////////////////////////////////////////////////////////////////////////////////////////

template <>
int ClingoApp<OCLINGO>::doRun()
{
	using namespace Clasp;
	if (gringo.groundOnly) { return GringoApp::doRun(); }
	if (cmdOpts_.basic.stats > 1) { 
		solver_.stats.solve.enableJumpStats(); 
		stats_.enableJumpStats();
	}
	Streams s;
	configureInOut(s);
	ClaspFacade clasp;
	facade_ = &clasp;
	timer_[0].start();
	if (clingo.mode == CLASP || clingo.mode == CLINGO)
	{
		clingo.iStats = false;
		clasp.solve(*in_, config_, this);
	}
	else if(clingo.mode == ICLINGO) { clasp.solveIncremental(*in_, config_, clingo.inc, this); }
	// TODO move in clingo_app.h ?
	else { clasp.solveIncremental(*in_, config_, oclingo.online, this); }
	timer_[0].stop();
	printResult(reason_end);
	if      (clasp.result() == ClaspFacade::result_unsat) { return S_UNSATISFIABLE; }
	else if (clasp.result() == ClaspFacade::result_sat)   { return S_SATISFIABLE; }
	else                                                  { return S_UNKNOWN; }
}

#define STATUS(v1,x) if (generic.verbose<v1);else (x)

template <>
void ClingoApp<OCLINGO>::state(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f) {
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
			STATUS(2, cout << "Reading      : ");
		}
		else if (f.state() == ClaspFacade::state_preprocess)
		{
			STATUS(2, cout << "Preprocessing: ");
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
		if (generic.verbose > 1 && (f.state() == ClaspFacade::state_read || f.state() == ClaspFacade::state_preprocess))
			cout << fixed << setprecision(3) << timer_[f.state()].elapsed() << endl;
		if (f.state() == ClaspFacade::state_solve)
		{
			stats_.accu(solver_.stats.solve);
			if (clingo.iStats)
			{
				timer_[0].stop();
				cout << "\nModels   : " << solver_.stats.solve.models << "\n"
						 << "Time     : " << fixed << setprecision(3) << timer_[0].current() << " (g: " << timer_[ClaspFacade::state_read].current()
						 << ", p: " << timer_[ClaspFacade::state_preprocess].current() << ", s: " << timer_[ClaspFacade::state_solve].current() << ")\n"
						 << "Rules    : " << f.api()->stats.rules[0] << "\n"
						 << "Choices  : " << solver_.stats.solve.choices   << "\n"
						 << "Conflicts: " << solver_.stats.solve.conflicts << "\n";
				timer_[0].start();
			}
			if(luaImpl.get()) { luaImpl->onEndStep(); }
			solver_.stats.solve.reset();
		}
	}

}

template <>
void ClingoApp<OCLINGO>::event(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f)
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
		else { out_->initSolve(solver_, f.api(), f.config()->solve.enumerator()); }
	}

	// TODO move into own hook function and remove above
	if(clingo.mode == OCLINGO && e == ClaspFacade::event_model) {
		// TODO unified model output (also minimize + consequences)
		string model = "";
		assert(solver_.strategies().symTab.get());
		const AtomIndex& index = *solver_.strategies().symTab;
		for (AtomIndex::const_iterator it = index.begin(); it != index.end(); ++it) {
			if (solver_.value(it->second.lit.var()) == trueValue(it->second.lit) && !it->second.name.empty()) {
				model += it->second.name + " ";
			}
		}
		dynamic_cast<oClaspOutput*>(dynamic_cast<FromGringo<OCLINGO>*>(in_.get())->out.get())->getExternalKnowledge().sendModel(model);
	}
}
