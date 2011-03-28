#include "gringo/valvecset.h"

namespace
{
	ValVecSet::Index g_invalid;
}

ValVecSet::Cmp::Cmp(ValVecSet *argSet) :
	set(argSet)
{
}

size_t ValVecSet::Cmp::operator()(const Index &i) const
{
	return boost::hash_range(set->vals_.begin() + i, set->vals_.begin() + i + set->arity_);
}

bool ValVecSet::Cmp::operator()(const Index &a, const Index &b) const
{
	return std::equal(set->vals_.begin() + a, set->vals_.begin() + a + set->arity_, set->vals_.begin() + b);
}

ValVecSet::Index::Index()
	: index(std::numeric_limits<uint32_t>::max())
	, fact(false)
{
}

ValVecSet::Index::Index(uint32_t index, bool fact)
	: index(index)
	, fact(fact)
{
}

bool ValVecSet::Index::valid() const
{
	return *this != g_invalid;
}

ValVecSet::ValVecSet(uint32_t arity)
	: arity_(arity)
	, valSet_(0, Cmp(this), Cmp(this))
{
}

ValVecSet::ValVecSet(const ValVecSet &set)
	: arity_(set.arity_)
	, valSet_(0, Cmp(this), Cmp(this))
{
	extend(set);
}

ValVecSet &ValVecSet::operator=(const ValVecSet &set)
{
	arity_  = set.arity_;
	valSet_ = ValSet(0, Cmp(this), Cmp(this));
	extend(set);
	return *this;
}

const ValVecSet::Index &ValVecSet::find(const const_iterator &v) const
{
	Index idx(vals_.size());
	vals_.insert(vals_.end(), v, v + arity_);
	ValSet::iterator i = valSet_.find(idx);
	vals_.resize(idx.index);
	if(i != valSet_.end()) return *i;
	else return g_invalid;
}

ValVecSet::InsertRes ValVecSet::insert(const const_iterator &v, bool fact)
{
	Index idx(vals_.size(), fact);
	vals_.insert(vals_.end(), v, v + arity_);
	std::pair<ValSet::iterator, bool> res = valSet_.insert(idx);
	if(!res.second)
	{
		vals_.resize(idx.index);
		if(fact) res.first->fact = fact;
	}
	return ValVecSet::InsertRes(*res.first, res.second);
}

void ValVecSet::extend(const ValVecSet &other)
{
	foreach(const Index &idx, other.valSet_)
	{
		insert(other.vals_.begin() + idx.index, idx.fact);
	}
}
