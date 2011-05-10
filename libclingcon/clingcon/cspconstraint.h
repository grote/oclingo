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
                        Constraint(Constraint& cc);
                        Constraint operator=(Constraint& cc);
                        ~Constraint();

                        unsigned int getLinearSize() const;

                        CSPLit::Type getType() const;

                        CSPLit::Type getRelation(const GroundConstraint*& a, const GroundConstraint*& b) const;

                        std::vector<unsigned int> getAllVariables(CSPSolver* csps) const;

		private:

                        std::auto_ptr<const GroundConstraint> a_;
                        std::auto_ptr<const GroundConstraint> b_;
                        CSPLit::Type type_;

	};
    typedef std::vector<std::pair<unsigned int, Constraint*> > ConstraintVec;

}
#endif
