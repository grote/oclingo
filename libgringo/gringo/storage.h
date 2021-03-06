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
#include <gringo/func.h>

class Storage
{
public:
	typedef std::vector<Domain*> DomainVec;
private:
	typedef boost::multi_index::multi_index_container
	<
		std::string, boost::multi_index::indexed_by
		<
			boost::multi_index::random_access<>,
			boost::multi_index::hashed_unique<boost::multi_index::identity<std::string> >
		>
	> StringSet;
	typedef boost::multi_index::multi_index_container
	<
		Func, boost::multi_index::indexed_by
		<
			boost::multi_index::random_access<>,
			boost::multi_index::hashed_unique<boost::multi_index::identity<Func> >
		>
	> FuncSet;
public:
	std::string quote(const std::string &str) const;
	std::string unquote(const std::string &str) const;
	Storage(Output *output);
	uint32_t index(const Func &f);
	const Func &func(uint32_t i) const;
	uint32_t index(const std::string &s);
	const std::string &string(uint32_t i) const;
	Domain *domain(uint32_t domId);
	Domain const *domain(uint32_t domId) const;
	Domain *newDomain(uint32_t nameId, uint32_t arity);
	DomainVec &domains() { return domains_; }
	DomainVec const &domains() const { return domains_; }
	Output *output() const { return output_; }
	~Storage();
private:
	StringSet strings_;
	FuncSet   funcs_;
	DomainMap doms_;
	DomainVec domains_;
	Output   *output_;
};
