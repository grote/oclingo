// 
// Copyright (c) 2006-2009, Benjamin Kaufmann
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
#include "clasp_output.h"
#include <clasp/solver.h>
#include <clasp/smodels_constraints.h>
#include <clasp/solve_algorithms.h>
#include <stdio.h>

namespace Clasp { namespace {
inline double percent(uint64 r, uint64 b) { 
	if (b == 0) return 0;
	return (static_cast<double>(r)/b)*100.0;
}
inline double average(uint64 x, uint64 y) {
	if (!x || !y) return 0.0;
	return static_cast<double>(x) / static_cast<double>(y);
}
}

OutputFormat::OutputFormat()  {
	format[comment] = "";
	format[model]   = "";
	format[opt]     = "Optimization: ";
	format[solution]= "";
	format[atom_sep]= "";

	result[sat]     = "SATISFIABLE";
	result[unsat]   = "UNSATISFIABLE";
	result[unknwon] = "UNKNOWN";
	result[optimum] = "OPTIMUM FOUND";
}
OutputFormat::~OutputFormat() {}

void OutputFormat::initSolve(const Solver&, ProgramBuilder*) {}

void OutputFormat::printConsequences(const Solver& s, const Enumerator&) {
	assert(s.strategies().symTab.get());
	const AtomIndex& index = *s.strategies().symTab;
	printf("%s", format[model]);
	for (AtomIndex::const_iterator it = index.begin(); it != index.end(); ++it) {
		if (it->second.lit.watched()) {
			printf("%s%s ", it->second.name.c_str(), format[atom_sep]);
		}
	}
	printf("\n");
	fflush(stdout);
}
void OutputFormat::printOptimize(const MinimizeConstraint& min) {
	printf("%s", format[opt]);
	printOptimizeValues(min);
	printf("\n");
	fflush(stdout);
}
void OutputFormat::printOptimizeValues(const MinimizeConstraint& m) {
	for (uint32 i = 0; i < m.numRules(); ++i) {
		printf("%"PRId64" ", m.getOptimum(i));
	}
}
void OutputFormat::printSolution(const Solver& s, const Enumerator& en, bool complete) {
	Result state = unknwon;
	if (complete) {
		if      (s.stats.solve.models == 0)  state = unsat;
		else if (en.minimize())              state = optimum;
		else                                 state = sat;
	}
	else if (s.stats.solve.models > 0)     state = sat;
	printf("%s%s\n", format[solution], result[state]);
}
void OutputFormat::printJumpStats(const SolverStatistics& stats) {
	(void)stats;
#if MAINTAIN_JUMP_STATS == 1
	const SolveStats& st  = stats.solve; 
	int w = 20 - (int)strlen(format[comment]);
	const char* c = format[comment];
	printf("%s\n%-*s: %-6"PRIu64"\n", c, w, "Backtracks",st.conflicts-st.jumps);
	printf("%s%-*s: %-6"PRIu64" (Bounded: %"PRIu64")\n", c, w, "Backjumps", st.jumps, st.bJumps);
	printf("%s%-*s: %-6"PRIu64"\n", c, w, "Skippable Levels",st.jumpSum);
	printf("%s%-*s: %-6"PRIu64" (%5.1f%%)\n", c, w, "Levels skipped",st.jumpSum - st.boundSum, percent(st.jumpSum - st.boundSum, st.jumpSum));
	printf("%s%-*s: %-6u (Executed: %u)\n", c, w, "Max Jump Length",st.maxJump, st.maxJumpEx);
	printf("%s%-*s: %-6u\n", c, w, "Max Bound Length",st.maxBound);
	printf("%s%-*s: %-6.1f (Executed: %.1f)\n", c, w, "Average Jump Length",st.avgJumpLen(), st.avgJumpLenEx());
	printf("%s%-*s: %-6.1f\n", c, w, "Average Bound Length",average(st.boundSum,st.bJumps));
	printf("%s%-*s: %-6.1f\n", c, w, "Average Model Length",average(st.modLits,st.models));
#endif
}
/////////////////////////////////////////////////////////////////////////////////////////
// ASP output
/////////////////////////////////////////////////////////////////////////////////////////
AspOutput::AspOutput(bool asp09) : asp09_(asp09) {
	if (asp09) {
		format[atom_sep] = ".";
		result[sat]      = "";
	}
}

void AspOutput::initSolve(const Solver&, ProgramBuilder* api) {
	// grab the stats_ so that we can later print them
	if (api) stats_.accu(api->stats);
}

void AspOutput::printModel(const Solver& s, const Enumerator&) {
	assert(s.strategies().symTab.get());
	const AtomIndex& index = *s.strategies().symTab;
	for (AtomIndex::const_iterator it = index.begin(); it != index.end(); ++it) {
		if (s.value(it->second.lit.var()) == trueValue(it->second.lit) && !it->second.name.empty()) {
			printf("%s%s ", it->second.name.c_str(), format[atom_sep]);
		}
	}
	printExtendedModel(s);
	printf("\n");
	fflush(stdout);
}

void AspOutput::printStats(const SolverStatistics& stats, const Enumerator&) {
	if (!asp09_) {
		const SolveStats& st   = stats.solve;
		const ProblemStats& ps = stats.problem;
		const PreproStats& lp  = stats_;
		printf("%-12s: %"PRIu64"\n", "Choices",st.choices);
		printf("%-12s: %"PRIu64"\n", "Conflicts",st.conflicts);
		printf("%-12s: %"PRIu64"\n", "Restarts",st.restarts);
		printf("\n%-12s: %-6u", "Atoms", lp.atoms);
		if (lp.trStats) {
			printf(" (Original: %u Auxiliary: %u)", lp.atoms-lp.trStats->auxAtoms, lp.trStats->auxAtoms);
		}
		printf("\n%-12s: %-6u (", "Rules", lp.rules[0]);
		printf("1: %u", lp.rules[BASICRULE] - (lp.trStats?lp.trStats->rules[0]:0));
		if (lp.trStats) { printf("/%u", lp.rules[BASICRULE]); }
		for (int i = 2; i <= OPTIMIZERULE; ++i) {
			if (lp.rules[i] > 0) { 
				printf(" %d: %u", i, lp.rules[i]);
				if (lp.trStats) { printf("/%u", lp.rules[i]-lp.trStats->rules[i]); }
			}
		}
		printf(")\n");
		printf("%-12s: %-6u\n", "Bodies", lp.bodies);
		if (lp.sumEqs() > 0) {
			printf("%-12s: %-6u (Atom=Atom: %u Body=Body: %u Other: %u)\n", "Equivalences"
				, lp.sumEqs()
				, lp.numEqs(Var_t::atom_var)
				, lp.numEqs(Var_t::body_var)
				, lp.numEqs(Var_t::atom_body_var));
		}
		printf("%-12s: ", "Tight");
		if      (lp.sccs == 0)               { printf("Yes"); }
		else if (lp.sccs != PrgNode::noScc)  { printf("%-6s (SCCs: %u Nodes: %u)", "No", lp.sccs, lp.ufsNodes); }
		else                                 { printf("N/A"); }
		printf("\n");
		
		uint32 other = ps.constraints[0] - ps.constraints[1] - ps.constraints[2];
		printf("\n%-12s: %-6u (Eliminated: %4u)\n", "Variables", stats.problem.vars, stats.problem.eliminated);
		printf("%-12s: %-6u (Binary:%5.1f%% Ternary:%5.1f%% Other:%5.1f%%)\n", "Constraints"
			, ps.constraints[0]
			, percent(ps.constraints[1], ps.constraints[0])
			, percent(ps.constraints[2], ps.constraints[0])
			, percent(other, ps.constraints[0]));
		other = st.learnt[0] - st.learnt[1] - st.learnt[2];
		printf("%-12s: %-6u (Binary:%5.1f%% Ternary:%5.1f%% Other:%5.1f%%)\n", "Lemmas"
			, st.learnt[0]
			, percent(st.learnt[1], st.learnt[0])
			, percent(st.learnt[2], st.learnt[0])
			, percent(other, st.learnt[0]));
		printf("%-12s: %-6"PRIu64" (Average Length: %.1f) \n", "  Conflicts", st.learnt[0]-st.loops, average(st.lits[0], st.learnt[0]-st.loops));
		printf("%-12s: %-6"PRIu64" (Average Length: %.1f) \n", "  Loops",     st.loops, average(st.lits[1], st.loops));
		OutputFormat::printJumpStats(stats);
		fflush(stdout);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// DIMACS output
/////////////////////////////////////////////////////////////////////////////////////////
SatOutput::SatOutput() {
	format[comment] = "c ";
	format[model]   = "v ";
	format[opt]     = "o ";
	format[solution]= "s ";
	format[atom_sep]= "";
}

void SatOutput::printModel(const Solver& s, const Enumerator&) {
	printf("v ");
	const uint32 numVarPerLine = 10;
	for (Var v = 1, cnt=0; v <= s.numVars(); ++v) {
		printf("%s%u", s.value(v) == value_false ? "-":"", v);
		if (++cnt == numVarPerLine && v+1 <= s.numVars()) { cnt = 0; printf("\nv"); }
		printf(" ");
	}
	printf("0\n");
	fflush(stdout);
}

void SatOutput::printStats(const SolverStatistics& stats, const Enumerator&) {
	const SolveStats& st   = stats.solve;
	const ProblemStats& ps = stats.problem;
	printf("c %-10s: %"PRIu64"\n", "Choices",st.choices);
	printf("c %-10s: %"PRIu64"\n", "Conflicts",st.conflicts);
	printf("c %-10s: %"PRIu64"\n", "Restarts",st.restarts);	
	printf("c %-10s: %-6u (Eliminated: %u)\n", "Variables",stats.problem.vars, stats.problem.eliminated);
	printf("c %-10s: %u\n", "Clauses",ps.constraints[0]);
	printf("c %-10s: %u\n", "  Binary",ps.constraints[1]);
	printf("c %-10s: %u\n", "  Ternary",ps.constraints[2]);
	printf("c %-10s: %u\n", "Lemmas",st.learnt[0]);
	printf("c %-10s: %u\n", "  Binary",st.learnt[1]);
	printf("c %-10s: %u\n", "  Ternary",st.learnt[2]);
	printf("c %-10s: %u\n", "  Deleted",st.learnt[3]);
	OutputFormat::printJumpStats(stats);
	fflush(stdout);
}
/////////////////////////////////////////////////////////////////////////////////////////
// PB output
/////////////////////////////////////////////////////////////////////////////////////////
PbOutput::PbOutput(bool printSuboptModels) : lastModel_(0), printSuboptModels_(printSuboptModels) {}
PbOutput::~PbOutput() { delete [] lastModel_; }
void PbOutput::printModel(const Solver& s, const Enumerator& en) {
	if (!en.minimize() || printSuboptModels_) {
		printf("v ");
		for (Var v = 1; v <= s.numVars(); ++v) {
			printf("%sx%u ", s.value(v) == value_false ? "-":"", v);
		}
		printf("\n");
		fflush(stdout);
	}
	else {
		uint8* model = new uint8[s.numVars()+1];
		for (Var v = 1; v <= s.numVars(); ++v) {
			model[v] = s.value(v);
		}
		uint8* old   = lastModel_;
		lastModel_   = model;
		delete [] old;
	}
}
void PbOutput::printSolution(const Solver& s, const Enumerator& en, bool complete) {
	if (lastModel_) {
		printf("v ");
		for (Var v = 1; v <= s.numVars(); ++v) {
			printf("%sx%u ", lastModel_[v] == value_false ? "-":"", v);
		}
		printf("\n");
		fflush(stdout);
	}
	OutputFormat::printSolution(s, en, complete);
}
}
