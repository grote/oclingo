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

#include <clingcon/cspsolver.h>
#include <gringo/litdep.h>

namespace Clingcon {


CSPSolver::CSPSolver() : optimize_(false), domain_(std::numeric_limits<int>::min(), std::numeric_limits<int>::max())
{

}

bool CSPSolver::hasOptimizeStm() const
{
    return optimize_;
}


unsigned int CSPSolver::getVariable(const std::string& v)
{
    std::map<std::string,unsigned int>::iterator f = variableMap_.find(v);
    if (f != variableMap_.end())
        return f->second;
    else
    {
        variables_.push_back(v);
        variableMap_[v]=variables_.size()-1;
        return variables_.size()-1;

    }
 /*   for(unsigned int i = 0; i != variables_.size(); ++i)
    {
            if (variables_[i] == v)
                    return i;
    }

    variables_.push_back(v);
    return variables_.size()-1;
    */

}

const std::vector<std::string>&  CSPSolver::getVariables() const
{
    return variables_;
}

void CSPSolver::addConstraint(Constraint* c, int uid)
{
    //constraints_[uid] = c;//new Clingcon::Constraint(c);
    constraints_.insert(uid,c);

}

const CSPSolver::ConstraintMap& CSPSolver::getConstraints() const
{
    return constraints_;
}

void CSPSolver::addGlobalConstraints(LParseGlobalConstraintPrinter::GCvec& gcvec)
{
    globalConstraints_ = gcvec.release();
}
/*
void CSPSolver::addDomain(const std::string& var, int lower, int upper)
{
    //Domains::iterator found = domains_.find(var);
    //if (found==domains_.end())
    //    domains_[var]=RangeVec();

    domains_[var].push_back(lower);
    domains_[var].push_back(upper);

    //found->second.push_back(lower);
    //found->second.push_back(upper);
}
*/
/*
bool CSPSolver::hasDomain(const std::string& s) const
{
    Domains::const_iterator i = domains_.find(s);
    if (i==domains_.end())
        return false;
    return (i->second.size() > 0 ? true : false);
}*/
/*
const CSPSolver::RangeVec& CSPSolver::getDomain(const std::string& s) const
{
    return domains_.find(s)->second;
}
*/
const CSPSolver::Domain& CSPSolver::getDomain() const
{
    return domain_;
}

void CSPSolver::setDomain(int lower, int upper)
{
    if (upper==std::numeric_limits<int>::max())
    {
        upper--;
        std::cerr << "Warning: Reduced upper domain to " << upper;
    }
        //throw ASPmCSPException("Only one domain predicate allowed!");
    domain_.left = lower;
    domain_.right = upper+1;
}

/*
void CSPSolver::combineWithDefaultDomain()
{
    for(std::vector<std::string>::const_iterator var = variables_.begin(); var != variables_.end(); ++var)
    {
        domains_[*var].insert(domains_[*var].end(), domain_.begin(), domain_.end());
    }
}*/

void CSPSolver::setSolver(Clasp::Solver* s)
{
	assert(s);
	s_ = s;
}

Clasp::Solver* CSPSolver::getSolver()
{
	return s_;
}

void CSPSolver::setClingconPropagator(Clingcon::ClingconPropagator* cp)
{
	assert(cp);
	clingconPropagator_ = cp;
}

void CSPSolver::setOptimize(bool opt)
{
    optimize_=opt;
}

CSPSolver::~CSPSolver(){}



}
