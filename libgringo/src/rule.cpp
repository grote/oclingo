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

#include <gringo/rule.h>
#include <gringo/lit.h>
#include <gringo/instantiator.h>
#include <gringo/index.h>
#include <gringo/printer.h>
#include <gringo/litdep.h>
#include <gringo/grounder.h>
#include <gringo/output.h>
#include <gringo/exceptions.h>

Rule::Rule(const Loc &loc, Lit *head, LitPtrVec &body)
	: SimpleStatement(loc)
	, head_(head)
	, body_(body.release())
{
	if(head) { head->head(true); }
}

void Rule::expandHead(Grounder *g, Lit *lit, Lit::ExpansionType type)
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
			LitPtrVec body(body_);
			g->addInternal(new Rule(loc(), lit, body));
			break;
		}
	}
}

void Rule::normalize(Grounder *g)
{
	if(head_.get()) { head_->normalize(g, boost::bind(&Rule::expandHead, this, g, _1, _2)); }
	if(body_.size() > 0)
	{
		Lit::Expander exp = boost::bind(&Rule::append, this, _1);
		for(LitPtrVec::size_type i = 0; i < body_.size(); i++) { body_[i].normalize(g, exp); }
	}
}

bool Rule::edbFact() const
{
	return body_.size() == 0 && head_.get() && head_->edbFact();
}

void Rule::endGround(Grounder *g)
{
	if(head_.get()) { head_->endGround(g); }
}

void Rule::addDomain(Grounder *g, bool fact)
{
	if(head_.get())
	{
		try { head_->addDomain(g, fact); }
		catch(const AtomRedefinedException &ex)
		{
			std::stringstream ss;
			print(g, ss);
			throw ModularityException(StrLoc(g, loc()), ss.str(), ex.what());
		}
	}
}

bool Rule::grounded(Grounder *g)
{
	if(head_.get())
	{
		head_->grounded(g);
		if(head_->fact())
		{
			addDomain(g, true);
			return true;
		}
	}
	Printer *printer = g->output()->printer<Printer>();
	printer->begin();
	if(head_.get()) { head_->accept(printer); }
	printer->endHead();
	bool fact = true;
	foreach(Lit &lit, body_)
	{
		lit.grounded(g);
		if(!lit.fact())
		{
			lit.accept(printer);
			fact = false;
		}
		else if(lit.forcePrint()) { lit.accept(printer); }
	}
	printer->end();
	if(head_.get()) { addDomain(g, fact); }
	return true;
}

void Rule::append(Lit *l)
{
	body_.push_back(l);
}

void Rule::visit(PrgVisitor *v)
{
	for(size_t i = 0, n = body_.size(); i < n; i++) { v->visit(&body_[i], false); }
	if(head_.get()) { v->visit(head_.get(), false); }
}

void Rule::print(Storage *sto, std::ostream &out) const
{
	if(head_.get()) { head_->print(sto, out); }
	if(body_.size() > 0)
	{
		std::vector<const Lit*> body;
		foreach(const Lit &lit, body_) { body.push_back(&lit); }
		std::sort(body.begin(), body.end(), Lit::cmpPos);
		out << ":-";
		bool comma = false;
		foreach(const Lit *lit, body)
		{
			if(comma) out << ",";
			else comma = true;
			lit->print(sto, out);
		}
	}
	out << ".";
}

Rule::~Rule()
{
}

