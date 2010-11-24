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

#include <gringo/context.h>

Context::Context()
{
	contextStack_.push_back(ContextVec::value_type());
}

void Context::reserve(uint32_t vars)
{
	if(contextStack_.back().first.size() < vars)
	{
		contextStack_.back().first.resize(vars, -1);
		contextStack_.back().second.resize(vars);
	}
}

const Val &Context::val(uint32_t index) const
{
	assert(contextStack_.back().first[index] != -1);
	return contextStack_.back().second[index];
}

void Context::val(uint32_t index, const Val &v, int binder)
{
	contextStack_.back().first[index] = binder;
	contextStack_.back().second[index] = v;
}

int Context::binder(uint32_t index) const
{
	assert(index < contextStack_.back().first.size());
	return contextStack_.back().first[index];
}

void Context::unbind(uint32_t index)
{
	contextStack_.back().first[index] = -1;
}

void Context::pushContext()
{
	contextStack_.push_back(ContextVec::value_type(contextStack_.back().first, contextStack_.back().second));
	for(size_t i = 0; i < contextStack_.back().first.size(); i++)
		this->unbind(i);
}

void Context::popContext()
{
	assert(contextStack_.size() > 0);
	contextStack_.pop_back();
}

