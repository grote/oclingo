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

#include <gringo/display.h>
#include <gringo/lit.h>
#include <gringo/term.h>
#include <gringo/output.h>
#include <gringo/predlit.h>
#include <gringo/grounder.h>
#include <gringo/prgvisitor.h>
#include <gringo/litdep.h>
#include <gringo/exceptions.h>
#include <gringo/instantiator.h>

Display::Display(const Loc &loc, bool show, PredLit *head, LitPtrVec &body)
	: SimpleStatement(loc)
	, head_(head)
	, body_(body.release())
	, show_(show)
{
}

bool Display::grounded(Grounder *g)
{
	head_->grounded(g);
	Printer *printer = g->output()->printer<Printer>();
	printer->show(show_);
	printer->print(head_.get());
	return true;
}

namespace
{
	class DisplayHeadExpander : public Expander
	{
	public:
		DisplayHeadExpander(Grounder *g, Display &d) : g_(g), d_(d) { }
		void expand(Lit *lit, Type type);
	private:
		Grounder *g_;
		Display  &d_;
	};

	class BodyExpander : public Expander
	{
	public:
		BodyExpander(Display &d) : d_(d) { }
		void expand(Lit *lit, Type type) { (void)type; d_.body().push_back(lit); }
	private:
		Display &d_;
	};

	void DisplayHeadExpander::expand(Lit *lit, Expander::Type type)
	{
		switch(type)
		{
			case RANGE:
			case RELATION:
				d_.body().push_back(lit);
				break;
			case POOL:
				LitPtrVec body;
				foreach(Lit &l, d_.body()) body.push_back(l.clone());
				g_->addInternal(new Display(d_.loc(), d_.show(), static_cast<PredLit*>(lit), body));
				break;
		}
	}
}

void Display::normalize(Grounder *g)
{
	DisplayHeadExpander headExp(g, *this);
	BodyExpander bodyExp(*this);
	head_->normalize(g, &headExp);
	for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
	{
		body_[i].normalize(g, &bodyExp);
	}
}

void Display::visit(PrgVisitor *visitor)
{
	visitor->visit(head_.get(), false);
	foreach(Lit &lit, body_) { visitor->visit(&lit, true); }
}

void Display::print(Storage *sto, std::ostream &out) const
{
	out << (show_ ? "#show " : "#hide ");
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

void Display::append(Lit *lit)
{
	body_.push_back(lit);
}

Display::~Display()
{
}
