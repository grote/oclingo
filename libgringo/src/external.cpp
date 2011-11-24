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

#include <gringo/external.h>
#include <gringo/lit.h>
#include <gringo/term.h>
#include <gringo/output.h>
#include <gringo/predlit.h>
#include <gringo/grounder.h>
#include <gringo/prgvisitor.h>
#include <gringo/litdep.h>
#include <gringo/exceptions.h>
#include <gringo/instantiator.h>

External::External(const Loc &loc, PredLit *head, LitPtrVec &body)
	: SimpleStatement(loc)
	, head_(head)
	, body_(body.release())
{
	head_->head(true);
}

void External::endGround(Grounder *g)
{
	head_->endGround(g);
}

void External::addDomain(Grounder *g)
{
	head_->addDomain(g, false);
}

bool External::grounded(Grounder *g)
{
	head_->grounded(g);
	if (head_->fact()) { return true; }
	addDomain(g);
	Printer *printer = g->output()->printer<Printer>();
	printer->begin();
	if(head_.get()) { head_->accept(printer); }
	printer->endHead();
	foreach(Lit &lit, body_)
	{
		lit.grounded(g);
		if(!lit.fact()) { lit.accept(printer); }
		else if(lit.forcePrint()) { lit.accept(printer); }
	}
	printer->end();
	return true;
}

void External::expandHead(Grounder *g, Lit *lit, Lit::ExpansionType type)
{
	switch(type)
	{
		case Lit::RANGE:
		case Lit::RELATION:
		{
			body_.push_back(lit);
			break;
		}
		case Lit::POOL:
		{
			LitPtrVec body;
			foreach(Lit &l, body_) { body.push_back(l.clone()); }
			g->addInternal(new External(loc(), static_cast<PredLit*>(lit), body));
			break;
		}
	}
}

void External::normalize(Grounder *g)
{
	head_->normalize(g, boost::bind(&External::expandHead, this, g, _1, _2));
	for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
	{
		body_[i].normalize(g, boost::bind(&External::append, this, _1));
	}
}

void External::visit(PrgVisitor *visitor)
{
	visitor->visit(head_.get(), false);
	foreach(Lit &lit, body_) { visitor->visit(&lit, false); }
}

void External::print(Storage *sto, std::ostream &out) const
{
	out << "#external ";
	head_->print(sto, out);
	std::vector<const Lit*> body;
	foreach(const Lit &lit, body_) { body.push_back(&lit); }
	std::sort(body.begin(), body.end(), Lit::cmpPos);
	foreach(const Lit *lit, body)
	{
		out << ":";
		lit->print(sto, out);
	}
	out << ".";
}

void External::append(Lit *lit)
{
	body_.push_back(lit);
}

External::~External()
{
}
