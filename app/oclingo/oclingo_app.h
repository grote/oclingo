// Copyright (c) 2012, Torsten Grote <tgrote@uni-potsdam.de>
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

#include "oclingo/oclaspoutput.h"
#include "oclingo/externalknowledge.h"
#include "clingo/clingo_app.h"

/////////////////////////////////////////////////////////////////////////////////////////
// FromGringo
/////////////////////////////////////////////////////////////////////////////////////////

template <>
bool FromGringo<OCLINGO>::read(Clasp::Solver& s, Clasp::ProgramBuilder* api, int)
{
	assert(out.get());
	out->setProgramBuilder(api);
	solver = &s;
	if(converter.get())
	{
		out->initialize();
		converter->parse();
		converter.reset(0);
		out->finalize();
	}
	else
	{
		if (parser.get())
		{
			out->initialize();
			parser->parse();

			grounder->analyze(app.gringo.depGraph, app.gringo.stats);
			parser.reset(0);
			app.luaInit(*grounder, *out);
			app.setIinit(config);
			app.groundBase(*grounder, config, app.gringo.iinit, app.clingo.mode == CLINGO ? app.gringo.ifixed : 1, app.clingo.mode == CLINGO ? app.gringo.ifixed : app.clingo.inc.iQuery);
			out->finalize();
		}
		else if(app.clingo.mode != OCLINGO)
		{
			out->initialize();
			config.incStep++;
			app.groundStep(*grounder, config, config.incStep, app.clingo.inc.iQuery);
			out->finalize();
		}

		if(app.clingo.mode == OCLINGO)
		{
			ExternalKnowledge& ext = dynamic_cast<oClaspOutput*>(out.get())->getExternalKnowledge();

			if(solver->hasConflict())
			{
				ext.sendToClient("Error: The solver detected a conflict, so program is not satisfiable anymore.");
			}
			else
			{
				// inform controller in case there are no models within set bound
				if(!ext.hasModel() && config.incStep >= ext.getBound()) {
					std::stringstream s;
					s << config.incStep;
					ext.sendToClient("UNSAT at step "+s.str());
				}
				// get/receive new external input
				ext.get();

				// add new facts and check for termination condition
				if(!ext.addInput()) { return false; /* exit if received #stop. */ }

				// do new step only if bound is obeyed and if there's no model or the controller needs new step
				if(config.incStep < ext.getBound() && (!ext.hasModel() || ext.needsNewStep()))
				{
					// ground program up to current controller step
					do
					{
						out->initialize(); // gives new IncUid for volatiles
						config.incStep++;
						app.groundStep(*grounder, config, config.incStep, app.clingo.inc.iQuery);
						ext.endStep();
						out->finalize();
					}
					while(config.incStep < ext.getControllerStep());

					// now program is ready for premature knowledge to be added
					if(ext.addPrematureKnowledge())
					{
						// call finalize again if there was premature knowledge added
						out->finalize();
					}
				}
				else
				{
					// do not increase step, just finish this iteration
					ext.endIteration();
					out->finalize();
				}
			}
		} // end OCLINGO
	}
	release();
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////
// ClingoApp
/////////////////////////////////////////////////////////////////////////////////////////

template <>
void ClingoApp<OCLINGO>::doState(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f) {
	using namespace Clasp;
	ExternalKnowledge& ext = dynamic_cast<oClaspOutput*>(dynamic_cast<FromGringo<OCLINGO>*>(in_.get())->out.get())->getExternalKnowledge();

	if(clingo.mode == OCLINGO) {
		if(f.state() == ClaspFacade::state_solve && e == ClaspFacade::event_state_exit) {
			// make sure we are not adding new input while doing propagation in preprocessing
			ext.removePostPropagator(solver_);
		}
		if(f.state() == ClaspFacade::state_solve && e == Clasp::ClaspFacade::event_state_enter) {
			// make sure we are not adding new input while doing propagation in preprocessing
			ext.addPostPropagator(solver_);
		}
	}

}

template <>
void ClingoApp<OCLINGO>::doEvent(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f)
{
	(void) f;
	using namespace Clasp;

	if(clingo.mode == OCLINGO && e == ClaspFacade::event_model)
	{
		// TODO unified model output (also minimize + consequences)
		std::string model = "";
		assert(solver_.strategies().symTab.get());
		const AtomIndex& index = *solver_.strategies().symTab;
		for (AtomIndex::const_iterator it = index.begin(); it != index.end(); ++it)
		{
			if (solver_.value(it->second.lit.var()) == trueValue(it->second.lit) && !it->second.name.empty())
			{
				model += it->second.name + " ";
			}
		}
		dynamic_cast<oClaspOutput*>(dynamic_cast<FromGringo<OCLINGO>*>(in_.get())->out.get())->getExternalKnowledge().sendModel(model);
	}
}
