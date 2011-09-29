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

        static int counter = 0;
        Constraint::Constraint()
        {
        }

        Constraint::Constraint(Constraint& cc) : type_(cc.type_), a_(cc.a_), b_(cc.b_), lhs_(cc.lhs_), rhs_(cc.rhs_)
        {
        }

        Constraint::Constraint(CSPLit::Type type, const GroundConstraint* a, const GroundConstraint* b) : type_(type), a_(a), b_(b)
	{
	}

        Constraint::Constraint(CSPLit::Type type) : type_(type), a_(0), b_(0)
        {
        }

        Constraint::~Constraint()
	{
	}


        void Constraint::add(const Constraint* a)
        {
            if (lhs_.get()==0)
                lhs_.reset(a);
            else
                rhs_.reset(a);
        }

        bool Constraint::isSimple() const
        {
            if (lhs_.get()==0)
                return true;
            return false;
        }


        CSPLit::Type Constraint::getType() const
	{
		return type_;
	}

        CSPLit::Type Constraint::getRelations(const GroundConstraint*& a, const GroundConstraint*& b) const
	{
                a = a_.get();
                b = b_.get();
                return type_;
	}

        CSPLit::Type Constraint::getConstraints(const Constraint*& a, const Constraint*& b) const
        {
                a = lhs_.get();
                b = rhs_.get();
                return type_;
        }


        void Constraint::registerAllVariables(CSPSolver* csps) const
	{
            if (lhs_.get())
            {
                lhs_->registerAllVariables(csps);
                rhs_->registerAllVariables(csps);
            }
            else
            {
                a_->registerAllVariables(csps);
                b_->registerAllVariables(csps);
            }
	}

        void Constraint::getAllVariables(std::vector<unsigned int>& vec, CSPSolver* csps) const
        {
            if (lhs_.get())
            {
                lhs_->getAllVariables(vec, csps);
                rhs_->getAllVariables(vec, csps);
            }
            else
            {
                a_->getAllVariables(vec, csps);
                b_->getAllVariables(vec, csps);
            }
        }

}

