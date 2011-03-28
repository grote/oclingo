#pragma once

#include <gringo/gringo.h>

class ValVecSet
{
public:
	struct Index
	{
		Index();
		Index(uint32_t index, bool fact = false);
		uint32_t index : 31;
		mutable uint32_t fact : 1;
		bool valid() const;
		operator uint32_t() const { return index; }
	};
	typedef ValVec::iterator       iterator;
	typedef ValVec::const_iterator const_iterator;
	typedef boost::tuples::tuple<const Index&, bool> InsertRes;

private:
	struct Cmp
	{
		Cmp(ValVecSet *set);
		size_t operator()(const Index &i) const;
		bool operator()(const Index &a, const Index &b) const;
		ValVecSet *set;
	};
	typedef boost::unordered_set<Index, Cmp, Cmp> ValSet;

public:
	iterator       begin()       { return vals_.begin(); }
	const_iterator begin() const { return vals_.begin(); }
	iterator       end()         { return vals_.end(); }
	const_iterator end() const   { return vals_.end(); }
	uint32_t       size() const  { return valSet_.size(); }

	ValVecSet(uint32_t arity);
	ValVecSet(const ValVecSet &set);
	ValVecSet &operator=(const ValVecSet &set);
	const Index &find(const const_iterator &v) const;
	InsertRes insert(const const_iterator &v, bool fact = false);
	void extend(const ValVecSet &other);

private:
	uint32_t       arity_;
	mutable ValVec vals_;
	ValSet         valSet_;
};
