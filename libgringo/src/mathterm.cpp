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

#include <gringo/mathterm.h>
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

MathTerm::MathTerm(const Loc &loc, const Func &f, Term *a, Term *b) :
	Term(loc), f_(f), a_(a), b_(b)
{
}

Val MathTerm::val(Grounder *g) const
{
	Val va = a_->val(g);
	if(f_ == UMINUS)   { return va.invert(g); }
	else if(f_ == ABS) { return va.type == Val::NUM ? Val::number(std::abs(va.num)) : Val::undef(); }
	else
	{
		Val vb = b_->val(g);
		if(va.type == Val::NUM && vb.type == Val::NUM)
		{
			switch(f_)
			{
				case PLUS:  { return Val::number(va.num + vb.num); }
				case MINUS: { return Val::number(va.num - vb.num); }
				case MULT:  { return Val::number(va.num * vb.num); }
				case DIV:   { return vb.num == 0 ? Val::undef() : Val::number(va.num / vb.num); }
				case MOD:   { return vb.num == 0 ? Val::undef() : Val::number(va.num % vb.num); }
				case POW:   { return Val::number(ipow(va.num, vb.num)); }
				case AND:   { return Val::number(va.num & vb.num); }
				case XOR:   { return Val::number(va.num ^ vb.num); }
				case OR:    { return Val::number(va.num | vb.num); }
				default:    { assert(false); return Val::fail(); }
			}
		}
		else { return Val::undef(); }
	}
}

namespace
{
	bool _unify(Grounder *g, MathTerm::Func f, const Term &t, const Val &a, const Val &b, int binder)
	{
		assert(t.unifiable() && !t.constant());
		if(a.type == Val::NUM && b.type == Val::NUM)
		{
			switch(f)
			{
				case MathTerm::PLUS:  { return t.unify(g, Val::number(a.num - b.num), binder); }
				case MathTerm::MINUS: { return t.unify(g, Val::number(a.num + b.num), binder); }
				case MathTerm::MULT:  { return a.num % b.num == 0 && t.unify(g, Val::number(a.num / b.num), binder); }
				default:              { assert(false); return false; }
			}
		}
		else if(a.type != Val::UNDEF) { return false; }
		else                          { return t.unify(g, a, binder); }
	}
}

bool MathTerm::unify(Grounder *g, const Val &v, int binder) const
{
	if(constant()) { return v == val(g); }
	else if(f_ == UMINUS)
	{
		assert(a_->unifiable());
		return a_->unify(g, v.invert(g), binder);
	}
	else
	{
		assert(f_ == PLUS || f_ == MINUS || f_ == MULT);
		assert(a_->constant() || b_->constant());
		if(a_->constant()) { return _unify(g, f_, *b_, f_ == MINUS && v.type == Val::NUM ? v.invert(g) : v, a_->val(g), binder); }
		else               { return _unify(g, f_, *a_, v, b_->val(g), binder); }
	}
}

void MathTerm::vars(VarSet &v) const
{
	a_->vars(v);
	if(b_.get()) b_->vars(v);
}

void MathTerm::visit(PrgVisitor *visitor, bool bind)
{
	visitor->visit(a_.get(), bind && unifiable());
	if(b_.get()) { visitor->visit(b_.get(), bind && unifiable()); }
}

bool MathTerm::constant() const
{
	return a_->constant() && (!b_.get() || b_->constant());
}

void MathTerm::print(Storage *sto, std::ostream &out) const
{
	if(b_.get()) { a_->print(sto, out); }
	switch(f_)
	{
		case PLUS:  { out << "+"; break; }
		case MINUS: { out << "-"; break; }
		case MULT:  { out << "*"; break; }
		case DIV:   { out << "/"; break; }
		case MOD:   { out << "\\"; break; }
		case POW:   { out << "**"; break; }
		case AND:   { out << "&"; break; }
		case XOR:   { out << "^"; break; }
		case OR:    { out << "?"; break; }
		case UMINUS:
		case ABS:   { break; }
	}
	if(b_.get()) { b_->print(sto, out); }
	else if(f_ == UMINUS)
	{
		out << "-";
		a_->print(sto, out);
	}
	else
	{
		out << "|";
		a_->print(sto, out);
		out << "|";
	}
}

bool MathTerm::unifiable() const
{
	if(constant()) { return true; }
	switch(f_)
	{
		case PLUS:
		case MINUS:
		{
			return (a_->unifiable() && b_->constant()) || (b_->unifiable() && a_->constant());
		}
		case MULT:
		{
			if(a_->unifiable() && b_->constant())
			{
				Val b = b_->val(0);
				return b.type != Val::NUM || b.num != 0;
			}
			else if(b_->unifiable() && a_->constant())
			{
				Val a = a_->val(0);
				return a.type != Val::NUM || a.num != 0;
			}
			else { return false; }
		}
		case UMINUS: { return a_->unifiable(); }
		default: { return false; }
	}
}

void MathTerm::normalize(Lit *parent, const Ref &ref, Grounder *g, const Expander &e, bool unify)
{
	if(a_.get()) { a_->normalize(parent, PtrRef(a_), g, e, false); }
	if(b_.get()) { b_->normalize(parent, PtrRef(b_), g, e, false); }
	if((!a_.get() || a_->constant()) && (!b_.get() || b_->constant()))
	{
		ref.reset(new ConstTerm(loc(), val(g)));
	}
	else if(unify && !unifiable())
	{
		uint32_t var = g->createVar();
		e(new RelLit(a_->loc(), RelLit::ASSIGN, new VarTerm(a_->loc(), var), this), Lit::RELATION);
		ref.replace(new VarTerm(a_->loc(), var));
	}
}

AbsTerm::Ref* MathTerm::abstract(Substitution& subst) const
{
	return subst.anyVar();
}

Term *MathTerm::clone() const
{
	return new MathTerm(*this);
}

MathTerm::~MathTerm()
{
}
