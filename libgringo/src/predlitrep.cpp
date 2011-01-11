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

#include <gringo/predlitrep.h>
#include <gringo/domain.h>

PredLitSet::PredCmp::PredCmp(const ValVec &vals)
	: vals_(vals)
{
}

size_t PredLitSet::PredCmp::operator()(const PredSig &a) const
{
	size_t hash = a.first->dom()->domId();
	boost::hash_combine(hash, static_cast<size_t>(a.first->sign()));
	boost::hash_range(hash, vals_.begin() + a.second, vals_.begin() + a.second + a.first->dom()->arity() + 1);
	return hash;
}

bool PredLitSet::PredCmp::operator()(const PredSig &a, const PredSig &b) const
{
	return
		a.first->dom() == b.first->dom() &&
		a.first->sign() == b.first->sign() &&
		ValRng(vals_.begin() + a.second, vals_.begin() + a.second + a.first->dom()->arity() + 1) ==
		ValRng(vals_.begin() + b.second, vals_.begin() + b.second + a.first->dom()->arity() + 1);
}

PredLitSet::PredLitSet()
	: set_(0, PredCmp(vals_), PredCmp(vals_))
{
}

bool PredLitSet::insert(PredLitRep *pred, size_t pos, Val &val)
{
	ValRng rng = pred->vals(pos);
	size_t offset = vals_.size();
	vals_.insert(vals_.end(), rng.begin(), rng.end());
	vals_.insert(vals_.end(), val);
	if(set_.insert(PredSig(pred, offset)).second) return true;
	else
	{
		vals_.resize(offset);
		return false;
	}
}

void PredLitSet::clear()
{
	set_.clear();
	vals_.clear();
}

PredLitRep::PredLitRep(bool sign, Domain *dom)
	: sign_(sign)
	, match_(true)
	, top_(0)
	, dom_(dom)
{
}

bool PredLitRep::addDomain(Grounder *g, bool fact)
{
	assert(top_ + dom_->arity() <= vals_.size());
	return dom_->insert(g, vals_.begin() + top_, fact);
}

ValRng PredLitRep::vals() const
{
	return ValRng(vals_.begin() + top_, vals_.begin() + top_ + dom_->arity());
}

ValRng PredLitRep::vals(uint32_t top) const
{
	return ValRng(vals_.begin() + top, vals_.begin() + top + dom_->arity());
}

void PredLitRep::push()
{
	top_ = vals_.size();
}

bool PredLitRep::testUnique(PredLitSet &set, Val val)
{
	return set.insert(this, top_, val);
}

void PredLitRep::pop()
{
	top_-= dom_->arity();
}

void PredLitRep::move(size_t p)
{
	top_ = p * dom_->arity();
}

void PredLitRep::clear()
{
	top_ = 0;
	vals_.clear();
}
