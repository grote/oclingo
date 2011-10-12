
#include <clingcon/groundconstraint.h>
#include <clingcon/groundconstraintvarlit.h>
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
        else
        if (isOperator())
        {
            for (GroundConstraintVec::const_iterator i = a_.begin(); i != a_.end(); ++i)
                i->getAllVariables(vec,csps);
            /*if (a_!= 0)
                a_->getAllVariables(vec,csps);
            if (b_!= 0)
                b_->getAllVariables(vec,csps);
                */
        }
        else
            assert(isInteger());
    }


    void GroundConstraint::registerAllVariables(CSPSolver* csps) const
    {
        if (isVariable())
        {
           csps->getVariable(name_);
        }
        else
        if (isOperator())
        {
            for (GroundConstraintVec::const_iterator i = a_.begin(); i != a_.end(); ++i)
                i->registerAllVariables(csps);
        }
        else
            assert(isInteger());

    }

}
