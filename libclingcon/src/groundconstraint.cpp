
#include <clingcon/groundconstraint.h>
#include <clingcon/cspsolver.h>
#include <gringo/litdep.h>

namespace Clingcon
{
    void GroundConstraint::getAllVariables(std::vector<unsigned int>& vec, CSPSolver* csps) const
    {
        if (isVariable())
        {
            vec.push_back(csps->getVariable(name_));
        }
        if (isOperator())
        {
            if (a_.get()!= 0)
                a_->getAllVariables(vec,csps);
            if (b_.get()!= 0)
                b_->getAllVariables(vec,csps);
        }
    }
}
