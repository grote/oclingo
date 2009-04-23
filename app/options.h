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
#ifndef CLASP_OPTIONS_H_INCLUDED
#define CLASP_OPTIONS_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include "program_opts/program_options.h"

#include <string>
#include <utility>
#include <iosfwd>

#include <grounder.h>

#ifdef WITH_CLASP
#	include <clasp/include/solver.h>
#	include <clasp/include/solve_algorithms.h>

#endif

#ifdef WITH_ICLASP
//const char* const EXECUTABLE = "iclingo";
//const char* const CLASP_VERSION = "1.2.0";
#elif defined WITH_CLASP
//const char* const EXECUTABLE = "clingo";
//const char* const CLASP_VERSION = "1.2.0";
#else
//const char* const EXECUTABLE = "gringo";
#endif
//const char* const GRINGO_VERSION = "2.0.3";

class Options {
public:
	enum OutputFormat {SMODELS_OUT, GRINGO_OUT, CLASP_OUT, TEXT_OUT, ICLASP_OUT};
public:
	Options();
	void setDefaults();
	bool parse(int argc, char** argv, std::ostream& os, ProgramOptions::OptionValues &s);
#ifdef WITH_CLASP
	bool initSolver(Clasp::Solver &s, ProgramOptions::OptionValues &values);
#endif
	const std::string& getError() const {
		return error_;
	}
	const std::string getWarning() const {
		return warning_;
	}

	// common stuff
	bool             help;             // Default: false
	bool             version;          // Defailt: false
	bool             stats;            // Default: false
	bool             verbose;          // Default: false
	bool             syntax;           // Default: false
	std::vector<std::string> files;    // Default: "" -> read from stdin

	// gringo stuff

	// ifixed      => Default: -1
	// verbose     => Default: false
	// binderSplit => Default: true
	NS_GRINGO::Grounder::Options grounderOptions;
	bool             claspMode;        // Default: false
	bool             convert;          // Default: false
	std::vector<std::string> consts;

	bool             smodelsOut;
	int              aspilsOut;
	bool             shift;            // Default: false
#ifdef WITH_CLASP
	bool             claspOut;
#endif
	bool             textOut;
	OutputFormat     outf;             // Default: depends on build

#ifdef WITH_ICLASP
	// incremental stuff
	bool             iclaspOut;
	int              imin;             // Default: 1
	int              imax;             // Default: std::numeric_limits<int>::max()
	bool             iunsat;           // Default: false
	bool             keepLearnts;      // Default: true
	bool             keepHeuristic;    // Default: false
	bool             ibase;            // Default: false
	bool             istats;           // Default: false
#endif
#ifdef WITH_CLASP
	// clasp stuff
	Clasp::SolveParams solveParams;
	std::vector<int> optVals;          // Values for the optimization function
	std::vector<int> satPreParams;     // Params for the SatElite-preprocessor
	std::string      heuristic;        // Default: berkmin
	std::string      cons;             // Default: ""
        double           redFract;         // Default: 3.0
	double           redInc;           // Default: 1.1
	double           redMax;           // Default: 3.0
	int              seed;             // Default: -1 -> use default seed
	int              transExt;         // Default: 0 -> do not transform extended rules
	int              eqIters;          // Default: -1 -> run eq-preprocessing to fixpoint
	int              numModels;        // Default: 1
	int              lookahead;        // Default: lookahead_no
	int              initialLookahead; // Default: -1
	int              loopRep;          // Default: common
	int              projectConfig;    // Default: -1
	bool             optAll;           // Default: false
	bool             quiet;            // Default: false
	bool             dimacs;           // Default: false
	bool             suppModels;       // Default: false
	bool             ccmExp;           // Default: false
	bool             redOnRestart;     // Default: false
	bool             modelRestart;     // Default: false
	bool             recordSol;        // Default: false
	bool             project;          // Default: false
#endif
private:
	void initOptions(ProgramOptions::OptionGroup& allOpts, ProgramOptions::OptionGroup& hidden);
	void checkCommonOptions(const ProgramOptions::OptionValues&);
	void printSyntax(std::ostream& os) const;
	void printHelp(const ProgramOptions::OptionGroup& opts, std::ostream& os) const;
	void printVersion(std::ostream& os) const;
#ifdef WITH_CLASP
	std::vector<double> delDefault() const {
		std::vector<double> v; v.push_back(3.0); v.push_back(1.1); v.push_back(3.0);
		return v;
	}
	std::vector<double> restartDefault() const {
		std::vector<double> v; v.push_back(100.0); v.push_back(1.5); v.push_back(0.0);
		return v;
	}
	bool setSolverStrategies(Clasp::Solver& s, const ProgramOptions::OptionValues&);
	bool setSolveParams(Clasp::Solver& s, const ProgramOptions::OptionValues&);
	Clasp::DecisionHeuristic* createHeuristic(const std::vector<int>&) const;
#endif
	std::string error_;
	std::string warning_;
};
#endif
