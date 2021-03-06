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
#include <gringo/valvecset.h>

class Domain
{
public:
	typedef std::pair<PredIndex*,Formula*> PredInfo;
	typedef std::vector<PredInfo> PredInfoVec;
	typedef std::vector<PredIndex*> PredIndexVec;
	typedef std::vector<Formula*> GroundableVec;
public:
	Domain(uint32_t nameId, uint32_t arity, uint32_t domId);
	const ValVecSet::Index &find(const ValVec::const_iterator &v) const;
	bool insert(Grounder *g, const ValVec::const_iterator &v, bool fact = false);
	uint32_t size() const   { return vals_.size(); }
	void external(bool e)   { external_ = e; }
	bool external() const   { return external_; }
	uint32_t arity() const  { return arity_; }
	uint32_t nameId() const { return nameId_; }
	uint32_t domId() const  { return domId_; }
	bool extend(Grounder *g, PredIndex *idx, uint32_t offset);
	//! creates a map of all possible values for every variable in the literal
	void allVals(Grounder *g, const TermPtrVec &terms, VarDomains &varDoms);
private:
	uint32_t       nameId_;
	uint32_t       arity_;
	uint32_t       domId_;
	ValVecSet      vals_;
	PredInfoVec    index_;
	PredIndexVec   completeIndex_;
	uint32_t       new_;
	bool           external_;
public:
	bool           show;
	bool           hide;
};

