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
#include <gringo/storage.h>
#include <gringo/context.h>
#include <gringo/locateable.h>
#include <gringo/stats.h>

typedef struct lua_State lua_State;

class Module
{
	friend class Grounder;
private:
	struct Component
	{
		typedef std::vector<Statement*> StatementVec;
		Component() { }
		StatementVec statements;
	};
	typedef std::vector<Component> ComponentVec;
	typedef std::vector<Module*> ModuleVec;
public:
	StatementRng add(Grounder *g, Statement *s, bool optimizeEdb = true);
	void beginComponent();
	void addToComponent(Grounder *g, Statement *stm);
	void endComponent();
	bool compatible(Module *module);
	void parent(Module *module);
	StatementPtrVec &statements();
	~Module();
private:
	bool reachable(Module *module);
private:
	ModuleVec        parent_;
	ComponentVec     components_;
	StatementPtrVec  statements_;
};

class Grounder : public Storage, public Context
{
	friend class Module;
private:
	class LuaImpl;
	typedef std::deque<Groundable*> GroundableQueue;
	typedef boost::ptr_vector<Module> ModuleVec;

public:
	Grounder(Output *out, bool debug, BodyOrderHeuristicPtr heuristic);
	void analyze(const std::string &depGraph = "", bool stats = false);
	void addMagic();
	void ground(Module &module);
	void enqueue(Groundable *g);
	void externalStm(uint32_t nameId, uint32_t arity);
	uint32_t createVar();
	BodyOrderHeuristic& heuristic() const;
	Module *createModule();
	void addInternal(Statement *stm);
	uint32_t aggrUid();
	void addForgetTerm(Term *forget);
	void groundForget(int step);
	
	void luaExec(const Loc &loc, const std::string &s);
	void luaCall(const LuaLit *lit, const ValVec &args, ValVec &vals);
	int luaIndex(const Loc& loc, uint32_t name);
	lua_State *luaState();
	void luaPushVal(const Val &val);
	
	~Grounder();
private:
	void ground_();
	void setModule(Module *module, bool optimizeEdb);

private:
	ModuleVec              modules_;
	GroundableQueue        queue_;
	uint32_t               internal_;
	uint32_t               aggrUids_;
	bool                   debug_;
	std::auto_ptr<LuaImpl> luaImpl_;
	Stats                  stats_;
	BodyOrderHeuristicPtr  heuristic_;
	Module                *current_;
	bool                   optimizeEdb_;
	std::vector<Term*>     forgetTerms_;
};

// ========================== Module ==========================

inline bool Module::compatible(Module *module) { return module->reachable(this); }
inline StatementPtrVec &Module::statements() { return statements_; }

// ========================== Grounder ==========================

inline uint32_t Grounder::aggrUid() { return aggrUids_++; }
