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

#include <gringo/storage.h>
#include <gringo/domain.h>
#include <gringo/output.h>
#include <gringo/func.h>

Storage::Storage(Output *output) 
	: output_(output)
{
	output_->storage(this);
}

uint32_t Storage::index(const Func &f)
{
	FuncSet::iterator it = funcs_.push_back(f).first;
	return it - funcs_.begin();
}

const Func &Storage::func(uint32_t i)
{
	return funcs_.at(i);
}

uint32_t Storage::index(const std::string &s)
{
	StringSet::iterator it = strings_.push_back(s).first;
	return it - strings_.begin();
}

const std::string &Storage::string(uint32_t i)
{
	return strings_.at(i);
}

Domain *Storage::domain(uint32_t nameId, uint32_t arity)
{
	Signature sig(nameId, arity);
	DomainMap::iterator i = doms_.find(sig);
	if(i == doms_.end())
	{
		Domain *dom = new Domain(nameId, arity, doms_.size());
		output_->addDomain(dom);
		return doms_.insert(sig, dom).first->second;
	}
	else return i->second;
}

Storage::~Storage()
{
}

