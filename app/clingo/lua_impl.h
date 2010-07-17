// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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
#include "clingo/claspoutput.h"
#include "clingo/clingo_app.h"

#if WITH_LUA == 1
extern "C"
{
#	include <lua.h>
#	include <lualib.h>
#	include <lauxlib.h>
}
#else
#	include <lua/lua.h>
#	include <lua/lualib.h>
#	include <lua/lauxlib.h>
#endif

#define MODEL "Model"

namespace
{
	struct DomainIter
	{
		DomainIter(Grounder *g, Clasp::Solver *s, ClaspOutput *o)
			: grounder(g)
			, solver(s)
			, output(o)
			, active(false)
			, started(false)
		{ }

		void reset()
		{
			active  = true;
			started = false;
		}

		void begin(lua_State *L, const char *name, uint32_t arity)
		{
			if(!active) { luaL_error(L, "Model.begin() may only be called in onModel"); }
			dom     = grounder->domain(grounder->index(name), arity);
			const LparseConverter::SymbolMap &map = output->symbolMap(dom->domId());
			start   = map.begin();
			end     = map.end();
			started = false;
		}

		void pushNext(lua_State *L)
		{
			if(!active) { luaL_error(L, "Model.next() may only be called in onModel"); }
			if(start == end) { lua_pushboolean(L, false); }
			else
			{
				if(started) { start++; }
				else        { started = true; }
				lua_pushboolean(L, start != end);
			}
		}

		void pushName(lua_State *L)
		{
			if(!active)  { luaL_error(L, "Model.name() may only be called in onModel"); }
			if(!started) { luaL_error(L, "Model.name() called without prior call to Model.begin()"); }
			lua_pushstring(L, grounder->string(dom->nameId()).c_str());
		}

		void pushArity(lua_State *L)
		{
			if(!active)  { luaL_error(L, "Model.arity() may only be called in onModel"); }
			if(!started) { luaL_error(L, "Model.arity() called without prior call to Model.begin()"); }
			lua_pushinteger(L, dom->arity());
		}

		void pushArgs(lua_State *L)
		{
			if(!active)      { luaL_error(L, "Model.args() may only be called in onModel"); }
			if(!started)     { luaL_error(L, "Model.args() called without prior call to Model.begin()"); }
			if(start == end) { luaL_error(L, "Model.args() called after Model.next() returned false"); }
			ValRng rng = output->vals(dom, start->first);
			lua_createtable(L, rng.size(), 0);
			int i = 1;
			foreach(const Val &val, rng)
			{
				lua_pushinteger(L, i++);
				grounder->luaPushVal(val);
				lua_rawset(L, -3);
			}
		}

		void pushTruth(lua_State *L)
		{
			if(!active)      { luaL_error(L, "Model.truth() may only be called in onModel"); }
			if(!started)     { luaL_error(L, "Model.truth() called without prior call to Model.begin()"); }
			if(start == end) { luaL_error(L, "Model.truth() called after Model.next() returned false"); }
			Clasp::Atom *atom = solver->strategies().symTab->find(start->second);
			lua_pushboolean(L, atom && solver->isTrue(atom->lit));
		}

		typedef LparseConverter::SymbolMap::const_iterator SymIt;

		Grounder      *grounder;
		Clasp::Solver *solver;
		ClaspOutput   *output;
		bool           active;
		Domain        *dom;
		bool           started;
		SymIt          start;
		SymIt          end;
	};

	static DomainIter* checkDomainIter(lua_State *L)
	{
		lua_pushliteral(L, "Model.domainIter");
		lua_rawget(L, LUA_REGISTRYINDEX);
		DomainIter *domIter = (DomainIter *)lua_touserdata(L, -1);
		lua_pop(L, 1); // pop storage
		return domIter;
	}

	static int Model_begin(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		luaL_checkstring(L, 1);
		luaL_checkinteger(L, 2);
		const char *name = lua_tostring(L, 1);
		int arity  = lua_tointeger(L, 2);
		domIter->begin(L, name, arity);
		return 0;
	}

	static int Model_next(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushNext(L);
		return 1;
	}

	static int Model_args(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushArgs(L);
		return 1;
	}

	static int Model_truth(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushTruth(L);;
		return 1;
	}

	static int Model_name(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushName(L);
		return 1;
	}

	static int Model_arity(lua_State *L)
	{
		DomainIter *domIter = checkDomainIter(L);
		domIter->pushArity(L);
		return 1;
	}

	static const luaL_reg Model_methods[] =
	{
		{"begin", Model_begin},
		{"next",  Model_next},
		{"args",  Model_args},
		{"truth", Model_truth},
		{"args",  Model_args},
		{"name",  Model_name},
		{"arity", Model_arity},
		{0, 0}
	};

	static const luaL_reg Model_meta[] =
	{
		{0, 0}
	};

	int Model_register (lua_State *L, DomainIter *iter)
	{
		luaL_openlib(L, MODEL, Model_methods, 0);

		luaL_newmetatable(L, MODEL);

		luaL_openlib(L, 0, Model_meta, 0);

		lua_pushliteral(L, "__index");
		lua_pushvalue(L, -3);
		lua_rawset(L, -3);

		lua_pushliteral(L, "__metatable");
		lua_pushvalue(L, -3);
		lua_rawset(L, -3);

		lua_pop(L, 2); // pop metatable and meta methods

		lua_pushliteral(L, "Model.domainIter");
		lua_pushlightuserdata(L, iter);
		lua_rawset(L, LUA_REGISTRYINDEX);

		return 0;
	}

}

template <bool ICLINGO>
class ClingoApp<ICLINGO>::LuaImpl
{
public:
	LuaImpl(Grounder *g, Clasp::Solver *s, ClaspOutput *o)
		: domIter_(g, s, o)
	{
		lua_State *L = g->luaState();
		if(L)
		{
			lua_getglobal(L, "onModel");
			onModel_ = lua_gettop(L);
			if(!lua_isfunction(L, onModel_)) { onModel_ = -1; }
			Model_register(L, &domIter_);
		}
	}
	void onModel()
	{
		if(onModel_ != -1)
		{
			lua_State *L = domIter_.grounder->luaState();
			if(L)
			{
				domIter_.reset();
				lua_pushvalue(L, onModel_);
				lua_call(L, 0, 0);
				domIter_.active = false;
			}
		}
	}
	bool locked()
	{
		return onModel_ != -1;
	}
private:
	DomainIter        domIter_;
	int               onModel_;
};
