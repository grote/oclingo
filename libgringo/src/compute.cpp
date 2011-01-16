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

#include <gringo/compute.h>
#include <gringo/index.h>
#include <gringo/term.h>
#include <gringo/output.h>
#include <gringo/predlit.h>
#include <gringo/grounder.h>
#include <gringo/prgvisitor.h>
#include <gringo/litdep.h>
#include <gringo/exceptions.h>
#include <gringo/instantiator.h>
#include <gringo/prgvisitor.h>

////////////////////////////// Compute::Head //////////////////////////////

class Compute::Head : public Lit::Decorator
{
public:
	Head(PredLit *lit);
	Lit *decorated() const;
	bool fact() const;
	bool match(Grounder *grounder);
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	Monotonicity monotonicity() const;
	void visit(PrgVisitor *v);
	Lit *clone() const;
	// NOTE: normalize doesn't need to be overwritten
	~Head();
public:
	clone_ptr<PredLit> lit;
};

Compute::Head::Head(PredLit *lit)
	: Lit::Decorator(lit->loc())
	, lit(lit)
{
}

Lit *Compute::Head::decorated() const 
{
	return lit.get(); 
	
}

bool Compute::Head::fact() const
{
	return false;
}

bool Compute::Head::match(Grounder *g)
{
	lit->match(g);
	return true;
}

void Compute::Head::index(Grounder *g, Groundable *gr, VarSet &bound)
{
	gr->instantiator()->append(new MatchIndex(this));
}

Lit::Monotonicity Compute::Head::monotonicity() const
{
	return ANTIMONOTONE;
}

void Compute::Head::visit(PrgVisitor *v)
{
	v->visit(lit.get());
	foreach(Term &a, const_cast<TermPtrVec&>(lit->terms())) { v->visit(&a, false); }
}

Lit *Compute::Head::clone() const 
{
	return new Compute::Head(*this);
}

Compute::Head::~Head()
{
}

////////////////////////////// Compute //////////////////////////////

Compute::Compute(const Loc &loc, PredLit *head, LitPtrVec &body)
	: Statement(loc)
	, head_(new Head(head))
	, body_(body.release())
{
}

void Compute::ground(Grounder *g)
{
	inst_->ground(g);
}

bool Compute::grounded(Grounder *g)
{
	Printer *printer = g->output()->printer<Printer>();
	printer->print(head_->lit.get());
	return true;
}

namespace
{
	class ComputeHeadExpander : public Expander
	{
	public:
		ComputeHeadExpander(Grounder *g, Compute &min);
		void expand(Lit *lit, Type type);
	private:
		Grounder *g_;
		Compute &com_;
	};

	class ComputeBodyExpander : public Expander
	{
	public:
		ComputeBodyExpander(Compute &min);
		void expand(Lit *lit, Type type);
	private:
		Compute &com_;
	};

	ComputeHeadExpander::ComputeHeadExpander(Grounder *g, Compute &com)
		: g_(g)
		, com_(com)
	{
	}

	void ComputeHeadExpander::expand(Lit *lit, Expander::Type type)
	{
		switch(type)
		{
			case RANGE:
			case RELATION:
				com_.body().push_back(lit);
				break;
			case POOL:
				LitPtrVec body;
				foreach(Lit &l, com_.body()) body.push_back(l.clone());
				Compute *o = new Compute(com_.loc(), static_cast<PredLit*>(lit), body);
				g_->addInternal(o);
				break;
		}
	}

	ComputeBodyExpander::ComputeBodyExpander(Compute &min)
		: com_(min)
	{
	}

	void ComputeBodyExpander::expand(Lit *lit, Expander::Type type)
	{
		(void)type;
		com_.body().push_back(lit);
	}
}

void Compute::normalize(Grounder *g)
{
	ComputeHeadExpander headExp(g, *this);
	ComputeBodyExpander bodyExp(*this);
	head_->normalize(g, &headExp);
	for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
	{
		body_[i].normalize(g, &bodyExp);
	}
}

void Compute::visit(PrgVisitor *visitor)
{
	visitor->visit(static_cast<Lit*>(head_.get()), false);
	foreach(Lit &lit, body_) { visitor->visit(&lit, true); }
}

void Compute::print(Storage *sto, std::ostream &out) const
{
	out << "#compute{";
	head_->print(sto, out);
	std::vector<const Lit*> body;
	foreach(const Lit &lit, body_) { body.push_back(&lit); }
	std::sort(body.begin(), body.end(), Lit::cmpPos);
	foreach(const Lit *lit, body)
	{
		out << ":";
		lit->print(sto, out);
	}
	out << "}";
}

void Compute::append(Lit *lit)
{
	body_.push_back(lit);
}

Compute::~Compute()
{
}

