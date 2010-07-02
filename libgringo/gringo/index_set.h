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

#include <vector>
#include <boost/functional/hash.hpp>
#include <boost/unordered/unordered_set.hpp>

template <typename T>
class index_set
{
private:
	struct cmp;
	typedef std::vector<T> val_vec;
	typedef boost::unordered_set<uint32_t, cmp, cmp> idx_set;
	struct cmp
	{
		cmp(const index_set<T> &set);
		const T &val(uint32_t i) const;
		size_t operator()(uint32_t i) const;
		bool operator()(uint32_t a, uint32_t b) const;
		const index_set<T> &set_;
	};
public:
	index_set();
	uint32_t insert(const T &val);
	const T &get(uint32_t idx) const;
private:
	const T *lastVal_;
	val_vec  vals_;
	idx_set  index_;
};

template <typename T>
inline index_set<T>::index_set()
	: index_(42, cmp(*this), cmp(*this))
{
}

template <typename T>
inline index_set<T>::cmp::cmp(const index_set<T> &set)
	: set_(set)
{
}

template <typename T>
inline const T &index_set<T>::cmp::val(uint32_t i) const
{
	return i < set_.vals_.size() ? set_.vals_[i] : *set_.lastVal_;
}

template <typename T>
inline size_t index_set<T>::cmp::operator()(uint32_t i) const
{
	return boost::hash_value(val(i));
}

template <typename T>
inline bool index_set<T>::cmp::operator()(uint32_t a, uint32_t b) const
{
	return val(a) == val(b);
}

template <typename T>
uint32_t index_set<T>::insert(const T &val)
{
	lastVal_ = &val;
	std::pair<typename idx_set::iterator, bool> res = index_.insert(vals_.size());
	if(res.second) vals_.push_back(val);
	return *res.first;
}

template <typename T>
const T &index_set<T>::get(uint32_t idx) const
{
	return vals_[idx];
}

