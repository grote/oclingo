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

#include <gringo/inclit.h>
#include <gringo/term.h>
#include <gringo/index.h>
#include <gringo/grounder.h>
#include <gringo/groundable.h>
#include <gringo/instantiator.h>
#include <gringo/litdep.h>
#include <gringo/varcollector.h>
#include <gringo/varterm.h>
#include <gringo/output.h>
#include <gringo/exceptions.h>

IncLit::IncLit(const Loc &loc, IncConfig &config, Type type, uint32_t varId)
	: Lit(loc)
	, config_(config)
	, type_(type)
	, var_(new VarTerm(loc, varId))
{
}

bool IncLit::match(Grounder *grounder)
{
	return !isFalse(grounder);
}

bool IncLit::isFalse(Grounder *g)
{
	if(type_ == BASE)            { return config_.incBase(); }
	else if(config_.incBase())   { return true; }
	else if(type_ == UNBOUND)    { return false; }
	Val v = var_->val(g);
	if(v.type != Val::NUM)       { return true; }
	else if(type_ == CUMULATIVE) { return v.num != config_.incStep; }
	else if(config_.incVolatile) { return v.num != config_.incStep; }
	else                         { return true; }
}

namespace
{
	class IncIndex : public Index
	{
	public:
		IncIndex(IncLit *lit, bool bind);
		std::pair<bool,bool> firstMatch(Grounder *grounder, int binder);
		std::pair<bool,bool> nextMatch(Grounder *grounder, int binder);
		void reset();
		void finish();
		bool hasNew() const;
	private:
		IncLit *lit_;
		int     finished_;
		bool    bind_;
	};

	IncIndex::IncIndex(IncLit *lit, bool bind)
		: lit_(lit)
		, finished_(std::numeric_limits<int>::max())
		, bind_(bind)
	{
		reset();
	}

	void IncIndex::reset()
	{
		if(lit_->type() == IncLit::VOLATILE || lit_->type() == IncLit::CUMULATIVE)
		{
			finished_ = std::numeric_limits<int>::max();
		}
		else if(finished_ != 2) { finished_ = false; }
	}

	void IncIndex::finish()
	{
		if(lit_->type() == IncLit::VOLATILE || lit_->type() == IncLit::CUMULATIVE)
		{
			finished_ = lit_->config().incStep;
		}
		else if(lit_->type() == IncLit::UNBOUND && !lit_->config().incBase())
		{
			// an UNBOUND inc lit stays finished to prevent regrounding
			finished_ = 2;
		}
		else { finished_ = true; }
	}

	bool IncIndex::hasNew() const
	{
		if(lit_->type() == IncLit::VOLATILE || lit_->type() == IncLit::CUMULATIVE)
		{
			return finished_ != lit_->config().incStep;
		}
		else { return !finished_; }
	}

	std::pair<bool,bool> IncIndex::firstMatch(Grounder *grounder, int binder)
	{
		if(lit_->config().incBase())
		{
			if(lit_->type() == IncLit::BASE)  { return std::make_pair(true, hasNew()); }
			else                              { return std::make_pair(false, false); }
		}
		else if(lit_->type() == IncLit::BASE) { return std::make_pair(false, false); }
		if(bind_)
		{
			assert(lit_->type() != IncLit::UNBOUND);
			if(lit_->config().incVolatile || lit_->type() == IncLit::CUMULATIVE)
			{
				grounder->val(lit_->var()->index(), Val::create(Val::NUM, lit_->config().incStep), binder);
				return std::make_pair(true, hasNew());
			}
			else { return std::make_pair(false, false); }
		}
		else
		{
			if(lit_->match(grounder)) { return std::make_pair(true, hasNew()); }
			else                      { return std::make_pair(false, false); }
		}
	}

	std::pair<bool,bool> IncIndex::nextMatch(Grounder *, int)
	{
		return std::make_pair(false, false);
	}
}

void IncLit::index(Grounder *, Groundable *gr, VarSet &bound)
{
	VarSet vars;
	VarVec bind;
	if(type_ == VOLATILE || type_ == CUMULATIVE) { var_->vars(vars); }
	std::set_difference(vars.begin(), vars.end(), bound.begin(), bound.end(), std::back_insert_iterator<VarVec>(bind));
	gr->instantiator()->append(new IncIndex(this, bind.size() > 0));
	bound.insert(bind.begin(), bind.end());
}

void IncLit::accept(::Printer *v)
{
	if(type_ == VOLATILE)
	{
		Printer *printer = v->output()->printer<Printer>();
		printer->print();
	}
}

void IncLit::visit(PrgVisitor *v)
{
	if(type_ == VOLATILE || type_ == CUMULATIVE)
	{
		v->visit(var_.get(), true);
	}
}

void IncLit::print(Storage *sto, std::ostream &out) const
{
	switch(type_)
	{
		case BASE:
		{
			out << "#base";
			break;
		}
		case UNBOUND:
		{
			out << "#unbound";
			break;
		}
		case CUMULATIVE:
		{
			out << "#cumulative(";
			var_->print(sto, out);
			out << ")";
			break;
		}
		case VOLATILE:
		{
			out << "#volatile(";
			var_->print(sto, out);
			out << ")";
			break;
		}
	}
}

void IncLit::normalize(Grounder *, Expander *)
{
}

void IncLit::bind()
{
	assert(type_ == UNBOUND);
	type_ = CUMULATIVE;
}

Lit *IncLit::clone() const
{
	return new IncLit(*this);
}

double IncLit::score(Grounder *) const
{
	return -1;
}

IncLit::~IncLit()
{
}
