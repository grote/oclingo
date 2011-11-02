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

#include "oclingo/oclaspoutput.h"
#include "oclingo/externalknowledge.h"
#include "clingo/clingo_app.h"

/////////////////////////////////////////////////////////////////////////////////////////
// FromGringo
/////////////////////////////////////////////////////////////////////////////////////////

template <>
void FromGringo<OCLINGO>::getAssumptions(Clasp::LitVec& a)
{
	if(app.clingo.mode == ICLINGO || app.clingo.mode == OCLINGO)
	{
		const Clasp::AtomIndex& i = *solver->strategies().symTab.get();

		foreach(uint32_t atom, out->getIncUids()) {
			if(atom) a.push_back(i.find(atom)->lit);
		}

		if(app.clingo.mode == OCLINGO) {
			oClaspOutput *o_output = static_cast<oClaspOutput*>(out.get());

			// assume volatile atom to be true for this iteration only
			uint32_t vol_atom = o_output->getVolAtomAss();
			if(vol_atom) {
				a.push_back(i.find(vol_atom)->lit);
			}
			// only deprecate volatile atom if we want the answer set this step
			if(!o_output->getExternalKnowledge().needsNewStep()) {
				o_output->deprecateVolAtom();
			}

			// assume volatile atoms for time decay to be true
			VarVec window_ass = o_output->getVolWindowAtomAss(config.incStep-1);
			foreach(VarVec::value_type atom, window_ass) {
				if(atom) {
					a.push_back(i.find(atom)->lit);
				}
			}

			VarVec& ass = o_output->getExternalKnowledge().getExternals();
			for(VarVec::iterator lit = ass.begin(); lit != ass.end(); ++lit) {
				Clasp::Atom* atom = i.find(*lit);
				if(atom) { // atom is not in AtomIndex if hidden with #hide
					// assume external atom to be false
					a.push_back(~atom->lit);
				}
			} // end for
		} // end OCLINGO only part
	}
}

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
			app.groundBase(*grounder, config, app.gringo.iinit, app.clingo.mode == CLINGO ? app.gringo.ifixed : 1, app.clingo.mode == CLINGO ? app.gringo.ifixed : app.clingo.inc.iQuery);
			out->finalize();

			if(app.clingo.mode == OCLINGO) {
				ExternalKnowledge& ext = dynamic_cast<oClaspOutput*>(out.get())->getExternalKnowledge();
				ext.addPrematureKnowledge(); // only for freezing base and 1st step externals

				// get/receive new external input
				ext.get();
			}
		}
		else if(app.clingo.mode == OCLINGO) {
			ExternalKnowledge& ext = dynamic_cast<oClaspOutput*>(out.get())->getExternalKnowledge();

			// TODO rewrite process logic in a more sane way (might break things)
			if(solver->hasConflict()) {
				ext.sendToClient("Error: The solver detected a conflict, so program is not satisfiable anymore.");
			}
			else {
				// add new facts and check for termination condition
				if(!ext.addInput()) {
					return false; // exit if received #stop.
				}

				// do new step if there's no model or controller needs new step
				if(!ext.hasModel() || ext.needsNewStep()) {
					do {
						out->initialize(); // gives new IncUid for volatiles
						config.incStep++;
						app.groundStep(*grounder, config, config.incStep, app.clingo.inc.iQuery);
						ext.endStep();
						out->finalize();
					} while(config.incStep < ext.getControllerStep());

					if(ext.addPrematureKnowledge()) {
						// call finalize again if there was premature knowlegde added
						out->finalize();
					}
				} else {
					ext.endIteration();
					out->finalize();
				}
			}
			// get/receive new external input
			ext.get();
		} else {
			out->initialize();
			config.incStep++;
			app.groundStep(*grounder, config, config.incStep, app.clingo.inc.iQuery);
			out->finalize();
		}
	}
	release();
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////////
// ClingoApp
/////////////////////////////////////////////////////////////////////////////////////////

template <>
void ClingoApp<OCLINGO>::doEvent(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f)
{
	(void) f;
	using namespace Clasp;

	if(clingo.mode == OCLINGO && e == ClaspFacade::event_model) {
		// TODO unified model output (also minimize + consequences)
		std::string model = "";
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
