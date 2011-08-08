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
#ifndef CLINGCON_CONSTRAINT_H_INCLUDED
#define CLINGCON_CONSTRAINT_H_INCLUDED

//#include <clasp/include/util/misc_types.h>
#include <clingcon/csplit.h>
#include <vector>
#include <set>



namespace Clingcon {
class GroundConstraint;

//typedef std::pair<const GroundConstraint*,const GroundConstraint*> Pair;
//typedef std::pair<CSPLit::Type, Pair> Constraint;


        class Constraint
	{
		public:
                        Constraint();
                        Constraint(CSPLit::Type type, const GroundConstraint* a, const GroundConstraint* b);
                        Constraint(CSPLit::Type type);
                        Constraint(Constraint& cc);
                        //Constraint operator=(Constraint& cc);
                        ~Constraint();
                        // set lhs, if lhs is already set, set rhs
                        void add(const Constraint* a);

                        unsigned int getLinearSize() const;

                        // true of no boolean connectives are used
                        bool isSimple() const;

                        CSPLit::Type getType() const;

                        CSPLit::Type getRelations(const GroundConstraint*& a, const GroundConstraint*& b) const;
                        CSPLit::Type getConstraints(const Constraint*& a, const Constraint*& b) const;

                        void registerAllVariables(CSPSolver* csps) const;
                        void getAllVariables(std::vector<unsigned int>& vec, CSPSolver* csps) const;

		private:

                        CSPLit::Type type_;
                        std::auto_ptr<const GroundConstraint> a_;
                        std::auto_ptr<const GroundConstraint> b_;
                        std::auto_ptr<const Constraint>       lhs_;
                        std::auto_ptr<const Constraint>       rhs_;


	};
    typedef std::vector<std::pair<unsigned int, Constraint*> > ConstraintVec;

}
#endif
