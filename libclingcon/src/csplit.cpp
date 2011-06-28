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

	CSPLit::CSPLit(const Loc &loc, Type t, ConstraintTerm *a, ConstraintTerm *b)
		: Lit(loc)
		, t_(t)
		, a_(a)
		, b_(b)
	{
	}

	//
        bool CSPLit::match(Grounder *g)
	{
            if (a_.get())
                a_->match(g);
            if (b_.get())
                b_->match(g);
            return true;
	}

        Index* CSPLit::index(Grounder *, Formula *, VarSet &)
	{
                return new MatchIndex(this);
	}

        void CSPLit::grounded(Grounder *grounder)
        {
            ag_ = a_->toGroundConstraint(grounder);
            bg_ = b_->toGroundConstraint(grounder);
        }

	void CSPLit::accept(::Printer *v)
	{
		(void)v;
	        Printer *printer = v->output()->printer<Printer>();
                printer->begin(t_,ag_,bg_);
	}

	void CSPLit::visit(PrgVisitor *v)
	{
		if(a_.get()) a_->visitVarTerm(v);
		if(b_.get()) b_->visitVarTerm(v);
	}

	void CSPLit::print(Storage *sto, std::ostream &out) const
	{
		a_->print(sto, out);
		switch(t_)
		{
			case CSPLit::GREATER: out << "$>"; break;
			case CSPLit::LOWER:   out << "$<"; break;
			case CSPLit::EQUAL:   out << "$=="; break;
                        case CSPLit::GEQUAL:   out << "$>="; break;
                        case CSPLit::LEQUAL:   out << "$=<"; break;
			case CSPLit::INEQUAL: out << "$!="; break;
			case CSPLit::ASSIGN:  out << "$:="; break;
		}
		b_->print(sto, out);
	}

        void CSPLit::normalize(Grounder *g, const Lit::Expander& expander)
	{
                if(a_.get()) a_->normalize(this, ConstraintTerm::PtrRef(a_), g, expander, false);
                if(b_.get()) b_->normalize(this, ConstraintTerm::PtrRef(b_), g, expander, false);
	}

	CSPLit *CSPLit::clone() const
	{
		return new CSPLit(*this);
	}

	void CSPLit::revert()
	{
            t_ = revert(t_);
        }

	CSPLit::~CSPLit()
	{
	}

}
