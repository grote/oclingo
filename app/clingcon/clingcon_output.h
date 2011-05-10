
#pragma once


#include "clasp/clasp_output.h"
#include <clingcon/cspsolver.h>

namespace Clingcon
{
    class ClingconOutput : public Clasp::AspOutput {
    public:
        explicit ClingconOutput(bool asp09, CSPSolver* csp) : AspOutput(asp09), csp_(csp)
        {
        }

    private:
        // Hook for derived classes
        virtual void printExtendedModel(const Clasp::Solver&)
        {
            csp_->printAnswer();
        }
        CSPSolver* csp_;
    };

}
