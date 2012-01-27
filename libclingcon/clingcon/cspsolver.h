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
#ifndef CLASP_CSPSOLVER_H_INCLUDED
#define CLASP_CSPSOLVER_H_INCLUDED

#include <clingcon/cspconstraint.h>
#include <clasp/literal.h>
#include <clingcon/cspglobalprinter.h>
#include <clingcon/interval.h>

#include <vector>
namespace Clasp {
	class ProgramBuilder;
	class Solver;
}

namespace Clingcon {

class ASPmCSPException : public std::exception
{
public:
        ASPmCSPException(const std::string &message) : message_(message)
        {
        }
        const char* what() const throw()
        {
                return message_.c_str();
        }
        virtual ~ASPmCSPException() throw()
        {
        }
private:
        const std::string message_;
};
	class ClingconPropagator;
        class FwdLinearIRSRA;
        class CCIRSRA;
        class CCRangeRA;
        class CCRangeCA;
        class CCIISCA;

	class CSPSolver
	{

            friend class FwdLinearIRSRA;
            friend class CCIRSRA;
            friend class CCRangeRA;
            friend class CCRangeCA;
            friend class CCIISCA;

                public:
                        typedef Interval<int> Domain;
                        typedef boost::ptr_map<Clasp::Literal, Constraint> ConstraintMap;
                        CSPSolver();
                        virtual ~CSPSolver();
			/*
			 * adds a constraint with the uid of the atom representing the constraint
			 *
			 **/
                        virtual void setDomain(int lower, int upper);
                        virtual void addConstraint(Constraint* c, int uid);
                        virtual const ConstraintMap& getConstraints() const;
                        virtual void addGlobalConstraints(LParseGlobalConstraintPrinter::GCvec& gcvec);
                        virtual bool hasOptimizeStm() const;
                        //virtual void addDomain(const std::string& var, int lower, int upper);
                        virtual unsigned int getVariable(const std::string& s);
                        //virtual void combineWithDefaultDomain();
                        virtual const std::vector<std::string>&  getVariables() const;
                        //virtual bool hasDomain(const std::string& s) const;
                        //virtual const RangeVec& getDomain(const std::string& s) const;
                        virtual const Domain& getDomain() const;
			virtual void setSolver(Clasp::Solver* s);
			virtual Clasp::Solver* getSolver();
			virtual void setClingconPropagator(Clingcon::ClingconPropagator* cp);
                        virtual bool initialize() = 0;
			virtual bool propagate() = 0;
                        //propagate after model was found
                        virtual bool propagateMinimize() = 0;
			virtual void reset() = 0;
			virtual void propagateLiteral(const Clasp::Literal& l, int date) = 0;
                        virtual void undo(unsigned int level) = 0;
                        virtual void printStatistics() = 0;
			/*
			 * pre: complete assignment
			 * return true if a valid solution for the asp vars exists
			 * return false elsewise, generates a conflict
			 * post: the next answer can be called
			 *
			 * Only use this for the first answer
			 */
			virtual bool hasAnswer() = 0;
			virtual bool nextAnswer() = 0;
			/*
			 *pre: hasAnswer has been called and returned true
			 * prints the answer set
			 */
			virtual void printAnswer() = 0;

                        void setOptimize(bool opt);

		protected:

                        std::vector<std::string> variables_;
                        std::map<std::string,unsigned int> variableMap_;
                        //boost::unordered_map<Val,unsigned int> variables_;
			Clasp::Solver* s_;
			ClingconPropagator* clingconPropagator_;


                        ConstraintMap constraints_;
                        LParseGlobalConstraintPrinter::GCvec globalConstraints_;
        protected:

                        //Domains domains_;
                        Domain domain_; // the global domain of all variables(and all intermediate variables, this could be a problem)
                        //bool addedDomain_; // true if domain was already added
                        bool optimize_;

	};
}
#endif
