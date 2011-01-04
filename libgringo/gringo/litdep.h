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

#pragma once

#include <gringo/gringo.h>
#include <gringo/prgvisitor.h>

namespace LitDep
{
	class VarNode;
	class LitNode;

	struct LitNodeCmp
	{
		bool operator()(LitNode *a, LitNode *b);
	};

	typedef std::vector<LitNode*> LitNodeVec;

	class GrdNode
	{
	private:
		typedef boost::ptr_vector<LitNode> LitNodePtrVec;
		typedef boost::ptr_vector<VarNode> VarNodeVec;
	public:
		GrdNode(Groundable *groundable);
		void append(LitNode *litNode);
		void append(VarNode *varNode);
		void reset();
		bool check(VarTermVec &terms);
		void order(Grounder *g, const VarSet &bound);
		~GrdNode();
	private:
		Groundable   *groundable_;
		LitNodePtrVec litNodes_;
		VarNodeVec    varNodes_;
	};

	class Builder : public PrgVisitor
	{
	private:
		typedef std::vector<GrdNode*> GrdNodeVec;
		typedef std::vector<uint32_t> GrdNodeStack;
		typedef std::vector<LitNode*> LitNodeStack;
		typedef std::vector<VarNode*> VarNodeVec;
	public:
		Builder(uint32_t vars);
		void visit(VarTerm *var, bool bind);
		void visit(Term* term, bool bind);
		void visit(Lit *lit, bool domain);
		void visit(Groundable *groundable, bool choice);
		bool check(VarTermVec &terms);
		~Builder();
	private:
		LitNodeStack litStack_;
		GrdNodeVec   grdNodes_;
		GrdNodeStack grdStack_;
		VarNodeVec   varNodes_;
	};
}

inline LitDep::GrdNode* new_clone(const LitDep::GrdNode& a)
{
	(void)a;
	return 0;
}

