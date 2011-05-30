
#include <clingcon/cspsolver.h>
#include <clingcon/propagator.h>
#include <clasp/constraint.h>
#include <gringo/litdep.h>


namespace Clingcon {

		ClingconPropagator::ClingconPropagator(CSPSolver* s) :
			cspSolver_(s) {
			assert(s != 0);
			cspSolver_->setClingconPropagator(this);
		}
		ClingconPropagator::~ClingconPropagator() {
                    delete cspSolver_;
		}
                bool ClingconPropagator::propagate(Clasp::Solver& ) {
			return cspSolver_->propagate();
		}
                Clasp::Constraint::PropResult ClingconPropagator::propagate(const Clasp::Literal& l, uint32& date, Clasp::Solver& )
		{
			assert(cspSolver_);
			cspSolver_->propagateLiteral(l,date);
			return Clasp::Constraint::PropResult(true, true);
		}
		bool ClingconPropagator::nextSymModel(Clasp::Solver&, bool expand) {
			return expand && cspSolver_->nextAnswer();
		}
		bool ClingconPropagator::isModel(Clasp::Solver&) { return cspSolver_->hasAnswer(); }
		void ClingconPropagator::undoLevel(Clasp::Solver& s) {
                   cspSolver_->undoUntil(s.decisionLevel()-1);
		}

		void ClingconPropagator::reset()
		{
			cspSolver_->reset();
		}

		uint32 ClingconPropagator::priority() const
		{
			return Clasp::PostPropagator::priority_general;
		}
}
