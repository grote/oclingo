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

#include <program_opts/app_options.h>
#include <clasp/clasp_facade.h>
#include <program_opts/value.h>
#include "clingo/clingo_options.h"

struct oClingoConfig : public Clasp::IncrementalControl
{
	oClingoConfig()
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
	uint32 port;          /**< Socket port oclingo daemon should listen to */
	int    iQuery;
	bool   stopUnsat;     /**< Stop on first unsat problem? */
	bool   keepLearnt;    /**< Keep learnt nogoods between incremental steps? */
	bool   keepHeuristic; /**< Keep heuristic values between incremental steps? */

};

template <Mode M>
struct oClingoOptions
{
	oClingoOptions();
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden);
	bool validateOptions(ProgramOptions::OptionValues& values, bool claspMode, bool clingoMode, Mode& mode, iClingoConfig& iconfig, Messages&);
	void addDefaults(std::string& def);

	bool iclingoMode;
	oClingoConfig online;
};

template <Mode M>
oClingoOptions<M>::oClingoOptions() : iclingoMode(false) { }

template <Mode M>
void oClingoOptions<M>::initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden)
{
	(void)hidden;
	using namespace ProgramOptions;
	if(M == OCLINGO)
	{
		OptionGroup online_opts("Online Options");
		online_opts.addOptions()("port", storeTo(online.port), "Port oclingo daemon should listen to\n", "<num>");
		root.addOptions(online_opts);

		OptionGroup basic("Basic Options");
		basic.addOptions()("iclingo", bool_switch(&iclingoMode), "Run in iClingo mode");
		root.addOptions(basic,true);
	}
}


template <Mode M>
bool oClingoOptions<M>::validateOptions(ProgramOptions::OptionValues& values, bool claspMode, bool clingoMode, Mode& mode, iClingoConfig& iconfig, Messages& m)
{
	(void)values;

	if(M == OCLINGO) {
		if (claspMode && iclingoMode)
		{
			m.error = "Options '--iclingo' and '--clasp' are mutually exclusive";
			return false;
		}
		if (clingoMode && iclingoMode)
		{
			m.error = "Options '--clingo' and '--iclingo' are mutually exclusive";
			return false;
		}
	}

	if(iclingoMode) mode = ICLINGO;
	else if(!claspMode && !clingoMode) mode = OCLINGO;

	online.iQuery        = iconfig.iQuery;
	online.keepHeuristic = iconfig.keepHeuristic;
	online.keepLearnt    = iconfig.keepLearnt;
	online.maxSteps      = iconfig.maxSteps;
	online.minSteps      = iconfig.minSteps;

	return true;
}


template <Mode M>
void oClingoOptions<M>::addDefaults(std::string& def)
{
	if(M == OCLINGO)
	{
		def += "  --port=25277\n";
	}
}

//////////////////////////// oClingoConfig ////////////////////////////////////

void oClingoConfig::initStep(Clasp::ClaspFacade& f)
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
	std::cout << "Iteration: " << f.step()+1 << std::endl;
}

bool oClingoConfig::nextStep(Clasp::ClaspFacade& f)
{
	// TODO use stopUnsat?
	//using Clasp::ClaspFacade;
	//ClaspFacade::Result stopRes = stopUnsat ? ClaspFacade::result_unsat : ClaspFacade::result_sat;
	//return --maxSteps && ((minSteps > 0 && --minSteps) || f.result() != stopRes);
	(void) f;
	return --maxSteps;
}
