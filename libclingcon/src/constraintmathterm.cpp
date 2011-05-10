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
#include <clingcon/constraintvarterm.h>
#include <clingcon/exception.h>
#include <gringo/grounder.h>
#include <gringo/varterm.h>
#include <gringo/constterm.h>
#include <gringo/rellit.h>
#include <gringo/prgvisitor.h>
#include <gringo/exceptions.h>

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

	Val ConstraintMathTerm::val(Grounder *grounder) const
	{
		try
		{
			// TODO: what about moving all the functions into Val
			switch(f_)
			{
				case MathTerm::PLUS:  return Val::create(Val::NUM, a_->val(grounder).number() + b_->val(grounder).number()); break;
				case MathTerm::MINUS: return Val::create(Val::NUM, a_->val(grounder).number() - b_->val(grounder).number()); break;
				case MathTerm::MULT:  return Val::create(Val::NUM, a_->val(grounder).number() * b_->val(grounder).number()); break;
				case MathTerm::DIV:   return Val::create(Val::NUM, a_->val(grounder).number() / b_->val(grounder).number()); break;
				case MathTerm::MOD:   return Val::create(Val::NUM, a_->val(grounder).number() % b_->val(grounder).number()); break;
				case MathTerm::POW:   return Val::create(Val::NUM, ipow(a_->val(grounder).number(), b_->val(grounder).number())); break;
				case MathTerm::AND:   return Val::create(Val::NUM, a_->val(grounder).number() & b_->val(grounder).number()); break;
				case MathTerm::XOR:   return Val::create(Val::NUM, a_->val(grounder).number() ^ b_->val(grounder).number()); break;
				case MathTerm::OR:    return Val::create(Val::NUM, a_->val(grounder).number() | b_->val(grounder).number()); break;
				case MathTerm::ABS:   return Val::create(Val::NUM, std::abs(a_->val(grounder).number())); break;
			}
		}
		catch(const Val *val)
		{
			std::ostringstream oss;
			oss << "cannot convert ";
			val->print(grounder, oss);
			oss << " to integer";
			std::string str(oss.str());
			oss.str("");
			print(grounder, oss);
			throw TypeException(str, StrLoc(grounder, loc()), oss.str());
		}
		assert(false);
		return Val::create();
	}

	bool ConstraintMathTerm::unify(Grounder *grounder, const Val &v, int binder) const
	{
		(void)binder;
		if(constant()) return v == val(grounder);
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
		#pragma message "Check with Roland"
		a_->visit(visitor,bind);
		b_->visit(visitor,bind);
		//(void)bind;
		//visitor->visit(a_.get(), false);
		//if(b_.get()) visitor->visit(b_.get(), false);
	}

	bool ConstraintMathTerm::constant() const
	{
		return a_->constant() && (!b_.get() || b_->constant());
	}

	void ConstraintMathTerm::print(Storage *sto, std::ostream &out) const
	{
                if(b_.get()){ out << "("; a_->print(sto, out);}
		switch(f_)
		{
			case MathTerm::PLUS:  out << "+"; break;
			case MathTerm::MINUS: out << "-"; break;
			case MathTerm::MULT:  out << "*"; break;
			case MathTerm::DIV:   out << "/"; break;
			case MathTerm::MOD:   out << "\\"; break;
			case MathTerm::POW:   out << "**"; break;
			case MathTerm::AND:   out << "&"; break;
			case MathTerm::XOR:   out << "^"; break;
			case MathTerm::OR:    out << "?"; break;
			case MathTerm::ABS:   break;
		}
                if(b_.get()) {b_->print(sto, out);out << ")";}
		else
		{
			out << "|";
			a_->print(sto, out);
			out << "|";
		}
	}

	void ConstraintMathTerm::normalize(Lit *parent, const Ref &ref, Grounder *g, Expander *expander, bool unify)
	{
//		if(a_.get()) a_->normalize(parent, PtrRef(a_), g, expander, false);
//		if(b_.get()) b_->normalize(parent, PtrRef(b_), g, expander, false);
//		if((!a_.get() || a_->constant()) && (!b_.get() || b_->constant()))
//		{
//			ref.reset(new ConstraintConstTerm(loc(), val(g)));
//		}
//		else if(unify)
//		{
//			uint32_t var = g->createVar();
//			expander->expand(new RelLit(a_->loc(), RelLit::ASSIGN, new VarTerm(a_->loc(), var), this->toTerm()), Expander::RELATION);
//			ref.reset(new ConstraintVarTerm(a_->loc(), var));
//		}
	}

	ConstraintAbsTerm::Ref* ConstraintMathTerm::abstract(ConstraintSubstitution& subst) const
	{
		return subst.anyVar();
	}

	ConstraintTerm *ConstraintMathTerm::clone() const
	{
		return new ConstraintMathTerm(*this);
	}

        GroundConstraint* ConstraintMathTerm::toGroundConstraint(Grounder* g)
	{
                GroundConstraint::Operator o;
		switch(f_)
		{
                        case MathTerm::PLUS:  o=GroundConstraint::PLUS; break;
                        case MathTerm::MINUS: o=GroundConstraint::MINUS; break;
                        case MathTerm::MULT:  o=GroundConstraint::TIMES; break;
                        case MathTerm::DIV:   o=GroundConstraint::DIVIDE; break;
                        case MathTerm::ABS:   o=GroundConstraint::ABS; break;
                        default: throw CSPException("Unsupported Operator");
		}

                return new GroundConstraint(g,o,a_.get() ? a_->toGroundConstraint(g) : 0, b_.get() ? b_->toGroundConstraint(g) : 0);
	}

	ConstraintMathTerm::~ConstraintMathTerm()
	{
	}
}
