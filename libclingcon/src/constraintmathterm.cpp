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

#include <clingcon/constraintmathterm.h>
#include <clingcon/constraintconstterm.h>
#include <clingcon/exception.h>
#include <gringo/grounder.h>
#include <gringo/varterm.h>
#include <gringo/constterm.h>
#include <gringo/rellit.h>
#include <gringo/prgvisitor.h>
#include <gringo/exceptions.h>
#include <gringo/litdep.h>

namespace
{
	int ipow(int a, int b)
	{
		if(b < 0) { return 0; }
		else
		{
			int r = 1;
			while(b > 0)
			{
				if(b & 1) { r *= a; }
				b >>= 1;
				a *= a;
			}
			return r;
		}
	}
}
namespace Clingcon
{
	ConstraintMathTerm::ConstraintMathTerm(const Loc &loc, const MathTerm::Func &f, ConstraintTerm *a, ConstraintTerm *b) :
		ConstraintTerm(loc), f_(f), a_(a), b_(b)
	{
	}

        Val ConstraintMathTerm::val(Grounder *g) const
	{
            if((a_.get() && a_->isNumber(g)))
            {
                Val va = a_->val(g);
                if(f_ == MathTerm::UMINUS)   { return va.invert(g); }
                else if(f_ == MathTerm::ABS) { assert(va.type == Val::NUM); return Val::number(std::abs(va.num)); }
            }


            if((a_.get() && a_->isNumber(g)) && (b_.get() && b_->isNumber(g)))
            {
                Val va = a_->val(g);
                Val vb = b_->val(g);
                switch(f_)
                {
                case MathTerm::PLUS:  { return Val::number(va.num + vb.num); }
                case MathTerm::MINUS: { return Val::number(va.num - vb.num); }
                case MathTerm::MULT:  { return Val::number(va.num * vb.num); }
                case MathTerm::DIV:   { if (vb.num == 0) { std::cerr << "Warning: Division by 0 detected, using #undef in csp constraint." << std::endl; return Val::undef(); } else return Val::number(va.num / vb.num); }
                case MathTerm::MOD:   { if (vb.num == 0) { std::cerr << "Warning: Modulo by 0 detected, using #undef in csp constraint." << std::endl; return Val::undef(); }   else return Val::number(va.num % vb.num); }
                case MathTerm::POW:   { return Val::number(ipow(va.num, vb.num)); }
                case MathTerm::AND:   { return Val::number(va.num & vb.num); }
                case MathTerm::XOR:   { return Val::number(va.num ^ vb.num); }
                case MathTerm::OR:    { return Val::number(va.num | vb.num); }
                default:    { assert(false); return Val::fail(); }
                }
            }
            assert(false);
            return Val::fail();
	}

	bool ConstraintMathTerm::unify(Grounder *grounder, const Val &v, int binder) const
	{
		(void)binder;
                if(isNumber(grounder)) return v == val(grounder);
		assert(false);
		return false;
	}

	void ConstraintMathTerm::vars(VarSet &v) const
	{
		a_->vars(v);
		if(b_.get()) b_->vars(v);
	}

	void ConstraintMathTerm::visit(PrgVisitor *visitor, bool bind)
	{
		a_->visit(visitor,bind);
		b_->visit(visitor,bind);
		//(void)bind;
		//visitor->visit(a_.get(), false);
		//if(b_.get()) visitor->visit(b_.get(), false);
	}

        bool ConstraintMathTerm::match(Grounder* g)
        {
            assert(a_.get());
            a_->match(g);
            if (b_.get())
                b_->match(g);
        }

        bool ConstraintMathTerm::isNumber(Grounder* g) const
	{
                return a_->isNumber(g) && (!b_.get() || b_->isNumber(g));
	}

	void ConstraintMathTerm::print(Storage *sto, std::ostream &out) const
	{
                if(b_.get()){ out << "("; a_->print(sto, out);}
		switch(f_)
		{
                        case MathTerm::PLUS:   out << "+"; break;
                        case MathTerm::MINUS:  out << "-"; break;
                        case MathTerm::MULT:   out << "*"; break;
                        case MathTerm::DIV:    out << "/"; break;
                        case MathTerm::MOD:    out << "\\"; break;
                        case MathTerm::POW:    out << "**"; break;
                        case MathTerm::AND:    out << "&"; break;
                        case MathTerm::XOR:    out << "^"; break;
                        case MathTerm::OR:     out << "?"; break;
                        case MathTerm::UMINUS: out << "0-"; break;
                        case MathTerm::ABS:    break;
		}
                if(b_.get()) {b_->print(sto, out);out << ")";}
		else
		{
			out << "|";
			a_->print(sto, out);
			out << "|";
		}
	}

        void ConstraintMathTerm::normalize(Lit *parent, const Ref &ref, Grounder *g, const Lit::Expander& expander, bool )
	{
            if(a_.get()) a_->normalize(parent, PtrRef(a_), g, expander, false);
            if(b_.get()) b_->normalize(parent, PtrRef(b_), g, expander, false);
	}


        ConstraintMathTerm *ConstraintMathTerm::clone() const
	{
		return new ConstraintMathTerm(*this);
	}

        GroundConstraint* ConstraintMathTerm::toGroundConstraint(Grounder* g)
	{
            GroundConstraint* ret;
                GroundConstraint::Operator o;
		switch(f_)
		{
                        case MathTerm::PLUS:     o=GroundConstraint::PLUS; break;
                        case MathTerm::MINUS:    o=GroundConstraint::MINUS; break;
                        case MathTerm::MULT:     o=GroundConstraint::TIMES; break;
                        case MathTerm::DIV:      o=GroundConstraint::DIVIDE; break;
                        case MathTerm::ABS:      o=GroundConstraint::ABS; break;
                        case MathTerm::UMINUS:
                        {
                            ret = new GroundConstraint(g,GroundConstraint::MINUS, ConstraintConstTerm(a_->loc(), new ConstTerm(loc(),Val::number(0))).toGroundConstraint(g), a_->toGroundConstraint(g));
                            ret->simplify();
                            return ret;
                        }
                        default: throw CSPException("Unsupported Operator");
		}

                ret = new GroundConstraint(g,o,a_.get() ? a_->toGroundConstraint(g) : 0, b_.get() ? b_->toGroundConstraint(g) : 0);
                ret->simplify();
                return ret;
	}

	ConstraintMathTerm::~ConstraintMathTerm()
	{
	}
}
