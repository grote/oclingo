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
                enum Type { ASSIGN, GREATER, LOWER, EQUAL, GEQUAL, LEQUAL, INEQUAL, AND, OR, XOR, EQ };
	public:
	        class Printer : public ::Printer
	        {
	        public:
                    virtual void normal(CSPLit::Type t, const GroundConstraint* a, const GroundConstraint* b) = 0;
                    virtual void start() = 0;
                    virtual void open() = 0;
                    virtual void conn(CSPLit::Type t) = 0;
                    virtual void close() = 0;
                    virtual void end() = 0;
                    virtual ~Printer(){};
		};

                CSPLit(const Loc &loc, Type t, ConstraintTerm *a, ConstraintTerm *b);
                CSPLit(const Loc &loc, Type t, CSPLit *a, CSPLit *b);
                void grounded(Grounder *grounder);
                void normalize(Grounder *g, const Expander& expander);
                bool fact() const;
		void accept(::Printer *v);
                Index* index(Grounder *g, Formula *gr, VarSet &bound);
		void visit(PrgVisitor *visitor);
		bool match(Grounder *grounder);
		void print(Storage *sto, std::ostream &out) const;
                CSPLit *clone() const;
		void revert();
                ~CSPLit();
        private:
                bool isFixed(Grounder *g) const;
                bool getValue(Grounder *g) const;
                /*static Type switchRel(Type t)
                {
                    switch(t)
                    {
                            case CSPExpr::GREATER:  return CSPExpr::LOWER;
                            case CSPExpr::LOWER:    return CSPExpr::GREATER;
                            case CSPExpr::EQUAL:    return CSPExpr::INEQUAL;
                            case CSPExpr::GEQUAL:   return CSPExpr::LEQUAL;
                            case CSPExpr::LEQUAL:   return CSPExpr::GEQUAL;
                            case CSPExpr::INEQUAL:  return CSPExpr::EQUAL;
                        case CSPExpr::AND:  return CSPExpr::EQUAL;
                        case CSPExpr::OR:  return CSPExpr::EQUAL;
                        case CSPExpr::XOR:  return CSPExpr::EQUAL;
                        case CSPExpr::EQ:  return CSPExpr::EQUAL;
                        todo, not finished

                            default: assert("Illegal comparison operator in Constraint"!=0);
                    }
                    assert(false); // should never reach this
                    return CSPExpr::ASSIGN;

                }*/



	private:
                Type                      t_;
		clone_ptr<ConstraintTerm> a_;
		clone_ptr<ConstraintTerm> b_;
                clone_ptr<CSPLit>         lhs_;
                clone_ptr<CSPLit>         rhs_;
                GroundConstraint*         ag_;
                GroundConstraint*         bg_;
                bool                      isFact_;
	};

        inline CSPLit* new_clone(const CSPLit& a)
        {
                return a.clone();
        }
}
