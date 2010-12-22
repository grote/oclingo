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

struct TermExpansion
{
	virtual bool limit(Grounder *g, const ValRng &rng, int32_t &) const;
	virtual void expand(Grounder *g);
	virtual ~TermExpansion();
};
typedef std::auto_ptr<TermExpansion> TermExpansionPtr;

struct TermDepthExpansion : public TermExpansion
{
	TermDepthExpansion(IncConfig &config);
	virtual bool limit(Grounder *g, const ValRng &rng, int32_t &offset) const;
	virtual void expand(Grounder *g);
	virtual ~TermDepthExpansion();

	IncConfig &config;
	int        start;
};

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
	typedef std::deque<Groundable*> GroundableVec;
	typedef boost::ptr_vector<Module> ModuleVec;

public:
	Grounder(Output *out, bool debug, TermExpansionPtr exp, BodyOrderHeuristicPtr heuristic);
	void analyze(const std::string &depGraph = "", bool stats = false);
	void ground(Module &module);
	void enqueue(Groundable *g);
	void externalStm(uint32_t nameId, uint32_t arity);
	uint32_t createVar();
	TermExpansion &termExpansion() const;
	const BodyOrderHeuristic& heuristic() const;
	Module *createModule();
	void addInternal(Statement *stm);
	
	void luaExec(const Loc &loc, const std::string &s);
	void luaCall(const LuaLit *lit, const ValVec &args, ValVec &vals);
	int luaIndex(const LuaTerm *term);
	lua_State *luaState();
	void luaPushVal(const Val &val);
	
	~Grounder();
private:
	void ground_();
	void setModule(Module *module, bool optimizeEdb);

private:
	ModuleVec              modules_;
	GroundableVec          queue_;
	uint32_t               internal_;
	bool                   debug_;
	std::auto_ptr<LuaImpl> luaImpl_;
	Stats                  stats_;
	TermExpansionPtr       termExpansion_;
	BodyOrderHeuristicPtr  heuristic_;
	Module                *current_;
	bool                   optimizeEdb_;
};

// ========================== TermExpansion ==========================

inline bool TermExpansion::limit(Grounder *, const ValRng &, int32_t &) const { return false; }
inline void TermExpansion::expand(Grounder *) { }
inline TermExpansion::~TermExpansion() { }

// ========================== Module ==========================

inline bool Module::compatible(Module *module) { return module->reachable(this); }
inline StatementPtrVec &Module::statements() { return statements_; }

// ========================== Grounder ==========================

inline TermExpansion &Grounder::termExpansion() const { return *termExpansion_; }
