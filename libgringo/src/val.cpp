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

#include <gringo/val.h>
#include <gringo/storage.h>
#include <gringo/func.h>
#include <gringo/exceptions.h>

namespace
{

uint32_t invertId(const Val *v, uint32_t id, Storage *s);

}

///////////////////////////// Val /////////////////////////////

void Val::print(Storage *sto, std::ostream &out) const
{
	switch(type)
	{
		case ID:     { out << sto->string(index); break; }
		case STRING: { out << '"' << sto->quote(sto->string(index)) << '"'; break; }
		case NUM:    { out << num; break; }
		case FUNC:   { sto->func(index).print(sto, out); break; }
		case INF:    { out << "#infimum"; break; }
		case SUP:    { out << "#supremum"; break; }
		case UNDEF:  { assert(false); break; }
		default:     { return sto->print(out, type, index); }
	}
}

int Val::compare(const Val &v, Storage *s) const
{
	if(type != v.type) return type < v.type ? -1 : 1;
	switch(type)
	{
		case ID:
		case STRING: { return s->string(index).compare(s->string(v.index)); }
		case NUM:    { return num - v.num; }
		case FUNC:   { return s->func(index).compare(s->func(v.index), s); }
		case INF:
		case SUP:    { return 0; }
		case UNDEF:  { assert(false); break; }
	}
	return s->compare(type, index, v.index);
}

int Val::compare(const int64_t &v, Storage *) const
{
	if(type != NUM) return type < NUM ? -1 : 1;
	if(num < v) { return -1; }
	if(num > v) { return 1; }
	else        { return 0; }
}

int Val::compare(Storage *s, const ValVec &a, const ValVec &b)
{
	if(a.size() < b.size())      { return -1; }
	else if(a.size() > b.size()) { return 1; }
	else
	{
		for(ValVec::const_iterator i = a.begin(), j = b.begin(); i != a.end(); i++, j++)
		{
			int cmp = i->compare(*j, s);
			if(cmp != 0) { return cmp; }
		}
		return 0;
	}
}

Val Val::invert(Storage *s) const
{
	switch(type)
	{
		case ID:     { return Val::create(Val::ID, invertId(this, index, s)); }
		case STRING: { throw this; }
		case NUM:    { return Val::create(Val::NUM, -num); }
		case FUNC:
		{
			const Func &f = s->func(index);
			return Val::create(Val::FUNC, s->index(Func(s, invertId(this, f.name(), s), f.args())));
		}
		case INF:    { return Val::sup(); }
		case SUP:    { return Val::inf(); }
		case UNDEF:  { assert(false); break; }
	}
	assert(false);
	return *this;
}

///////////////////////////// anonymous /////////////////////////////

namespace
{

uint32_t invertId(const Val *v, uint32_t id, Storage *s)
{
	const std::string &str = s->string(id);
	if(str.empty())        { throw v; }
	else if(str[0] == '-') { return s->index(str.substr(1)); }
	else                   { return s->index(std::string("-") + str); }
}

}
