#ifndef CLINGCONPROPAGATOR_H
#define CLINGCONPROPAGATOR_H

#include <clingcon/cspsolver.h>
#include <clasp/solver.h>
#include <clasp/solve_algorithms.h>
#include <clasp/constraint.h>

namespace Clingcon {

class ClingconPropagator: public Clasp::PostPropagator {
public:
	ClingconPropagator(CSPSolver* s);
	~ClingconPropagator();
	bool propagate(Clasp::Solver& s);
	Clasp::Constraint::PropResult propagate(const Clasp::Literal& l, uint32& date, Clasp::Solver& s);
	bool nextSymModel(Clasp::Solver&, bool expand);
	bool isModel(Clasp::Solver& s);
	void undoLevel(Clasp::Solver& s);
	void reset();
	uint32 priority() const;
private:
	CSPSolver* cspSolver_; // wrapper around csp solver
};
}
#endif
