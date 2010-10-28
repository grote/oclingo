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

IncLit::IncLit(const Loc &loc, IncConfig &config, bool cumulative, uint32_t varId)
	: Lit(loc)
	, config_(config)
	, cumulative_(cumulative)
	, var_(new VarTerm(loc, varId))
{
}

bool IncLit::match(Grounder *grounder)
{
	return !isFalse(grounder);
}

bool IncLit::isFalse(Grounder *g)
{
	Val v = var_->val(g);
	if(v.type != Val::NUM)       { return true; }
	else if(cumulative_)         { return v.num < config_.incBegin || v.num >= config_.incEnd; }
	else if(config_.incVolatile) { return v.num + 1 != config_.incEnd; }
	else                         { return true; }
}

namespace
{
	class IncIndex : public Index
	{
	public:
		IncIndex(uint32_t var, bool cumulative, IncConfig &config);
		std::pair<bool,bool> firstMatch(Grounder *grounder, int binder);
		std::pair<bool,bool> nextMatch(Grounder *grounder, int binder);
		void reset()  { finished_ = std::numeric_limits<int>::max(); }
		void finish() { finished_ = config_.incBegin; }
		bool hasNew() const
		{
			return finished_ != config_.incBegin &&
				   (cumulative_ || config_.incVolatile);
		}
	private:
		int        finished_;
		bool       cumulative_;
		IncConfig &config_;
		uint32_t   var_;
		int        current_;
	};

	IncIndex::IncIndex(uint32_t var, bool cumulative, IncConfig &config)
		: finished_(std::numeric_limits<int>::max())
		, cumulative_(cumulative)
		, config_(config)
		, var_(var)
		, current_(0)
	{
	}

	std::pair<bool,bool> IncIndex::firstMatch(Grounder *grounder, int binder)
	{
		if(cumulative_)  { current_ = config_.incBegin; }
		else             { current_ = config_.incEnd - config_.incVolatile; }
		return nextMatch(grounder, binder);
	}

	std::pair<bool,bool> IncIndex::nextMatch(Grounder *grounder, int binder)
	{
		if(current_ < config_.incEnd)
		{
			grounder->val(var_, Val::create(Val::NUM, current_++), binder);
			return std::make_pair(true, hasNew());
		}
		else { return std::make_pair(false, false); }
	}
}

void IncLit::index(Grounder *g, Groundable *gr, VarSet &bound)
{
	(void)g;
	VarSet vars;
	VarVec bind;
	var_->vars(vars);
	std::set_difference(vars.begin(), vars.end(), bound.begin(), bound.end(), std::back_insert_iterator<VarVec>(bind));
	if(bind.size() > 0)
	{
		gr->instantiator()->append(new IncIndex(var_->index(), cumulative_, config_));
		bound.insert(bind.begin(), bind.end());
	}
	else
	{
		gr->instantiator()->append(new MatchIndex(this));
	}

}

void IncLit::accept(::Printer *v)
{
	assert(!cumulative_);
	Printer *printer = v->output()->printer<Printer>();
	printer->print();
}

void IncLit::visit(PrgVisitor *v)
{
	v->visit(var_.get(), true);
}

void IncLit::print(Storage *sto, std::ostream &out) const
{
	if(cumulative_) { out << "#cumulative("; }
	else { out << "#volatile("; }
	var_->print(sto, out);
	out << ")";
}

void IncLit::normalize(Grounder *g, Expander *expander)
{
	(void)g;
	(void)expander;
}

Lit *IncLit::clone() const
{
	return new IncLit(*this);
}

double IncLit::score(Grounder *) const
{
	return cumulative_ ? config_.incEnd - config_.incBegin : 1;
}

IncLit::~IncLit()
{
}
