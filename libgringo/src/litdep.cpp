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

#include <gringo/litdep.h>
#include <gringo/statement.h>
#include <gringo/varterm.h>
#include <gringo/rellit.h>

#include <gringo/grounder.h>

namespace LitDep
{

class VarNode
{
public:
	VarNode(VarTerm *var);
	VarTerm *var() const;
	void depend(LitNode *litNode);
	void provide(LitNode *litNode);
	void reset();
	bool done();
	void propagate(LitNodeVec &queue);
private:
	bool       done_;
	VarTerm    *var_;
	LitNodeVec provide_;
};

class LitNode
{
private:
	typedef std::vector<VarNode*> VarNodeVec;
public:
	LitNode(Lit *lit);
	void depend(VarNode *varNode);
	void provide(VarNode *varNode);
	void reset();
	void check(LitNodeVec &queue);
	void propagate(LitNodeVec &queue);
	bool done();
	void score(Lit::Score score) { score_ = score; }
	Lit::Score score() const { return score_; }
	Lit *lit() const { return lit_; }
private:
	Lit       *lit_;
	uint32_t   done_;
	uint32_t   depend_;
	Lit::Score score_;
	VarNodeVec provide_;
};

bool LitNodeCmp::operator()(LitNode *a, LitNode *b)
{
	return a->score() < b->score();
}

VarNode::VarNode(VarTerm *var) 
	: done_(false)
	, var_(var)
{ 
}

VarTerm *VarNode::var() const 
{ 
	return var_; 
}

void VarNode::depend(LitNode *litNode)
{ 
	(void)litNode;
}

void VarNode::provide(LitNode *litNode) 
{
	provide_.push_back(litNode); 
}

void VarNode::reset()
{
	done_ = false;
}

bool VarNode::done()
{
	return done_;
}

void VarNode::propagate(LitNodeVec &queue)
{
	if(!done_)
	{
		done_ = true;
		foreach(LitNode *lit, provide_) { lit->propagate(queue); }
	}
}

LitNode::LitNode(Lit *lit)
	: lit_(lit)
	, done_(0)
	, depend_(0)
	, score_(Lit::Score(Lit::LOWEST,0))
{
}

void LitNode::depend(VarNode *varNode) 
{ 
	(void)varNode;
	depend_++;
	done_++;
}

void LitNode::provide(VarNode *varNode) 
{ 
	provide_.push_back(varNode); 
}

void LitNode::reset()
{
	done_ = depend_;
}

void LitNode::check(LitNodeVec &queue)
{
	foreach(VarNode *var, provide_) { var->propagate(queue); }
}

void LitNode::propagate(LitNodeVec &queue)
{
	assert(done_ > 0);
	if(--done_ == 0) { queue.push_back(this); }
}

bool LitNode::done()
{
	return done_ == 0;
}

GrdNode::GrdNode(Groundable *groundable)
	: groundable_(groundable)
{ 
}

void GrdNode::append(LitNode *litNode) 
{ 
	litNodes_.push_back(litNode); 
}

void GrdNode::append(VarNode *varNode) 
{ 
	varNodes_.push_back(varNode); 
}

void GrdNode::reset()
{
	foreach(LitNode &lit, litNodes_) { lit.reset(); }
	foreach(VarNode &var, varNodes_) { var.reset(); }
}

bool GrdNode::check(VarTermVec &terms)
{
	reset();
	LitNodeVec queue;
	foreach(LitNode &lit, litNodes_)
	{
		if(lit.done()) { queue.push_back(&lit); }
	}
	while(!queue.empty())
	{
		LitNode *lit = queue.back();
		queue.pop_back();
		lit->check(queue);
	}
	bool res = true;
	foreach(VarNode &var, varNodes_)
	{
		if(!var.done()) 
		{
			terms.push_back(var.var());
			res = false;
		}
	}
	return res;
}

void GrdNode::order(Grounder *g, PrgVisitor *v, const VarSet &b)
{
	reset();
	LitNodeVec queue;
	foreach(LitNode &lit, litNodes_)
	{
		if(lit.done()) { queue.push_back(&lit); }
	}
	VarSet bound(b);
	uint32_t position = 0;
	while(!queue.empty())
	{
		foreach(LitNode *lit, queue) { lit->score(lit->lit()->score(g, bound)); }
		LitNodeVec::iterator min = std::min_element(queue.begin(), queue.end(), LitNodeCmp());
		LitNode *lit = *min;
		queue.erase(min);
		v->visit(lit->lit(), false);
		//lit->lit()->index(g, groundable_, bound);
		lit->check(queue);
		lit->lit()->position = position++;
	}
}

GrdNode::~GrdNode()
{
}

Builder::Builder(uint32_t vars)
	: varNodes_(vars)
{
}

void Builder::visit(VarTerm *var, bool bind)
{
	assert(var->level() < grdStack_.size());
	VarNode *varNode;
	if(!varNodes_[var->index()])
	{
		varNodes_[var->index()] = varNode = new VarNode(var);
		grdNodes_[grdStack_[var->level()]]->append(varNode);
	}
	else varNode = varNodes_[var->index()];
	if(bind && var->level() == grdStack_.size() - 1)
	{
		varNode->depend(litStack_.back());
		litStack_.back()->provide(varNode);
	}
	else
	{
		varNode->provide(litStack_[var->level()]);
		litStack_[var->level()]->depend(varNode);
	}
}

void Builder::visit(Term *term, bool bind)
{
	term->visit(this, bind);
}

void Builder::visit(Lit *lit, bool domain)
{
	(void)domain;
	LitNode *node = new LitNode(lit);
	litStack_.back() = node;
	grdNodes_[grdStack_.back()]->append(node);
	lit->visit(this);
}

void Builder::visit(Groundable *groundable, bool choice)
{
	(void)choice;
	grdStack_.push_back(grdNodes_.size());
	grdNodes_.push_back(new GrdNode(groundable));
	groundable->litDep(grdNodes_.back());
	litStack_.push_back(0);
	groundable->visit(this);
	grdStack_.pop_back();
	litStack_.pop_back();
}

bool Builder::check(VarTermVec &terms)
{
	bool res = true;
	foreach(GrdNode *grd, grdNodes_)
		if(!grd->check(terms)) res = false;
	return res;
}

Builder::~Builder()
{
}

}

