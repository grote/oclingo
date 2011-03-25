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

#include <gringo/statement.h>
#include <gringo/lit.h>
#include <gringo/litdep.h>
#include <gringo/varterm.h>
#include <gringo/grounder.h>
#include <gringo/exceptions.h>
#include <gringo/instantiator.h>
#include <gringo/grounder.h>

namespace
{
	class IndexAdder : private PrgVisitor
	{
	private:
		IndexAdder(Grounder *g);
		virtual void visit(Lit *lit, bool domain);
		virtual void visit(Groundable *grd, bool choice);
	public:
		static void add(Grounder *g, Statement *s);
	private:
		Grounder   *g_;
		Groundable *grd_;
		VarSet      bound_;
		bool        top_;
		bool        createIndex_;
	};

	class VarCollector : public PrgVisitor
	{
		typedef std::map<uint32_t, VarTerm*> VarMap;
		typedef std::vector<uint32_t> VarStack;
		typedef std::vector<Groundable*> GrdQueue;
	public:
		VarCollector(Grounder *grounder);
		void visit(VarTerm *var, bool bind);
		void visit(Term *term, bool bind);
		void visit(Lit *lit, bool domain);
		void visit(Groundable *g, bool choice);
		uint32_t collect();
	private:
		VarMap      varMap_;
		VarStack    varStack_;
		GrdQueue    grdQueue_;
		Groundable *grd_;
		uint32_t    vars_;
		uint32_t    level_;
		Grounder   *grounder_;
	};
}

//////////////////////////////// Statement ////////////////////////////////

void Statement::init(Grounder *g)
{
	IndexAdder::add(g, this);
}

Statement::Statement(const Loc &loc)
	: Groundable(loc)
{
}

void Statement::check(Grounder *g)
{
	VarCollector collector(g);
	collector.visit(this, choice());
	uint32_t numVars = collector.collect();
	if(numVars > 0)
	{
		LitDep::Builder builder(numVars);
		builder.visit(this, choice());
		VarTermVec terms;
		if(!builder.check(terms))
		{
			std::ostringstream oss;
			print(g, oss);
			UnsafeVarsException ex(StrLoc(g, loc()), oss.str());
			foreach(VarTerm *term, terms)
			{
				oss.str("");
				term->print(g, oss);
				ex.add(StrLoc(g, term->loc()), oss.str());
			}
			throw ex;
		}
	}
}

//////////////////////////////// IndexAdder ////////////////////////////////

IndexAdder::IndexAdder(Grounder *g)
	: g_(g)
	, grd_(0)
	, top_(true)
	, createIndex_(true)
{ }

void IndexAdder::visit(Lit *lit, bool)
{
	lit->visit(this);
	if(createIndex_) { lit->index(g_, grd_, bound_); }
}

void IndexAdder::visit(Groundable *grd, bool)
{
	bool top         = top_;
	bool createIndex = createIndex_;
	top_             = false;
	if(!grd->instantiator())
	{
		Groundable *oldGrd(grd_);
		VarSet      oldBound(bound_);
		grd_         = grd;
		createIndex_ = true;
		grd_->instantiator(new Instantiator(grd_));
		if(grd_->litDep()) { grd_->litDep()->order(g_, this, bound_); }
		else               { grd_->visit(this); }
		grd_->instantiator()->fix();
		grd_ = oldGrd;
		std::swap(bound_, oldBound);
	}
	else
	{
		createIndex_ = false;
		grd->visit(this);
	}
	createIndex_ = createIndex;
	grd->instantiator()->init(g_, top);
}

void IndexAdder::add(Grounder *g, Statement *s)
{
	IndexAdder adder(g);
	adder.visit(s, false);
}

//////////////////////////////// IndexAdder ////////////////////////////////

VarCollector::VarCollector(Grounder *grounder)
	: vars_(0)
	, level_(0)
	, grounder_(grounder)
{
}

void VarCollector::visit(VarTerm *var, bool bind)
{
	(void)bind;
	if(var->anonymous())
	{
		var->index(vars_, level_, true);
		grd_->vars().push_back(vars_);
		vars_++;
		grounder_->reserve(vars_);
	}
	else
	{
		std::pair<VarMap::iterator, bool> res = varMap_.insert(VarMap::value_type(var->nameId(), var));
		if(res.second)
		{
			varStack_.push_back(var->nameId());
			var->index(vars_, level_, true);
			grd_->vars().push_back(vars_);
			vars_++;
			grounder_->reserve(vars_);

		}
		else var->index(res.first->second->index(), res.first->second->level(), res.first->second->level() == level_);
	}
}

void VarCollector::visit(Term *term, bool bind)
{
	term->visit(this, bind);
}

void VarCollector::visit(Lit *lit, bool domain)
{
	(void)domain;
	lit->visit(this);
}

void VarCollector::visit(Groundable *grd, bool choice)
{
	(void)choice;
	grdQueue_.push_back(grd);
}

uint32_t VarCollector::collect()
{
	level_ = 0;
	while(grdQueue_.size() > 0)
	{
		grd_ = grdQueue_.back();
		grdQueue_.pop_back();
		if(grd_)
		{
			grdQueue_.push_back(0);
			varStack_.push_back(std::numeric_limits<uint32_t>::max());
			grd_->level(level_);
			grd_->visit(this);
			level_++;
		}
		else
		{
			level_--;
			while(varStack_.back() != std::numeric_limits<uint32_t>::max())
			{
				varMap_.erase(varStack_.back());
				varStack_.pop_back();
			}
			varStack_.pop_back();
		}
	}
	return vars_;
}
