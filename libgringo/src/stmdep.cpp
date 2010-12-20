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

#include <gringo/stmdep.h>
#include <gringo/domain.h>
#include <gringo/predlit.h>
#include <gringo/statement.h>
#include <gringo/grounder.h>
#include <gringo/exceptions.h>

namespace StmDep
{

namespace
{

template <class T>
class Remember
{
public:
	Remember(T &ref, const T &val);
	~Remember();
private:
	T old_;
	T &ref_;
};

template <class T>
Remember<T>::Remember(T &ref, const T &val)
	: old_(ref)
	, ref_(ref)
{
	ref = val;
}

template <class T>
Remember<T>::~Remember()
{
	ref_ = old_;
}

}

Node::Node()
	: visited_(0)
	, component_(0)
{
}

PredNode::PredNode()
	: pred_(0)
	, next_(0)
{ }

void PredNode::depend(StmNode *stm)
{
	depend_.push_back(stm);
	stm->provide(this);
}

Node *PredNode::node(uint32_t i)
{
	if(i < depend_.size()) return depend_[i];
	else return 0;
}

Node *PredNode::next()
{
	for(; next_ < depend_.size(); next_++)
		if(!depend_[next_]->marked())
			return depend_[next_++];
	return 0;
}

bool PredNode::root()
{
	bool root = true;
	foreach(DependVec::value_type &stm, depend_)
	{
		assert(stm->visited());
		if(!stm->hasComponent() && stm->visited() < visited())
		{
			root = false;
			visited(stm->visited());
		}
	}
	return root;
}

bool PredNode::edbFact() const
{
	assert(!isNull());
	return depend_.empty();
}

bool PredNode::empty() const
{
	assert(!isNull());
	return pred_->dom()->size() == 0;
}

void PredNode::print(Storage *sto, std::ostream &out) const
{
	assert(!isNull());
	pred_->print(sto, out);
}

const Loc &PredNode::loc() const
{
	assert(!isNull());
	return pred_->loc();
}

void PredNode::provide(Todo &todo)
{
	todo.provided++;
	provide_.push_back(&todo);
}

PredNode::TodoVec &PredNode::provide()
{
	return provide_;
}

StmNode::StmNode(Statement *stm)
	: stm_(stm)
	, next_(0)
{ }

void StmNode::depend(PredNode *pred, Type t)
{
	depend_.push_back(DependVec::value_type(pred, t));
}

void StmNode::provide(PredNode *pred)
{
	provide_.push_back(pred);
}

Node *StmNode::node(uint32_t i)
{
	if(i < depend_.size()) return depend_[i].first;
	else return 0;
}

Node::Type StmNode::type(uint32_t i)
{
	return depend_[i].second;
}

Node *StmNode::next()
{
	for(; next_ < depend_.size(); next_++)
		if(!depend_[next_].first->marked())
			return depend_[next_++].first;
	return 0;
}

void StmNode::addToComponent(Grounder *g)
{
	// TODO: nested groundables have to be added to the component too
	//       this should be easily possible with a visitor
	g->addToComponent(stm_);
	foreach(PredNode *pred, provide_)
	{
		foreach(Todo *todo, pred->provide())
		{
			if(--todo->provided == 0)
			{
				todo->provided = component();
			}
		}
	}
}

bool StmNode::root()
{
	bool root = true;
	foreach(DependVec::value_type &pred, depend_)
	{
		assert(pred.first->visited());
		if(!pred.first->hasComponent() && pred.first->visited() < visited())
		{
			root = false;
			visited(pred.first->visited());
		}
	}
	return root;
}

void StmNode::print(Storage *sto, std::ostream &out) const
{
	stm_->print(sto, out);
}

const Loc &StmNode::loc() const
{
	return stm_->loc();
}

Builder::Builder(Grounder *g)
	: domain_(false)
	, monotonicity_(Lit::MONOTONE)
	, head_(true)
	, choice_(false)
	, predNodes_(g->domains().size())
{
}

void Builder::visit(PredLit *pred)
{
	if(domain_) { todo_.push_back(Todo(&stmNodes_.back(), pred, Node::DOM)); }
	else if(head_)
	{
		if(monotonicity_ == Lit::ANTIMONOTONE)
		{
			todo_.push_back(Todo(&stmNodes_.back(), pred, Node::NEG));
		}
		else
		{
			Domain *dom = pred->dom();
			PredNode *node = new PredNode();
			predNodes_[dom->domId()].push_back(node);
			node->pred(pred);
			if(monotonicity_ != Lit::MONOTONE || choice_) { stmNodes_.back().depend(node, Node::NEG); }
			node->depend(&stmNodes_.back());
		}
	}
	else { todo_.push_back(Todo(&stmNodes_.back(), pred, monotonicity_ != Lit::MONOTONE ? Node::NEG : Node::POS)); }
}

void Builder::visit(Lit *lit, bool domain)
{
	Remember<bool> r1(domain_, domain_ ? true : domain);
	uint32_t monotonicity = Lit::MONOTONE;
	switch(monotonicity_)
	{
		case Lit::MONOTONE:
			monotonicity = lit->monotonicity();
			break;
		case Lit::NONMONOTONE:
			monotonicity = lit->monotonicity() == Lit::ANTIMONOTONE ? Lit::ANTIMONOTONE : Lit::NONMONOTONE;
			break;
		case Lit::ANTIMONOTONE:
			monotonicity = Lit::ANTIMONOTONE;
			break;
	}
	Remember<uint32_t> r2(monotonicity_, monotonicity);
	Remember<bool> r3(head_, head_ ? lit->head() : false);
	lit->visit(this);
}

void Builder::visit(Groundable *grd, bool choice)
{
	Remember<bool> r1(choice_, choice_ ? true : choice);
	grd->visit(this);
}

void Builder::visit(Statement *stm)
{
	choice_ = stm->choice();
	stmNodes_.push_back(new StmNode(stm));
	stm->visit(this);
}

namespace
{

class Tarjan
{
private:
	typedef std::vector<Node*> NodeStack;
	typedef std::vector<bool>  ComponentVec;
public:
	Tarjan();
	void component(Grounder *g, Node *root);
	void start(Grounder *g, Node *n);
private:
	uint32_t     index_;
	ComponentVec components_;
	NodeStack    dfsStack_;
	NodeStack    sccStack_;
};

Tarjan::Tarjan()
	: index_(0)
	, components_(0)
{
}

void Tarjan::component(Grounder *g, Node *root)
{
	g->beginComponent();
	uint32_t component = components_.size();
	components_.push_back(true);
	sccStack_.push_back(root);
	assert(!root->hasComponent());
	root->component(component);
	root->addToComponent(g);
	while(!sccStack_.empty())
	{
		Node *x = sccStack_.back();
		sccStack_.pop_back();
		Node *y;
		for(uint32_t i = 0; (y = x->node(i)); i++)
		{
			if(y->hasComponent() && y->component() < component)
			{
				if(!components_[y->component()])
				{
					components_.back() = false;
					if(x->type(i) == StmNode::DOM)
					{
						std::ostringstream oss;
						x->print(g, oss);
						std::string stmStr = oss.str();
						oss.str("");
						y->print(g, oss);
						throw UnstratifiedException(StrLoc(g, x->loc()), stmStr, StrLoc(g, y->loc()), oss.str());
					}
				}
			}
			else
			{
				assert(y->visited() >= root->visited());
				if(!y->hasComponent())
				{
					y->component(component);
					sccStack_.push_back(y);
					y->addToComponent(g);
				}
				if(x->type(i) == Node::NEG) components_.back() = false;
				if(x->type(i) == Node::DOM)
				{
					std::ostringstream oss;
					x->print(g, oss);
					std::string stmStr = oss.str();
					oss.str("");
					y->print(g, oss);
					throw UnstratifiedException(StrLoc(g, x->loc()), stmStr, StrLoc(g, y->loc()), oss.str());
				}
			}
		}
	}
	g->endComponent(components_.back());
}

void Tarjan::start(Grounder *g, Node *n)
{
	if(n->marked()) return;
	dfsStack_.push_back(n);
	n->mark();
	while(!dfsStack_.empty())
	{
		Node *x = dfsStack_.back();
		if(!x->visited()) x->visited(++index_);
		Node *y = x->next();
		if(y)
		{
			dfsStack_.push_back(y);
			y->mark();
		}
		else
		{
			dfsStack_.pop_back();
			if(x->root()) component(g, x);
		}
	}
}

}

void Builder::analyze(Grounder *g)
{
	bool warned = false;
	g->beginComponent();
	foreach(Todo &todo, todo_)
	{
		bool hasInput = false;
		foreach(PredNode &node, predNodes_[todo.lit->dom()->domId()])
		{
			if(node.pred()->compatible(todo.lit))
			{
				node.provide(todo);
				// TODO: need module check
				todo.stm->depend(&node, todo.type);
				hasInput = true;
			}
		}
		if(!hasInput && !todo.lit->dom()->external())
		{
			if(!todo.lit->dom()->size())
			{
				// TODO: warnings should be communicated in a different way!!!
				if(!warned)
				{
					std::cerr << "WARNING: predicate cannot match:" << std::endl;
					warned = true;
				}
				std::cerr << "\t" << StrLoc(g, todo.lit->loc()) << ": ";
				todo.lit->print(g, std::cerr);
				std::cerr << std::endl;
			}
			todo.lit->complete(true);
		}
	}
	g->endComponent(true);
	Tarjan t;
	foreach(StmNode &stm, stmNodes_) t.start(g, &stm);
	foreach(Todo &todo, todo_)
	{
		if(todo.provided < todo.stm->component())
		{
			todo.lit->complete(true);
		}
	}
	foreach(PredNodeVec &preds, predNodes_)
	{
		foreach(PredNode &pred, preds)
		{
			foreach(Todo *todo, pred.provide())
			{
				if(!todo->lit->complete())
				{
					pred.pred()->provide(todo->lit);
				}
			}
		}
	}
}

void Builder::toDot(Grounder *g, std::ostream &out)
{
	std::map<StmNode*,  int>  stmMap;
	std::map<PredNode*, int> predMap;

	out << "digraph {\n";
	foreach(StmNode &stm, stmNodes_)
	{
		int id = stmMap.size() + predMap.size();
		stmMap[&stm] = id;
		out << id << "[label=\"";
		stm.print(g, out);
		out << "\", shape=box]\n";
	}
	foreach(PredNodeVec &preds, predNodes_)
	{
		foreach(PredNode &pred, preds)
		{
			if(!pred.isNull())
			{
				int id = stmMap.size() + predMap.size();
				predMap[&pred] = id;
				out << id << "[label=\"";
				pred.print(g, out);
				out << "\", shape=ellipse]\n";
			}
		}
	}
	foreach(StmNode &stm, stmNodes_)
	{
		typedef std::pair<PredNode*, Node::Type> Pred;
		foreach(Pred pred, stm.depend_)
		{
			out << predMap[pred.first] << " -> " << stmMap[&stm];
			if(pred.second == Node::NEG) { out << "[style=\"dashed\"]"; }
			out << ";\n";
		}
	}
	foreach(PredNodeVec &preds, predNodes_)
	{
		foreach(PredNode &pred, preds)
		{
			foreach(StmNode *stm, pred.depend_)
			{
				out << stmMap[stm] << " -> " << predMap[&pred] << ";\n";
			}
		}
	}
	out << "}\n";
}

Builder::~Builder()
{
}

}

