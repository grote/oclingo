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

class Groundable
{
public:
	Groundable();
	virtual bool grounded(Grounder *g) = 0;
	virtual void ground(Grounder *g) = 0;
	virtual void visit(PrgVisitor *visitor) = 0;
	virtual void doEnqueue(bool enqueue);
	VarVec &vars();
	bool enqueued() const;
	void enqueued(bool e);
	void instantiator(Instantiator *inst);
	Instantiator *instantiator() const;
	uint32_t level() const;
	void level(uint32_t level);
	void litDep(LitDep::GrdNode *litDep);
	LitDep::GrdNode *litDep();
protected:
	~Groundable();
protected:
	bool                       enqueued_;
	uint32_t                   level_;
	clone_ptr<Instantiator>    inst_;
	clone_ptr<LitDep::GrdNode> litDep_;
	VarVec                     vars_;
};

///////////////////////////////////// Groundable /////////////////////////////////////

inline void Groundable::doEnqueue(bool enqueue) { }
inline VarVec &Groundable::vars() { return vars_; }
inline bool Groundable::enqueued() const { return enqueued_; }
inline void Groundable::enqueued(bool e)
{
	if(enqueued_ != e) { doEnqueue(e); }
	enqueued_ = e;
}
inline uint32_t Groundable::level() const { return level_; }
inline void Groundable::level(uint32_t level) { level_ = level; }
inline LitDep::GrdNode *Groundable::litDep() { return litDep_.get(); }
