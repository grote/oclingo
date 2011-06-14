// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <gringo/gringo.h>
#include <clingcon/clingcon.h>
#include <gringo/lit.h>
#include <gringo/printer.h>
#include <gringo/index.h>
#include <clingcon/groundconstraint.h>

namespace Clingcon
{
        class CSPLit : public Lit, public Matchable
	{
	public:
                enum Type { ASSIGN, GREATER, LOWER, EQUAL, GEQUAL, LEQUAL, INEQUAL };
	public:
	        class Printer : public ::Printer
	        {
	        public:
                        virtual void begin(CSPLit::Type t, const GroundConstraint* a, const GroundConstraint* b) = 0;
	                virtual ~Printer(){};
		};

		CSPLit(const Loc &loc, Type t, ConstraintTerm *a, ConstraintTerm *b);
                void grounded(Grounder *grounder);
		void normalize(Grounder *g, Expander *expander);
		bool fact() const { return false; }
		void accept(::Printer *v);
                Index* index(Grounder *g, Formula *gr, VarSet &bound);
		void visit(PrgVisitor *visitor);
		bool match(Grounder *grounder);
		void print(Storage *sto, std::ostream &out) const;
		CSPLit *clone() const;
		void revert();
		~CSPLit();
                static Type switchRel(Type t)
                {
                    switch(t)
                    {
                            case CSPLit::GREATER: return CSPLit::LOWER;
                            case CSPLit::LOWER:   return CSPLit::GREATER;
                            case CSPLit::EQUAL:   return CSPLit::INEQUAL;
                            case CSPLit::GEQUAL:   return CSPLit::LEQUAL;
                            case CSPLit::LEQUAL:   return CSPLit::GEQUAL;
                            case CSPLit::INEQUAL: return CSPLit::EQUAL;
                            default: assert("Illegal comparison operator in Constraint"!=0);
                    }
                    assert(false); // should never reach this
                    return CSPLit::ASSIGN;

                }

                static Type revert(Type t)
                {
                    switch(t)
                    {
                            case CSPLit::GREATER: return CSPLit::LEQUAL;
                            case CSPLit::LOWER:   return CSPLit::GEQUAL;
                            case CSPLit::EQUAL:   return CSPLit::INEQUAL;
                            case CSPLit::GEQUAL:   return CSPLit::LOWER;
                            case CSPLit::LEQUAL:   return CSPLit::GREATER;
                            case CSPLit::INEQUAL: return CSPLit::EQUAL;
                            default: assert("Illegal comparison operator in Constraint"!=0);
                    }
                    assert(false); // should never reach this
                    return CSPLit::ASSIGN;

                }

	private:
		Type            t_;
		clone_ptr<ConstraintTerm> a_;
		clone_ptr<ConstraintTerm> b_;
                GroundConstraint*         ag_;
                GroundConstraint*         bg_;
	};
}
