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

#include <clingcon/csplit.h>
//#include <gringo/term.h>
#include <gringo/index.h>
#include <gringo/grounder.h>
#include <gringo/formula.h>
#include <gringo/instantiator.h>
#include <gringo/litdep.h>
#include <clingcon/constraintterm.h>
#include <gringo/output.h>

namespace Clingcon
{

    CSPLit::CSPLit(const Loc &loc, Type t, ConstraintTerm *a, ConstraintTerm *b, bool sign)
        : Lit(loc)
        , t_(t)
        , a_(a)
        , b_(b)
        , rhs_(0)
        , lhs_(0)
        , isFact_(false)
        , sign_(sign)
    {
        switch(t_)
        {
        case CSPLit::GREATER:
        case CSPLit::GEQUAL:
        case CSPLit::INEQUAL:
            revertComparison();
            sign_=!sign_;
            break;
        case CSPLit::ASSIGN:  assert(false);  break;

        }
    }

    CSPLit::CSPLit(const Loc &loc, Type t, CSPLit *a, CSPLit *b, bool sign)
        : Lit(loc)
        , t_(t)
        , a_(0)
        , b_(0)
        , rhs_(a)
        , lhs_(b)
        , isFact_(false)
        , sign_(sign)
    {
    }

    //
    bool CSPLit::match(Grounder *g)
    {
        if (lhs_.get())
        {
            lhs_->match(g);
            rhs_->match(g);    
        }
        else
        {
            if (a_.get())
                a_->match(g);
            if (b_.get())
                b_->match(g);
        }
        if (isFixed(g))
        {
            isFact_ = getValue(g);
            return isFact_;
        }
        return true;
    }

    bool CSPLit::isFixed(Grounder *g) const
    {
        if (lhs_.get())
        {
            return lhs_->isFixed(g) && rhs_->isFixed(g);
        }
        else
        {
            return (a_.get() && a_->isNumber(g) && b_.get() && b_->isNumber(g) );
        }
    }

    //pre, everything is fixed
    bool CSPLit::getValue(Grounder *g) const
    {
        if (lhs_.get())
        {

            switch(t_)
            {
            case CSPLit::AND: return (lhs_->getValue(g) && rhs_->getValue(g))^sign_;
            case CSPLit::OR:  return (lhs_->getValue(g) || rhs_->getValue(g))^sign_;
            case CSPLit::XOR: return (lhs_->getValue(g) ^  rhs_->getValue(g))^sign_;
            case CSPLit::EQ:  return (lhs_->getValue(g) == rhs_->getValue(g))^sign_;
            }
            assert(false);
            return false;
        }
        else
        {
            if (a_.get() && a_->isNumber(g) && b_.get() && b_->isNumber(g))
            {
                switch(t_)
                {
                case CSPLit::GREATER: return (a_->val(g).number() > b_->val(g).number())^sign_;
                case CSPLit::LOWER:   return (a_->val(g).number() < b_->val(g).number())^sign_;
                case CSPLit::EQUAL:   return (a_->val(g).number() == b_->val(g).number())^sign_;
                case CSPLit::GEQUAL:  return (a_->val(g).number() >= b_->val(g).number())^sign_;
                case CSPLit::LEQUAL:  return (a_->val(g).number() <= b_->val(g).number())^sign_;
                case CSPLit::INEQUAL: return (a_->val(g).number() != b_->val(g).number())^sign_;
                case CSPLit::ASSIGN:  assert(false);  break;
                }
            }
        }
        assert(false);
        return false;

    }

    bool CSPLit::fact() const
    {
        return isFact_;
    }

    Index* CSPLit::index(Grounder *g, Formula *f, VarSet &v)
    {

        if (lhs_.get())
        {
            lhs_->index(g,f,v);
            rhs_->index(g,f,v);
        }
        else
        {
            if (a_.get())
                a_->initInst(g);
            if (b_.get())
                b_->initInst(g);

        }
        return new MatchIndex(this);
    }

    void CSPLit::grounded(Grounder *grounder)
    {
        if (lhs_.get())
        {
            lhs_->grounded(grounder);
            rhs_->grounded(grounder);
        }
        else
        {
            ag_ = a_->toGroundConstraint(grounder);
            bg_ = b_->toGroundConstraint(grounder);
        }
    }

    void CSPLit::accept(::Printer *v)
    {
        Printer *printer = v->output()->printer<Printer>();
        printer->start();
        if (lhs_.get())
        {
            printer->open();
            printer->conn(t_);
            lhs_->accept(v);
            rhs_->accept(v);
            printer->close();
        }
        else
            printer->normal(t_,ag_,bg_,sign_);
        printer->end();
    }

    void CSPLit::visit(PrgVisitor *v)
    {
        if (lhs_.get())
        {
            lhs_->visit(v);
            rhs_->visit(v);

        }
        else
        {
            if(a_.get()) a_->visitVarTerm(v);
            if(b_.get()) b_->visitVarTerm(v);
        }
    }

    void CSPLit::print(Storage *sto, std::ostream &out) const
    {
        if (sign_)
            out << "not ";
        if (lhs_.get())
        {
            out << "(";
            lhs_->print(sto, out);
            switch(t_)
            {
            case CSPLit::AND: out << " $and "; break;
            case CSPLit::OR:  out << " $or ";  break;
            case CSPLit::XOR: out << " $xor "; break;
            case CSPLit::EQ:  out << " $eq ";  break;
            }
            rhs_->print(sto, out);
            out << ")";
        }
        else
        {
            a_->print(sto, out);
            switch(t_)
            {
            case CSPLit::GREATER: out << "$>";   break;
            case CSPLit::LOWER:   out << "$<";   break;
            case CSPLit::EQUAL:   out << "$==";  break;
            case CSPLit::GEQUAL:   out << "$>="; break;
            case CSPLit::LEQUAL:   out << "$<="; break;
            case CSPLit::INEQUAL: out << "$!=";  break;
            case CSPLit::ASSIGN:  out << "$:=";  assert(false); break;
            }
            b_->print(sto, out);
        }
    }

    void CSPLit::normalize(Grounder *g, const Lit::Expander& expander)
    {
        if (lhs_.get())
        {
            lhs_->normalize(g,expander);
            rhs_->normalize(g,expander);
        }
        else
        {
            if(a_.get()) a_->normalize(this, ConstraintTerm::PtrRef(a_), g, expander, false);
            if(b_.get()) b_->normalize(this, ConstraintTerm::PtrRef(b_), g, expander, false);
        }
    }

    CSPLit *CSPLit::clone() const
    {
        if (lhs_.get())
        {
            CSPLit* templ = new CSPLit(*lhs_);
            CSPLit* tempr = new CSPLit(*rhs_);
            return new CSPLit(loc(), t_, templ, tempr,sign_);
        }
        else
            return new CSPLit(*this);
    }

    void CSPLit::revert()
    {
        sign_=!sign_;
    }

    void CSPLit::revertComparison()
    {
        if (lhs_.get())
        {
            assert(rhs_);
            lhs_->revert();

            switch(t_)
            {
            case CSPLit::AND: t_ = CSPLit::OR;  rhs_->revert(); break;
            case CSPLit::OR:  t_ = CSPLit::AND; rhs_->revert(); break;
            case CSPLit::XOR: t_ = CSPLit::EQ;  break;
            case CSPLit::EQ:  t_ = CSPLit::XOR; break;
            }
        }
        else
        {
            switch(t_)
            {
            case CSPLit::GREATER:  t_ = CSPLit::LEQUAL;  break;
            case CSPLit::LOWER:    t_ = CSPLit::GEQUAL;  break;
            case CSPLit::EQUAL:    t_ = CSPLit::INEQUAL; break;
            case CSPLit::GEQUAL:   t_ = CSPLit::LOWER;   break;
            case CSPLit::LEQUAL:   t_ = CSPLit::GREATER; break;
            case CSPLit::INEQUAL:  t_ = CSPLit::EQUAL;   break;
            default: assert("Illegal comparison operator in Constraint"!=0);
            }
        }
    }

    CSPLit::~CSPLit()
    {
    }

}
