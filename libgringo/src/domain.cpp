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

#include <gringo/domain.h>
#include <gringo/val.h>
#include <gringo/predindex.h>
#include <gringo/grounder.h>
#include <gringo/groundable.h>
#include <gringo/instantiator.h>
#include <gringo/grounder.h>

Domain::TupleCmp::TupleCmp(Domain *d) :
	dom(d)
{
}

size_t Domain::TupleCmp::operator()(const Index &i) const
{
	return boost::hash_range(dom->vals_.begin() + i, dom->vals_.begin() + i + dom->arity_);
}

bool Domain::TupleCmp::operator()(const Index &a, const Index &b) const
{
	return std::equal(dom->vals_.begin() + a, dom->vals_.begin() + a + dom->arity_, dom->vals_.begin() + b);
}

namespace
{
	const Domain::Index invalid(std::numeric_limits<uint32_t>::max(), 0);
}

Domain::Index::Index(uint32_t index, bool fact) :
	index(index),
	fact(fact)
{
}

bool Domain::Index::valid() const
{
	return *this != invalid;
}

Domain::Domain(uint32_t nameId, uint32_t arity, uint32_t domId)
	: nameId_(nameId)
	, arity_(arity)
	, domId_(domId)
	, valSet_(0, TupleCmp(this), TupleCmp(this))
	, new_(0)
	, complete_(false)
        , external_(false)
{
}

const Domain::Index &Domain::find(const ValVec::const_iterator &v) const
{
	Index idx(vals_.size());
	vals_.insert(vals_.end(), v, v + arity_);
	ValSet::iterator i = valSet_.find(idx);
	vals_.resize(idx.index);
	if(i != valSet_.end()) return *i;
	else return invalid;
}

void Domain::insert(const ValVec::const_iterator &v, bool fact)
{
	Index idx(vals_.size(), fact);
	vals_.insert(vals_.end(), v, v + arity_);
	std::pair<ValSet::iterator, bool> res = valSet_.insert(idx);
	if(!res.second) 
	{
		vals_.resize(idx.index);
		if(fact) res.first->fact = fact;
	}
}

void Domain::enqueue(Grounder *g)
{
	uint32_t end = valSet_.size();
	foreach(const PredInfo &info, index_)
	{
		bool modified = false;
		ValVec::const_iterator k = vals_.begin() + arity_ * new_;
		for(ValSet::size_type i = new_; i < valSet_.size(); ++i, k+= arity_)
			modified = info.first->extend(g, k) || modified;
		if(modified) g->enqueue(info.second);
	}
	foreach(PredIndex *idx, completeIndex_)
	{
		ValVec::const_iterator k = vals_.begin() + arity_ * new_;
		for(ValSet::size_type i = new_; i < valSet_.size(); ++i, k+= arity_)
			idx->extend(g, k);
	}
	new_ = end;
}

void Domain::append(Grounder *g, Groundable *gr, PredIndex *idx)
{
	ValVec::const_iterator j = vals_.begin();
	for(ValSet::size_type i = 0; i < new_; ++i, j+= arity_) idx->extend(g, j);
	assert(!complete_ || new_ == valSet_.size());
	if(!complete_) { index_.push_back(PredInfo(idx, gr)); }
	else { completeIndex_.push_back(idx); }
}

