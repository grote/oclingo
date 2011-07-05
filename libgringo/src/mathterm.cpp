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

	void catchVal(Grounder *g, const MathTerm *term, const Val*val)
	{
		std::ostringstream oss;
		if(term->f() == MathTerm::UMINUS)
		{
			oss << "cannot invert term ";
			val->print(g, oss);
		}
		else
		{
			oss << "cannot convert ";
			val->print(g, oss);
			oss << " to integer";
		}
		std::string str(oss.str());
		oss.str("");
		term->print(g, oss);
		throw TypeException(str, StrLoc(g, term->loc()), oss.str());
	}
}

MathTerm::MathTerm(const Loc &loc, const Func &f, Term *a, Term *b) :
	Term(loc), f_(f), a_(a), b_(b)
{
}

Val MathTerm::val(Grounder *g) const
{
	try
	{
		// TODO: what about moving all the functions into Val
		switch(f_)
		{
			case PLUS:   return Val::create(Val::NUM, a_->val(g).number() + b_->val(g).number()); break;
			case MINUS:  return Val::create(Val::NUM, a_->val(g).number() - b_->val(g).number()); break;
			case MULT:   return Val::create(Val::NUM, a_->val(g).number() * b_->val(g).number()); break;
			case DIV:    return Val::create(Val::NUM, a_->val(g).number() / b_->val(g).number()); break;
			case MOD:    return Val::create(Val::NUM, a_->val(g).number() % b_->val(g).number()); break;
			case POW:    return Val::create(Val::NUM, ipow(a_->val(g).number(), b_->val(g).number())); break;
			case AND:    return Val::create(Val::NUM, a_->val(g).number() & b_->val(g).number()); break;
			case XOR:    return Val::create(Val::NUM, a_->val(g).number() ^ b_->val(g).number()); break;
			case OR:     return Val::create(Val::NUM, a_->val(g).number() | b_->val(g).number()); break;
			case ABS:    return Val::create(Val::NUM, std::abs(a_->val(g).number())); break;
			case UMINUS: return a_->val(g).invert(g); break;
		}
	}
	catch(const Val *val) { catchVal(g, this, val); }
	assert(false);
	return Val::create();
}

bool MathTerm::unify(Grounder *g, const Val &v, int binder) const
{
	if(constant()) { return v == val(g); }
	else if(f_ == UMINUS || b_->constant())
	{
		assert(a_->unifiable());
		try
		{
			switch(f_)
			{
				case PLUS:   { return v.type == Val::NUM && a_->unify(g, Val::create(Val::NUM, v.num - b_->val(g).number()), binder); }
				case MINUS:  { return v.type == Val::NUM && a_->unify(g, Val::create(Val::NUM, v.num + b_->val(g).number()), binder); }
				case UMINUS: { return v.type == Val::NUM && a_->unify(g, v, binder); }
				case MULT:
				{
					int32_t b = b_->val(g).number();
					return v.type == Val::NUM && v.num % b == 0 && a_->unify(g, Val::create(Val::NUM, v.num / b), binder);
				}
				default: { break; }
			}
		} catch(const Val *val) { catchVal(g, this, val); }
	}
	else
	{
		assert(a_->constant() && b_->unifiable());
		try
		{
			switch(f_)
			{
				case PLUS:  { return v.type == Val::NUM && b_->unify(g, Val::create(Val::NUM, v.num - a_->val(g).number()), binder); }
				case MINUS: { return v.type == Val::NUM && b_->unify(g, Val::create(Val::NUM, a_->val(g).number() - v.num), binder); }
				case MULT:
				{
					int32_t a = a_->val(g).number();
					return v.type == Val::NUM && v.num % a == 0 && b_->unify(g, Val::create(Val::NUM, v.num / a), binder);
				}
				default: { break; }
			}
		} catch(const Val *val) { catchVal(g, this, val); }
	}
	assert(false);
	return false;
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
		case PLUS:  out << "+"; break;
		case MINUS: out << "-"; break;
		case MULT:  out << "*"; break;
		case DIV:   out << "/"; break;
		case MOD:   out << "\\"; break;
		case POW:   out << "**"; break;
		case AND:   out << "&"; break;
		case XOR:   out << "^"; break;
		case OR:    out << "?"; break;
		case UMINUS:
		case ABS:   break;
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
	if(a_.get()) a_->normalize(parent, PtrRef(a_), g, e, false);
	if(b_.get()) b_->normalize(parent, PtrRef(b_), g, e, false);
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
