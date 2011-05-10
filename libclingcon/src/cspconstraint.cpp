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

#include <clingcon/cspconstraint.h>
#include <clasp/util/misc_types.h>
#include <cassert>

namespace Clingcon {

        Constraint::Constraint()
        {
        }

        Constraint::Constraint(Constraint& cc) : type_(cc.type_), a_(cc.a_), b_(cc.b_)
        {
        }

        Constraint::Constraint(CSPLit::Type type, const GroundConstraint* a, const GroundConstraint* b) : type_(type), a_(a), b_(b)
	{
	}

        Constraint::~Constraint()
	{
	}


        Constraint Constraint::operator=(Constraint& cc)
	{
            type_ = cc.type_;
            a_ = cc.a_;
            b_ = cc.b_;
	}


 //       Constraint::Constraint(const Constraint& cc) :
 //                                                              a_(cc.a_ ? new GroundConstraint(*cc.a_) : 0),
//                                                                b_(cc.b_ ? new GroundConstraint(*cc.b_) : 0),
//																			  lin_(cc.lin_)
//	{
//            assert(false && "Warning, who deletes this stuff");

//	}



        CSPLit::Type Constraint::getType() const
	{
		return type_;
	}

        CSPLit::Type Constraint::getRelation(const GroundConstraint*& a, const GroundConstraint*& b) const
	{
                a = a_.get();
                b = b_.get();
                return type_;
	}


        std::vector<unsigned int> Constraint::getAllVariables(CSPSolver* csps) const
	{
            std::vector<unsigned int> re;
            a_->getAllVariables(re,csps);
            b_->getAllVariables(re,csps);
            return re;
	}

}

