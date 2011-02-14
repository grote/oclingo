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
#include <gringo/lit.h>
#include <gringo/printer.h>
#include <gringo/groundable.h>
#include <gringo/valvecset.h>
#include <gringo/index.h>

class AggrLit;

class AggrState
{
private:
	typedef boost::unordered_map<ValVec,bool> ValVecSet;
	typedef std::map<Lit*, ValVec> Substitution;
public:
	AggrState();

	void accumulate(Grounder *g, Lit *head, const VarVec &headVars, AggrLit &lit, const ValVec &set, bool fact);
	ValVec &substitution(Lit *lit);

	virtual void doAccumulate(Grounder *g, AggrLit &lit, const ValVec &set, bool isNew, bool newFact) = 0;

	virtual bool match() const = 0;
	virtual void finish() = 0;

	virtual bool fact() const = 0;
	virtual bool isNew() const = 0;
	virtual void lock() = 0;

	virtual bool bindFirst(Val *val) = 0;
	virtual bool bindNext(Val *val) = 0;

	virtual ~AggrState();

protected:
	ValVecSet    sets_;
	Substitution subst_;
};

class BoundAggrState : public AggrState
{
public:
	BoundAggrState();

	bool match() const;
	void finish();

	bool fact() const;
	bool isNew() const;
	void lock();

	bool bindFirst(Val *val);
	bool bindNext(Val *val);

	virtual ~BoundAggrState();

protected:
	template<class T>
	void checkBounds(AggrLit &lit, const T &min, const T &max, const T &lower, const T &upper);
	template<class T, class P>
	void checkBounds(AggrLit &lit, const T &min, const T &max, const T &lower, const T &upper, const P &lt);

private:
	bool locked_;
	bool new_;
	// NOTE: have to be set in accumulate
	bool match_;
	bool fact_;
};

class AssignAggrState : public AggrState
{
public:
	struct Assign
	{
		struct Less
		{
			Less(Storage *s);
			bool operator()(const Assign &a, const Assign &b);

			Storage *storage;
		};

		Assign(const Val &val);
		bool operator==(const Assign &a);

		Val  val;
		bool isNew;
		bool locked;
	};
	typedef std::vector<Assign> AssignVec;

public:
	AssignAggrState();

	bool match() const;
	void finish();

	bool fact() const;
	bool isNew() const;
	void lock();

	bool bindFirst(Val *val);
	bool bindNext(Val *val);

	virtual ~AssignAggrState();

protected:
	AssignVec           assign_;
	AssignVec::iterator current_;
	bool                fact_;
};

AggrState* new_clone(const AggrState& state);

class AggrDomain
{
	typedef boost::ptr_vector<AggrState> AggrStateVec;
public:
	AggrDomain();
	void init(const VarSet &global);
	AggrState *state(Grounder *g, AggrLit *lit);
	void finish();
	void markNew();
	bool hasNew() const;
	AggrState *last() const;

private:
	AggrStateVec states_; // set of aggrstates
	ValVecSet    domain_; // vals -> states
	VarVec       global_; // global variables in the aggregate
	AggrState   *last_;
	bool         new_;    // wheather there is (possibly) a new match
};

class AggrLit : public Lit
{
private:
	typedef boost::ptr_vector<AggrState> AggrStates;
public:
	AggrLit(const Loc &loc, CondLitVec &conds);

	virtual AggrState *newAggrState(Grounder *g) = 0;

	void lower(Term *l);
	void upper(Term *u);
	void assign(Term *a);
	Term *assign() const;
	Term *lower() const;
	Term *upper() const;
	void sign(bool s);
	bool sign() const;

	void add(CondLit *cond);
	CondLitVec &conds();
	AggrDomain &domain();
	void enqueue(Grounder *g);
	void ground(Grounder *g);

	~AggrLit();

	// Lit interface
	virtual void normalize(Grounder *g, Expander *expander);

	void doHead(bool head);

	bool matchLast() const;
	bool match(Grounder *grounder);
	bool fact() const;

	bool complete() const;

	virtual Monotonicity monotonicity() const;

	void grounded(Grounder *grounder);
	void addDomain(Grounder *g, bool fact);
	void finish(Grounder *g);

	void index(Grounder *g, Groundable *gr, VarSet &bound);
	Score score(Grounder *g, VarSet &bound);

	void visit(PrgVisitor *visitor);

protected:
	bool            sign_;
	bool            assign_;
	bool            fact_;
	mutable tribool complete_;
	clone_ptr<Term> lower_;
	clone_ptr<Term> upper_;
	Groundable     *parent_;
	CondLitVec      conds_;
	AggrDomain      domain_;
	Val             valLower_;
	Val             valUpper_;
};

class SetLit : public Lit
{
public:
	SetLit(const Loc &loc, TermPtrVec &terms);

	TermPtrVec *terms();

	bool match(Grounder *g);
	bool fact() const;

	void addDomain(Grounder *g, bool fact);
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	bool edbFact() const;
	void normalize(Grounder *g, Expander *e);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	void accept(Printer *v);
	Lit *clone() const;
	~SetLit();
private:
	TermPtrVec terms_; //! determines the uniqueness + holds the weight
};

SetLit* new_clone(const SetLit& a);

class CondLit : public Groundable, public Locateable
{
	friend class AggrLit;
	using Groundable::finish;
public:
	class Printer : public ::Printer
	{
	public:
		virtual void begin(AggrState *state) = 0;
		virtual void endHead() = 0;
		virtual void set(const ValVec &set) = 0;
		virtual void trueLit() = 0;
		virtual void print(PredLitRep *l) = 0;
		virtual void end() = 0;
		virtual ~Printer();
	};
	enum Style { STYLE_DLV, STYLE_LPARSE, STYLE_LPARSE_SET };
public:
	CondLit(const Loc &loc, TermPtrVec &terms, LitPtrVec &lits, Style style);
	AggrLit *aggr();
	TermPtrVec *terms();
	LitPtrVec *lits();
	Style style();
	bool complete() const;
	void add(Lit *lit);
	void head(bool head);
	void initHead();

	void finish();
	void finish(Grounder *g);
	void enqueue(Grounder *g);
	void ground(Grounder *g);
	bool grounded(Grounder *g);

	void normalize(Grounder *g, uint32_t number);
	void visit(PrgVisitor *visitor);
	void print(Storage *sto, std::ostream &out) const;
	void addDomain(Grounder *g, bool fact);

	~CondLit();
private:
	Style           style_;
	SetLit          set_;
	LitPtrVec       lits_;
	VarVec          headVars_;
	AggrLit        *aggr_;
	mutable tribool complete_;
};

/////////////////////////////// AggrState ///////////////////////////////

inline ValVec &AggrState::substitution(Lit *lit) { return subst_[lit]; }

inline AggrState* new_clone(const AggrState&)
{
	// NOTE: should never be called
	assert(false);
	return 0;
}

/////////////////////////////// BoundAggrState ///////////////////////////////

inline BoundAggrState::BoundAggrState()
	: locked_(false)
	, new_(true)
	, match_(false)
	, fact_(false)
{ }

inline bool BoundAggrState::match() const { return match_; }
inline void BoundAggrState::finish() { new_ = false; }

inline bool BoundAggrState::fact() const { return fact_; }
inline bool BoundAggrState::isNew() const { return new_; }
inline void BoundAggrState::lock() { locked_ = true; }

inline bool BoundAggrState::bindFirst(Val *) { return locked_ || match_; }
inline bool BoundAggrState::bindNext(Val *) { return false; }

template <class T>
void BoundAggrState::checkBounds(AggrLit &lit, const T &min, const T &max, const T &lower, const T &upper)
{
	checkBounds<T, std::less<T> >(lit, min, max, lower, upper, std::less<T>());
}

template <class T, class P>
void BoundAggrState::checkBounds(AggrLit &lit, const T &min, const T &max, const T &lower, const T &upper, const P &lt)
{
	match_ = lt(upper, lower) || lt(min, lower) || lt(upper, max);
	fact_  = lt(upper, lower) || lt(max, lower) || lt(upper, min);
	if(!lit.sign())
	{
		bool match = !fact_;
		fact_      = !match_;
		match_     = match;
	}
}

inline BoundAggrState::~BoundAggrState() { }

/////////////////////////////// AssignAggrState::Assign::Less ///////////////////////////////

inline AssignAggrState::Assign::Less::Less(Storage *s)
 : storage(s)
{ }

inline bool AssignAggrState::Assign::Less::operator()(const Assign &a, const Assign &b)
{
	if(     a.val    != b.val)    { return a.val.compare(b.val, storage) < 0; }
	else if(a.locked != b.locked) { return a.locked && !b.locked; }
	else if(a.isNew  != b.isNew)  { return !a.isNew && b.isNew; }
	return false;
}

/////////////////////////////// AssignAggrState::Assign ///////////////////////////////

inline AssignAggrState::Assign::Assign(const Val &val)
	: val(val)
	, isNew(true)
	, locked(false)
{
}

inline bool AssignAggrState::Assign::operator==(const Assign &a) { return val == a.val; }

/////////////////////////////// AssignAggrState ///////////////////////////////

inline AssignAggrState::AssignAggrState()
	: fact_(true)
{ }

inline bool AssignAggrState::match() const { return true; }
inline void AssignAggrState::finish()
{
	foreach(Assign &assign, assign_) { assign.isNew = false; }
}

inline bool AssignAggrState::fact() const { return fact_; }
inline bool AssignAggrState::isNew() const { return (current_ - 1)->isNew; }
inline void AssignAggrState::lock() { (current_ - 1)->locked = true; }

inline bool AssignAggrState::bindFirst(Val *val)
{
	current_ = assign_.begin();
	return bindNext(val);
}
inline bool AssignAggrState::bindNext(Val *val)
{
	if(current_ != assign_.end())
	{
		*val = current_++->val;
		return true;
	}
	return false;
}

inline AssignAggrState::~AssignAggrState() { }

/////////////////////////////// AggrDomain ///////////////////////////////

inline void AggrDomain::markNew() { new_ = true; }
inline bool AggrDomain::hasNew() const { return new_; }
inline AggrState *AggrDomain::last() const { return last_; }

/////////////////////////////// AggrLit ///////////////////////////////

inline Term *AggrLit::assign() const { return assign_ ? lower_.get() : 0; }

inline Term *AggrLit::lower() const { return lower_.get(); }

inline Term *AggrLit::upper() const { return assign_ ? lower_.get() : upper_.get(); }

inline void AggrLit::sign(bool s) { sign_ = s; }

inline bool AggrLit::sign() const { return sign_; }

inline bool AggrLit::match(Grounder *)
{
	// NOTE: should not be called
	assert(false);
	return false;
}

inline AggrDomain &AggrLit::domain() { return domain_; }
inline CondLitVec &AggrLit::conds() { return conds_; }

inline Lit::Monotonicity AggrLit::monotonicity() const { return NONMONOTONE; }

inline Lit::Score AggrLit::score(Grounder *, VarSet &) { return Score(LOWEST, std::numeric_limits<double>::max()); }

/////////////////////////////// SetLit ///////////////////////////////

inline TermPtrVec *SetLit::terms() { return &terms_; }

inline bool SetLit::match(Grounder *) { return true; }
inline bool SetLit::fact() const { return true; }

inline void SetLit::addDomain(Grounder *, bool) { assert(false); }
inline void SetLit::index(Grounder *, Groundable *, VarSet &) { }
inline bool SetLit::edbFact() const { return false; }
inline void SetLit::accept(Printer *) { }

/////////////////////////////// CondLit::Printer ///////////////////////////////

inline CondLit::Printer::~Printer() { }

/////////////////////////////// CondLit ///////////////////////////////

inline void CondLit::add(Lit *lit) { lits_.push_back(lit); }

inline AggrLit *CondLit::aggr() { return aggr_; }

inline TermPtrVec *CondLit::terms() { return set_.terms(); }

inline LitPtrVec *CondLit::lits() { return &lits_; }

inline CondLit::Style CondLit::style() { return style_;  }
