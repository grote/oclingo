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

#include <gringo/formula.h>
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
		IndexAdder(Grounder *g, Formula *f, Instantiator &inst);
		void visit(Lit *lit, bool domain);

	public:
		static void add(Grounder *g, Formula *f, Instantiator &inst);

	private:
		Grounder     *g_;
		Formula      *f_;
		Instantiator &inst_;
		VarSet        bound_;
	};

	class FormulaInitializer : private PrgVisitor
	{
	private:
		FormulaInitializer(Grounder *g);
		void visit(Lit *lit, bool domain);
		void visit(Formula *f, bool choice);

	public:
		static void init(Grounder *g, Formula *f);

	private:
		Grounder *g_;
	};


	class VarCollector : public PrgVisitor
	{
		typedef std::map<uint32_t, VarTerm*> VarMap;
		typedef std::vector<uint32_t> VarStack;
		typedef std::vector<Formula*> GrdQueue;
	public:
		VarCollector(Grounder *grounder);
		void visit(VarTerm *var, bool bind);
		void visit(Term *term, bool bind);
		void visit(Lit *lit, bool domain);
		void visit(Formula *g, bool choice);
		uint32_t collect();
	private:
		VarMap      varMap_;
		VarStack    varStack_;
		GrdQueue    grdQueue_;
		Formula *grd_;
		uint32_t    vars_;
		uint32_t    level_;
		Grounder   *grounder_;
	};
}

//////////////////////////////// Formula ////////////////////////////////

Formula::Formula(const Loc &loc)
	: Locateable(loc)
	, level_(0)
{
}

void Formula::litDep(LitDep::FormulaNode *litDep)
{
	litDep_.reset(litDep);
}

Formula::~Formula()
{
}

void Formula::simpleInitInst(Grounder *g, Instantiator &inst)
{
	IndexAdder::add(g, this, inst);
}

//////////////////////////////// Statement ////////////////////////////////

Statement::Statement(const Loc &loc)
	: Formula(loc)
{
}

void Statement::init(Grounder *g)
{
	FormulaInitializer::init(g, this);
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

//////////////////////////////// SimpleStatement ////////////////////////////////

SimpleStatement::SimpleStatement(const Loc &loc)
	: Statement(loc)
	, enqueued_(false)
{

}

void SimpleStatement::initInst(Grounder *g)
{
	if(!inst_.get())
	{
		inst_.reset(new Instantiator(this));
		simpleInitInst(g, *inst_);
	}
	inst_->init(g);
}

void SimpleStatement::enqueue(Grounder *g)
{
	if(!enqueued_)
	{
		enqueued_ = true;
		g->enqueue(this);
	}
}

void SimpleStatement::ground(Grounder *g)
{
	doGround(g);
	if(inst_.get()) { inst_->finish(); }
}

SimpleStatement::~SimpleStatement()
{
}

//////////////////////////////// FormulaInitializer ////////////////////////////////

FormulaInitializer::FormulaInitializer(Grounder *g)
	: g_(g)
{ }

void FormulaInitializer::visit(Lit *lit, bool)
{
	lit->visit(this);
}

void FormulaInitializer::visit(Formula *f, bool)
{
	f->initInst(g_);
	f->visit(this);
}

void FormulaInitializer::init(Grounder *g, Formula *f)
{
	FormulaInitializer init(g);
	init.visit(f, false);
}

//////////////////////////////// IndexAdder ////////////////////////////////

IndexAdder::IndexAdder(Grounder *g, Formula *f, Instantiator &inst)
	: g_(g)
	, f_(f)
	, inst_(inst)
{ }

void IndexAdder::visit(Lit *lit, bool)
{
	inst_.append(lit->index(g_, f_, bound_));
}

void IndexAdder::add(Grounder *g, Formula *f, Instantiator &inst)
{
	IndexAdder adder(g, f, inst);
	if(f->litDep()) { f->litDep()->order(g, &adder, adder.bound_); }
	else { f->visit(&adder); }
	inst.fix();
}

//////////////////////////////// VarCollector ////////////////////////////////

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
		else { var->index(res.first->second->index(), res.first->second->level(), res.first->second->level() == level_); }
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

void VarCollector::visit(Formula *grd, bool choice)
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
