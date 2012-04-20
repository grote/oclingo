//
// Copyright (c) 2006-2007, Benjamin Kaufmann
//
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/
//
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#include <iostream>
#include <clingcon/gecodesolver.h>
#include <clasp/program_builder.h>
#include <clasp/solver.h>
#include <clingcon/propagator.h>
#include <gecode/search.hh>
#include <gecode/int.hh>
//#include <gecode/minimodel.hh>
#include <exception>
#include <sstream>
#include <clingcon/cspconstraint.h>
#include <gringo/litdep.h>
#include <gecode/kernel/wait.hh>
#include <clingcon/gecodeconflict.h>
#include <clingcon/gecodereason.h>
#include <clingcon/gecodereifwait.h>

using namespace Gecode;
using namespace Clasp;
namespace Clingcon {


Gecode::IntConLevel ICL;
Gecode::IntVarBranch branchVar; //integer branch variable
Gecode::IntValBranch branchVal; //integer branch value

std::string GecodeSolver::num2name(Clasp::Var var)
{
    Clasp::AtomIndex& index = *s_->strategies().symTab;
    for (Clasp::AtomIndex::const_iterator it = index.begin(); it != index.end(); ++it) {
        if ( it->second.lit.var()==var) {
            if (!it->second.name.empty()) {
                {
                return it->second.name.c_str();
                }
            }else
                return "leer!";
        }
    }
    std::stringstream s;
    s<< var;
    return s.str();
}


std::vector<int> GecodeSolver::optValues;
bool             GecodeSolver::optAll = false;

GecodeSolver::GecodeSolver(bool lazyLearn, bool weakAS, int numAS,
                           const std::string& ICLString, const std::string& branchVarString,
                           const std::string& branchValString, std::vector<int> optValueVec,
                           bool optAllPar, bool initialLookahead, const std::string& reduceReason,
                           const std::string& reduceConflict, unsigned int cspPropDelay, unsigned int cloning) :
    /*currentSpace_(0),*/ lazyLearn_(lazyLearn), weakAS_(weakAS), numAS_(numAS), enumerator_(0), dfsSearchEngine_(0), babSearchEngine_(0),
    dummyReason_(this), updateOpt_(false), conflictAnalyzer_(0), reasonAnalyzer_(0), recording_(true),
    initialLookahead_(initialLookahead), cspPropDelay_(abs(cspPropDelay)), cspPropDelayCounter_(1), propagated_(0), deepCopy_(cloning), deepCopyCounter_(0)
{
    if (deepCopy_==0) deepCopyCounter_=1;
    optValues.insert(optValues.end(),optValueVec.begin(), optValueVec.end());
    propToDoLength_.push_back(0);

    optAll=optAllPar;

    ///////////////////////
    //// Currently the solver relies on the fact that i have no equivalences/antivalences between constraint literals
    ///////////////////////

    if(ICLString == "bound") ICL = ICL_BND;
    if(ICLString == "domain") ICL = ICL_DOM;
    if(ICLString == "default") ICL = ICL_DEF;
    if(ICLString == "value") ICL = ICL_VAL;

    if(branchVarString == "none") branchVar = INT_VAR_NONE;
    if(branchVarString == "rnd") branchVar = INT_VAR_RND;
    if(branchVarString == "degree-min") branchVar = INT_VAR_DEGREE_MIN;
    if(branchVarString == "degreee-max") branchVar = INT_VAR_DEGREE_MAX;
    if(branchVarString == "afc-min") branchVar = INT_VAR_AFC_MIN;
    if(branchVarString == "afc-max") branchVar = INT_VAR_AFC_MAX;
    if(branchVarString == "min-min") branchVar = INT_VAR_MIN_MIN;
    if(branchVarString == "min-max") branchVar = INT_VAR_MIN_MAX;
    if(branchVarString == "max-min") branchVar = INT_VAR_MAX_MIN;
    if(branchVarString == "max-max") branchVar = INT_VAR_MAX_MAX;
    if(branchVarString == "size-min") branchVar = INT_VAR_SIZE_MIN;
    if(branchVarString == "size-max") branchVar = INT_VAR_SIZE_MAX;
    if(branchVarString == "size-degree-min") branchVar = INT_VAR_SIZE_DEGREE_MIN;
    if(branchVarString == "size-degree-max") branchVar = INT_VAR_SIZE_DEGREE_MAX;
    if(branchVarString == "size-afc-min") branchVar = INT_VAR_SIZE_AFC_MIN;
    if(branchVarString == "size-afc-max") branchVar = INT_VAR_SIZE_AFC_MAX;
    if(branchVarString == "regret-min-min") branchVar = INT_VAR_REGRET_MIN_MIN;
    if(branchVarString == "regret-min-max") branchVar = INT_VAR_REGRET_MIN_MAX;
    if(branchVarString == "regret-max-min") branchVar = INT_VAR_REGRET_MAX_MIN;
    if(branchVarString == "regret-max-max") branchVar = INT_VAR_REGRET_MAX_MAX;

    if(branchValString == "min") branchVal = INT_VAL_MIN;
    if(branchValString == "med") branchVal = INT_VAL_MED;
    if(branchValString == "max") branchVal = INT_VAL_MAX;
    if(branchValString == "rnd") branchVal = INT_VAL_RND;
    if(branchValString == "split-min") branchVal = INT_VAL_SPLIT_MIN;
    if(branchValString == "split-max") branchVal = INT_VAL_SPLIT_MAX;
    if(branchValString == "range-min") branchVal = INT_VAL_RANGE_MIN;
    if(branchValString == "range-max") branchVal = INT_VAL_RANGE_MAX;
    if(branchValString == "values-min") branchVal = INT_VALUES_MIN;
    if(branchValString == "values-max") branchVal = INT_VALUES_MAX;

    if (reduceReason == "simple")         reduceReason_ = SIMPLE;
    if (reduceReason == "backward")       reduceReason_ = LINEAR;
    if (reduceReason == "forward")        reduceReason_ = LINEAR_FWD;
    if (reduceReason == "cc")             reduceReason_ = CC;
    if (reduceReason == "range")          reduceReason_ = RANGE;
    if (reduceReason == "ccrange")        reduceReason_ = CCRANGE;


    if (reduceConflict == "simple")       reduceConflict_ = SIMPLE;
    if (reduceConflict == "backward")     reduceConflict_ = LINEAR;
    if (reduceConflict == "forward")      reduceConflict_ = LINEAR_FWD;
    if (reduceConflict == "cc")           reduceConflict_ = CC;
    if (reduceConflict == "range")        reduceConflict_ = RANGE;
    if (reduceConflict == "ccrange")      reduceConflict_ = CCRANGE;


}

GecodeSolver::~GecodeSolver()
{

    for (std::vector<SearchSpace*>::iterator i = spaces_.begin(); i != spaces_.end(); ++i)
        delete *i;

    constraints_.clear();
    globalConstraints_.clear();

    delete conflictAnalyzer_;
    delete reasonAnalyzer_;
    delete dfsSearchEngine_;
    delete babSearchEngine_;
    delete enumerator_;
}


void GecodeSolver::propagateLiteral(const Clasp::Literal& l, int)
{
    propQueue_.push_back(l);
}

void GecodeSolver::reset()
{
    propQueue_.clear();
}

void GecodeSolver::addLitToIndex(Clasp::Literal lit, unsigned int index)
{
    litToIndex_[lit] = index;
    indexToLit_[index] = lit;
}

unsigned int GecodeSolver::currentDL() const
{
    assert(dl_.size());
    return dl_.back();
}

GecodeSolver::SearchSpace* GecodeSolver::getCurrentSpace()
{
    size_t i = spaces_.size()-2;

    if (spaces_.back()!=0)
    {
        return spaces_.back();
    }
    while(i>=0)
    {
        // this only happens if we backtracked and did not deepCopied the backtrack point

        if (spaces_[i]!=0)
        {
            recording_=false;
            spaces_.back()=static_cast<GecodeSolver::SearchSpace*>(spaces_[i]->clone());
            spaces_.back()->propagate(assignment_.begin()+assLength_[i], assignment_.begin()+propagated_);

            SpaceStatus s = spaces_.back()->status();
            assert(s!=SS_FAILED);
            recording_=true;
            return spaces_.back();
        }
        --i;
    }
}

bool GecodeSolver::initialize()
{
    dfsSearchEngine_ = 0;
    babSearchEngine_ = 0;
    enumerator_ = 0;
    if (optimize_)
    {
        weakAS_=false;
        numAS_=0;
    }

    if (domain_.left < Int::Limits::min)
    {
        domain_.left = Int::Limits::min;
        std::cerr << "Warning: Raised lower domain to " << domain_.left;
    }

    if (domain_.right > Int::Limits::max+1)
    {
        domain_.right = Int::Limits::max+1;
        std::cerr << "Warning: Reduced upper domain to " << domain_.right-1;
    }



    /////////
    // Register all variables in the Constraints
    // Convert uids to solver literal ids
    // Guess initial domains of level0 constraints        //if (tester->failed() || tester->status()==Gecode::SS_FAILED)
    GecodeSolver::ConstraintMap newConstraints;
    AtomIndex::const_iterator begin = s_->strategies().symTab->begin();
    GecodeSolver::ConstraintMap::iterator i = constraints_.begin();
    while(i != constraints_.end())
    {

        begin = s_->strategies().symTab->lower_bound(begin, i->first.index());

        Clasp::Literal newLit = begin->second.lit;

        // convert uids to solver literal ids

        i->second->registerAllVariables(this);

        //guess domains of already decided constraints
        if (s_->isTrue(newLit))
        {
            guessDomains(i->second, true);
        }
        else
        if (s_->isFalse(newLit))
        {
            guessDomains(i->second, false);
        }
        else
        {
            s_->addWatch(newLit, clingconPropagator_,0);
            s_->addWatch(~newLit, clingconPropagator_,0);
        }

        newConstraints.insert(newLit, constraints_.release(i).release());
        i = constraints_.begin();

    }
    constraints_.clear();
    constraints_.swap(newConstraints);

    for (LParseGlobalConstraintPrinter::GCvec::iterator i = globalConstraints_.begin(); i != globalConstraints_.end(); ++i)
        for (boost::ptr_vector<IndexedGroundConstraintVec>::iterator j = i->heads_.begin(); j != i->heads_.end(); ++j)
            for (IndexedGroundConstraintVec::iterator k = j->begin(); k != j->end(); ++k)
            {
                k->a_->registerAllVariables(this);
                k->b_.registerAllVariables(this);
            }

    // End
    /////////

    // propagate empty constraint set, maybe some trivial constraints can be fullfilled
    spaces_.push_back(new SearchSpace(this, variables_.size(), constraints_, globalConstraints_)); // special root space
    dl_.push_back(0);
    assLength_.push_back(0);



    switch(reduceConflict_)
    {
        case SIMPLE:         conflictAnalyzer_ = new SimpleCA(); break;
        case LINEAR:         conflictAnalyzer_ = new LinearIISCA(this); break;
        case LINEAR_FWD:     conflictAnalyzer_ = new FwdLinearIISCA(this); break;
        case CC:             conflictAnalyzer_ = new CCIISCA(this); break; break;
        case RANGE:          conflictAnalyzer_ = new RangeCA(this); break;
        case CCRANGE:        conflictAnalyzer_ = new CCRangeCA(this); break;
        default: assert(false);
    };

    switch(reduceReason_)
    {
        case SIMPLE:         reasonAnalyzer_ = new SimpleRA(); break;
        case LINEAR:         reasonAnalyzer_ = new LinearIRSRA(this); break;
        case LINEAR_FWD:     reasonAnalyzer_ = new FwdLinearIRSRA(this); break;
        case CC:             reasonAnalyzer_ = new CCIRSRA(this); break;
        case RANGE:          reasonAnalyzer_ = new RangeIRSRA(this); break;
        case CCRANGE:        reasonAnalyzer_ = new CCRangeRA(this); break;
        default: assert(false);
    };

    constraints_.clear();
    globalConstraints_.clear();

    if(!getCurrentSpace()->failed() && getCurrentSpace()->updateOptValues() && !getCurrentSpace()->failed() && getCurrentSpace()->status() != SS_FAILED)
    {
        if (!propagateNewLiteralsToClasp(0))
        {
            propQueue_.push_back(negLit(0));
            return false;
        }
        else
        {
            if (initialLookahead_)
            {
                //initial lookahead

                for (std::map<Clasp::Literal, unsigned int>::const_iterator i = litToIndex_.begin(); i != litToIndex_.end(); ++i)
                {
                    Clasp::Literal test(i->first);

                    for (unsigned int both = 0; both < 2; ++both)
                    {
                        derivedLits_.clear();
                        SearchSpace* tester = getRootSpace();

                        tester->propagate(test);
                        if (tester->failed() || tester->status()==SS_FAILED)
                        {

                            if (!s_->force(~test,Antecedent()))
                            {

                                propQueue_.push_back(negLit(0));
                                return false;
                            }
                            getCurrentSpace()->propagate(~test);
                        }
                        else
                        {
                            ClauseCreator gc(s_);
                            for (Clasp::LitVec::const_iterator j = derivedLits_.begin(); j != derivedLits_.end();++j)
                            {
                                if (*j==test)
                                    continue;

                                gc.start();
                                gc.add(~test);
                                gc.add(*j);
                                if(!gc.end())
                                {
                                    assert(false); // should not happen as both literals are free before
                                    return false;
                                }
                             }
                         }
                        delete tester;
                        test=~test;
                    }
                }
                assignment_.clear();
                propagated_=0;
            }
            variableMap_.clear();
            return true;
        }
    }
    else
    {
        variableMap_.clear();
        propQueue_.push_back(negLit(0));
        return false;
    }
}


void GecodeSolver::newlyDerived(Clasp::Literal lit)
{

    if (recording_)
    {
        // ignore it if it is already assigned on the current decision level, we do this manually
        if (s_->level(lit.var())==dl_[spaces_.size()-1] && s_->isTrue(lit))
                return;
        //std::cout << "push back derived: " << lit.sign() << " " << num2name(lit.var()) << "@" << s_->level(lit.var()) << " " << s_->isTrue(lit) << " " << s_->isFalse(lit) << std::endl;
        derivedLits_.push_back(lit);
        //assignment_.insert(assignment_.begin()+propagated_,lit);
        assignment_.push_back(lit);
        propagated_++;
    }
}


void addToDomain(GecodeSolver::DomainMap& domain,  const GecodeSolver::DomainMap& add);
GecodeSolver::DomainMap intersect(const GecodeSolver::DomainMap& a,  const GecodeSolver::DomainMap& b);
GecodeSolver::DomainMap unite(const GecodeSolver::DomainMap& a,  const GecodeSolver::DomainMap& b);
GecodeSolver::DomainMap xorit(const GecodeSolver::DomainMap& a,  const GecodeSolver::DomainMap& b);
GecodeSolver::DomainMap eq(const GecodeSolver::DomainMap& a,  const GecodeSolver::DomainMap& b);

void GecodeSolver::guessDomains(const Constraint* c, bool val)
{
    DomainMap dom(guessDomainsImpl(c));
    if (!val)
        for (DomainMap::iterator i = dom.begin(); i != dom.end(); ++i)
            i->second.complement(Interval<int>(Int::Limits::min, Int::Limits::max+1));

    addToDomain(domains_, dom);
}

GecodeSolver::DomainMap GecodeSolver::guessDomainsImpl(const Constraint* c)
{
    DomainMap m;
    switch(c->getType())
    {
    case CSPLit::AND:
    {
        Constraint const* a;
        Constraint const* b;
        c->getConstraints(a,b); 
        m = intersect(guessDomainsImpl(a), guessDomainsImpl(b));
        return m;
    }
    case CSPLit::OR:
    {
        Constraint const* a;
        Constraint const* b;
        c->getConstraints(a,b);
        m = unite(guessDomainsImpl(a), guessDomainsImpl(b));
        return m;
    }
    case CSPLit::XOR:
    {
        Constraint const* a;
        Constraint const* b;
        c->getConstraints(a,b);;
        m = xorit(guessDomainsImpl(a), guessDomainsImpl(b));
        return m;
    }
    case CSPLit::EQ:
    {
        Constraint const* a;
        Constraint const* b;
        c->getConstraints(a,b);
        m = eq(guessDomainsImpl(a), guessDomainsImpl(b));
        return m;
    }
    }

    const GroundConstraint* a(0);
    const GroundConstraint* b(0);
    CSPLit::Type comp = c->getRelations(a,b);
    if ((a->isVariable() && b->isInteger()) || (b->isVariable() && a->isInteger()))
    {
        unsigned int var = -1;
        int value = 0;
        if (a->isVariable())
        {
            assert(b->isInteger());
            var = getVariable(a->getString());
            value = b->getInteger();
        }
        else
        {
            assert(b->isVariable());
            assert(a->isInteger());
            switch(comp)
            {
            case CSPLit::GREATER:  comp = CSPLit::LOWER;   break;
            case CSPLit::LOWER:    comp = CSPLit::GREATER; break;
            case CSPLit::EQUAL:    comp = CSPLit::EQUAL; break;
            case CSPLit::GEQUAL:   comp = CSPLit::LEQUAL;  break;
            case CSPLit::LEQUAL:   comp = CSPLit::GEQUAL;  break;
            case CSPLit::INEQUAL:  comp = CSPLit::INEQUAL;   break;
            }
            var = getVariable(b->getString());
            value = a->getInteger();
        }
        m[var] = getSet(var, comp, value);
        return m;
    }
    return m;
}

GecodeSolver::Domain GecodeSolver::getSet(unsigned int var, CSPLit::Type t, int x) const
{
    Domain ret;

    switch(t)
    {
    case CSPLit::GREATER: ret.unite(Interval<int>(x+1,Int::Limits::max+1));  break;
    case CSPLit::LOWER:   ret.unite(Interval<int>(Int::Limits::min, x-1+1)); break;
    case CSPLit::GEQUAL:  ret.unite(Interval<int>(x,Int::Limits::max+1));    break;
    case CSPLit::LEQUAL:  ret.unite(Interval<int>(Int::Limits::min, x+1));   break;
    case CSPLit::EQUAL:   ret.unite(Interval<int>(x,x+1));                   break;
    case CSPLit::INEQUAL: ret.unite(Interval<int>(x,x+1)); ret.complement(Interval<int>(Int::Limits::min, Int::Limits::max+1)); break;
    }
    return ret;
}

void addToDomain(GecodeSolver::DomainMap& domain,  const GecodeSolver::DomainMap& add)
{
    GecodeSolver::DomainMap::iterator found = domain.begin();
    GecodeSolver::DomainMap::iterator last = found; // last valid position
    for (GecodeSolver::DomainMap::const_iterator i = add.begin(); i != add.end(); ++i)
    {

        found = domain.lower_bound(i->first);

        //if not found
        if (found == domain.end())
        {
            domain.insert(last, std::make_pair(i->first,i->second));

        }
        else // still not found
        if (found->first != i->first)
        {
            --found;
            // add it to the domain
            domain.insert(found, std::make_pair(i->first,i->second));
            last = found;
        }
        else // found
        {
            found->second.intersect(i->second);
            last = found;
        }
    }
}

GecodeSolver::DomainMap intersect(const GecodeSolver::DomainMap& a,  const GecodeSolver::DomainMap& b)
{
    GecodeSolver::DomainMap ret;
    for (GecodeSolver::DomainMap::const_iterator i = a.begin(); i != a.end(); ++i)
    {
        GecodeSolver::DomainMap::const_iterator found = b.find(i->first);

        if (found == b.end())
        {
            ret[i->first] = i->second;
        }
        else
        {
            GecodeSolver::Domain inter = i->second;
            inter.intersect(found->second);
            ret[i->first]=inter;
        }
    }

    // can optimize this
    // all variables that we have only in b but not in a
    for (GecodeSolver::DomainMap::const_iterator i = b.begin(); i != b.end(); ++i)
    {
        GecodeSolver::DomainMap::const_iterator found = a.find(i->first);

        if (found == a.end())
        {
            ret[i->first] = i->second;
        }
    }
    return ret;
}

GecodeSolver::DomainMap unite(const GecodeSolver::DomainMap& a,  const GecodeSolver::DomainMap& b)
{
    GecodeSolver::DomainMap ret;
    for (GecodeSolver::DomainMap::const_iterator i = a.begin(); i != a.end(); ++i)
    {
        GecodeSolver::DomainMap::const_iterator found = b.find(i->first);

        if (found == b.end())
        {
        }
        else
        {
            GecodeSolver::Domain inter = i->second;
            inter.unite(found->second);
            ret[i->first]=inter;
        }
    }
    return ret;
}

GecodeSolver::DomainMap xorit(const GecodeSolver::DomainMap& a,  const GecodeSolver::DomainMap& b)
{
    // (a \/ b) /\ (-a \/ -b)
    GecodeSolver::DomainMap ret;
    for (GecodeSolver::DomainMap::const_iterator i = a.begin(); i != a.end(); ++i)
    {
        GecodeSolver::DomainMap::const_iterator found = b.find(i->first);

        if (found == b.end())
        {
        }
        else
        {
            GecodeSolver::Domain l(i->second);
            l.unite(found->second);
            GecodeSolver::Domain nega = i->second;
            nega.complement(GecodeSolver::IntInterval(Int::Limits::min, Int::Limits::max+1));
            GecodeSolver::Domain negb = found->second;
            negb.complement(GecodeSolver::IntInterval(Int::Limits::min, Int::Limits::max+1));
            nega.unite(negb);
            l.intersect(nega);
            ret[i->first] = l;
        }
    }
    return ret;
}

GecodeSolver::DomainMap eq(const GecodeSolver::DomainMap& a,  const GecodeSolver::DomainMap& b)
{
    // (a /\ b) \/ (-a /\ -b)
    GecodeSolver::DomainMap ret;
    for (GecodeSolver::DomainMap::const_iterator i = a.begin(); i != a.end(); ++i)
    {
        GecodeSolver::DomainMap::const_iterator found = b.find(i->first);

        if (found == b.end())
        {
        }
        else
        {
            GecodeSolver::Domain l(i->second);
            l.intersect(found->second);
            GecodeSolver::Domain nega = i->second;
            nega.complement(GecodeSolver::IntInterval(Int::Limits::min, Int::Limits::max+1));
            GecodeSolver::Domain negb = found->second;
            negb.complement(GecodeSolver::IntInterval(Int::Limits::min, Int::Limits::max+1));
            nega.intersect(negb);
            l.unite(nega);
            ret[i->first]=l;
        }
    }
    return ret;
}


bool GecodeSolver::hasAnswer()
{
    // first weak answer? of solver assignment A
    delete dfsSearchEngine_;
    delete babSearchEngine_;
    dfsSearchEngine_ = 0;
    babSearchEngine_ = 0;
    asCounter_ = 0;

    unsigned int oldDL = s_->decisionLevel();
    if (!finishPropagation(true))
    {
        return false;
    }
    if (oldDL > s_->decisionLevel()) //we backjumped
    {
        assert(false);
        return false;
    }


    // currently using a depth first search, this could be changed by options
    if (!optimize_)
        dfsSearchEngine_ = new DFS<GecodeSolver::SearchSpace>(getCurrentSpace(), searchOptions_);
    else
        babSearchEngine_ = new BAB<GecodeSolver::SearchSpace>(getCurrentSpace(), searchOptions_);

    if (enumerator_)
    {
        delete enumerator_;
        enumerator_ = 0;
    }
    if (!optimize_)
        enumerator_ = dfsSearchEngine_->next();
    else
        enumerator_ = babSearchEngine_->next();

    //std::cout << "Fail: " << dfsSearchEngine_->statistics().fail << std::endl;
    //std::cout << "Nodes: " << dfsSearchEngine_->statistics().node << std::endl;
    //std::cout << "Propagation: " << dfsSearchEngine_->statistics().propagate << std::endl;
    //std::cout << "Depth: " << dfsSearchEngine_->statistics().depth << std::endl;
    if (enumerator_ != NULL)
    {
        if (optimize_)
        {
            GecodeSolver::optValues.clear();
            for (int i = 0; i < enumerator_->opts_.size(); ++i)
                GecodeSolver::optValues.push_back(enumerator_->opts_[i].val());

            updateOpt_=true;
        }
        asCounter_++;
        return true;
    }
    else
    {
        // only with propagation i can not find a inconsistency, so i will take the whole assignment
        setConflict(assignment_, false, false);
    }
    return false;
}

bool GecodeSolver::nextAnswer()
{
    assert(enumerator_);
    if (weakAS_) return false;
    if (numAS_ && asCounter_ >= numAS_) return false;

    GecodeSolver::SearchSpace* oldEnum = enumerator_;

    ++asCounter_;
    if (!optimize_)
        enumerator_ = dfsSearchEngine_->next();
    else
        enumerator_ = babSearchEngine_->next();
    delete oldEnum;

    if (enumerator_)
    {
        GecodeSolver::optValues.clear();
        for (int i = 0; i < enumerator_->opts_.size(); ++i)
            GecodeSolver::optValues.push_back(enumerator_->opts_[i].val());

    }
    return (enumerator_ != NULL);
}

GecodeSolver::SearchSpace* GecodeSolver::getRootSpace() const
{
    assert(spaces_.size());
    return (spaces_[0]->failed() ? 0 : static_cast<GecodeSolver::SearchSpace*>(spaces_[0]->clone()));
}

void GecodeSolver::setRecording(bool r)
{
    recording_ = r;
}

/*
  4 cases:
  1. all spaces are available and propagated, conflict is complete assignment
  2. unknown number of spaces, conflict is the assigned variables in the index space
  3. unknown number of spaces, conflict is the assigned variables in the index space + 1 more literal
  4. empty conflict
  */
void GecodeSolver::setConflict(Clasp::LitVec conflict, bool last, bool shrink)
{
//        std::cout << "Conflict" << std::endl;
//        for(Clasp::LitVec::const_iterator i = conflict.begin(); i != conflict.end(); ++i)
//            std::cout << i->sign() << " " << num2name(i->var()) << "@" << s_->level(i->var()) << " ";
//        std::cout << std::endl;
    for (Clasp::LitVec::const_iterator i = conflict.begin(); i != conflict.end(); ++i)
        assert(s_->isTrue(*i));
    if (shrink)
        conflictAnalyzer_->shrink(conflict, last);
//    std::cout << "Shrunken" << std::endl;
//    for(Clasp::LitVec::const_iterator i = conflict.begin(); i != conflict.end(); ++i)
//        std::cout << i->sign() << " " << num2name(i->var()) << "@" << s_->level(i->var()) << "  ";
//    std::cout << std::endl;

    if (conflict.size()==0) // if root level conflict due to global constraints
        conflict.push_back(posLit(0));
    s_->setConflict(conflict, true);
    return;
}


std::pair<unsigned int,bool> GecodeSolver::litToIndex(Clasp::Literal lit)
{
    //assert(litToIndex_.find(lit)!= litToIndex_.end());
    // this function will return the index only depending on the variable
    std::map<Clasp::Literal, unsigned int>::iterator f = litToIndex_.find(lit);
    return std::pair<unsigned int,bool>(f==litToIndex_.end() ? litToIndex_[~lit] : f->second, f!=litToIndex_.end());
    //return litToIndex_[lit];
}

Clasp::Literal GecodeSolver::indexToLit(unsigned int index)
{
    assert(indexToLit_.find(index)!= indexToLit_.end());
    return indexToLit_[index];
}


void GecodeSolver::printAnswer()
{
    assert(enumerator_);
    assert(getCurrentSpace());
    if (weakAS_)
        getCurrentSpace()->print(variables_);
    else
        enumerator_->print(variables_);
}

bool GecodeSolver::propagate()
{
    assert(compare(assignment_));
//    std::cout << "Clasp Assignment bofire my propagation on level : " << s_->decisionLevel() << std::endl;
//    for (Clasp::LitVec::const_iterator i = s_->assignment().begin(); i != s_->assignment().end(); ++i)
//        std::cout << i->sign() << " " << num2name(i->var()) << "@" << s_->level(i->var()) << " " << bool(s_->isFalse(*i) | s_->isTrue(*i)) << "  ";
//    std::cout << std::endl;
//    std::cout << "start prop " << std::endl;
//    std::cout << "PropToDoLength: ";
//    for (std::list<size_t>::const_iterator i = propToDoLength_.begin(); i != propToDoLength_.end(); ++i)
//        std::cout << *i << " ";
//    std::cout << std::endl;
//    std::cout << "AssLength: ";
//    for (std::vector<size_t>::const_iterator i = assLength_.begin(); i != assLength_.end(); ++i)
//        std::cout << *i << " ";
//    std::cout << std::endl;
//    std::cout << "DL: ";
//    for (std::vector<size_t>::const_iterator i = dl_.begin(); i != dl_.end(); ++i)
//        std::cout << *i << " ";
//    std::cout << std::endl;
    // if already failed, create conflict, this may be on a lower level
    if (getCurrentSpace()->failed())
    {
        Clasp::LitVec clits(assignment_.begin(), assignment_.begin()+propagated_);
        setConflict(clits, false);
        assert(s_->decisionLevel()==0);
        return false;
    }


    if (updateOpt_)
    {
        return propagateMinimize();
    }

    // remove already assigned literals, this happens if we have propagated a new literal to clasp
    Clasp::LitVec clits;
    //bool newKnowledge = false;

    for (Clasp::LitVec::const_iterator i = propQueue_.begin(); i != propQueue_.end(); ++i)
    {
        // add if free
        // conflict if contradicting
        GecodeSolver::SearchSpace::Value constr = getCurrentSpace()->getValueOfConstraint(*i);
        if ( constr == SearchSpace::BFREE)
        {
            clits.push_back(*i);
            //newKnowledge = true;
        }
        if (constr == SearchSpace::BFALSE)
        {
//                std::cout << "In PropQueue :" << i->sign() << " " << num2name(i->var()) << std::endl;
//                for(Clasp::LitVec::const_iterator k = assignment_.begin(); k !=  assignment_.begin()+propagated_; ++k)
//                    std::cout << k->sign() << " " << num2name(k->var()) << " " << s_->isTrue(*k) << " " << s_->isFalse(*k) << " --- ";
//                std::cout << std::endl;
            clits.clear();
            clits.assign(assignment_.begin(), assignment_.begin()+propagated_);
            clits.push_back(*i);
            setConflict(clits, true);
            return false;
        }
    }
    propQueue_.clear();

    // we have something to propagate
    if (clits.size())
    {
        // if we have a new decision level, create a new space
        if (!_propagate(clits))
        {
//            std::cout << "Clasp Assignment after my propagation on level : " << s_->decisionLevel() << std::endl;
//            for (Clasp::LitVec::const_iterator i = s_->assignment().begin(); i != s_->assignment().end(); ++i)
//                std::cout << i->sign() << " " << num2name(i->var()) << " " << s_->reason(*i).isNull() << " " << s_->level(i->var()) << " ";
//            std::cout << std::endl;
            return false;
        }
//        std::cout << "Clasp Assignment after my propagation on level : " << s_->decisionLevel() << std::endl;
//        for (Clasp::LitVec::const_iterator i = s_->assignment().begin(); i != s_->assignment().end(); ++i)
//            std::cout << i->sign() << " " << num2name(i->var()) << " " << s_->reason(*i).isNull() << " " << s_->level(i->var()) << " ";
//        std::cout << std::endl;
    }
    return true;
}


bool GecodeSolver::_propagate(Clasp::LitVec& clits)
{
    assert(assLength_.size() > 1 ? assLength_[assLength_.size()-2] < assLength_[assLength_.size()-1] : true);

    if (s_->decisionLevel()==0)
    {
        // do not need to store clits to assignment on level 0
        derivedLits_.clear();
        getCurrentSpace()->propagate(clits.begin(), clits.end());
        if (!getCurrentSpace()->failed() && getCurrentSpace()->status() != SS_FAILED)
        {
            // this function avoids propagating already decided literals
            if(!propagateNewLiteralsToClasp(0))
                return false;
        }
        else
        {
            clits.clear();
            setConflict(clits, false);
            return false;
        }

        return true;
    }

    assert(s_->decisionLevel()!=0);
    ++cspPropDelayCounter_;
    if (cspPropDelay_ && cspPropDelayCounter_  > cspPropDelay_)
    {
        cspPropDelayCounter_ = 1;

        //propagate all old knowledge!

        //redo propagation

        derivedLits_.clear();

        size_t old = s_->decisionLevel();
        if (!finishPropagation())
            return false;
        if (s_->decisionLevel() < old)
            return true;






        if (s_->decisionLevel() != currentDL())
        {
            //std::cout << "new space for propagation level " << s_->decisionLevel() << std::endl;
            if (deepCopy_) ++deepCopyCounter_;
            if(deepCopyCounter_==deepCopy_ || spaces_.size()==1)
            {
                deepCopyCounter_=0;
                spaces_.push_back(static_cast<SearchSpace*>(getCurrentSpace()->clone()));

            }
            else
            {
                SearchSpace* t = spaces_.back();
                spaces_.back()=0;
                spaces_.push_back(t);
            }
            //register in solver for undo event
            s_->addUndoWatch(s_->decisionLevel(),clingconPropagator_);
            // this is important to first do propagation and then set assLength, because getCurrentSpace CAN
            // redo propagation
            dl_.push_back(s_->decisionLevel());
            assLength_.push_back(assignment_.size());
        }

        //assignment_.insert(assignment_.end(), clits.begin(), clits.end());
        //if (clits.size())
        //    getCurrentSpace();

        //std::cout << "propagate " << clits.size() << " literals on level " << s_->decisionLevel() << std::endl;

        for(Clasp::LitVec::const_iterator i = clits.begin(); i != clits.end(); ++i)
        {
            //std::cout << "Propagate clit " << i->sign() << " " << num2name(i->var()) << std::endl;
            //getCurrentSpace()->print(variables_); std::cout << std::endl;

            if (s_->level(i->var())==dl_[spaces_.size()-1] && s_->isTrue(*i))
            {
                assignment_.push_back(*i);
                ++propagated_;
            }
            getCurrentSpace()->propagate(*i); // does automatically add to the assignment if not assigned on current decision level

            getCurrentSpace()->status();  // does automatically add to the assignment,

            //getCurrentSpace()->print(variables_); std::cout << std::endl;
            assLength_.back()=propagated_;
//            for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
//                std::cout << i->sign() << " " << num2name(i->var()) << " ";
//            std::cout << std::endl;
            if(!propagateNewLiteralsToClasp(spaces_.size()-1))
                return false;


            if (getCurrentSpace()->failed())
            {
                clits.clear();
                clits.insert(clits.begin(), assignment_.begin(), assignment_.begin()+propagated_);
                setConflict(clits, false);
                return false;
            }
        }
    }
    else
    {
        propToDo_.insert(propToDo_.end(), clits.begin(), clits.end());
        if (s_->decisionLevel() != currentDL())
        {
            dl_.push_back(s_->decisionLevel());
            //std::cout << "did not propagate on dl " << dl_.back() << std::endl;
            //std::cout << "need to prop to " << propToDo_.size() << std::endl;
            propToDoLength_.push_back(propToDo_.size());
            //register in solver for undo event
            s_->addUndoWatch(s_->decisionLevel(),clingconPropagator_);
        }
        else
        {
            propToDoLength_.back() = propToDo_.size();
        }

    }

    return true;
}

bool GecodeSolver::compare(Clasp::LitVec temp)
{
    /*SearchSpace* s = getRootSpace();
    for (Clasp::LitVec::const_iterator i = temp.begin(); i != temp.end(); ++i)
    {
        s->propagate(*i);
        s->status();
    }
    if ((s->failed() && !getCurrentSpace()->failed()) || (!s->failed() && getCurrentSpace()->failed()) )
    {
        std::cout << "NOT EQUAL" << std::endl;
        s->print(variables_);
        getCurrentSpace()->print(variables_);
        return false;
    }

    for (int i = 0; i < s->b_.size(); ++i)
    {
        Int::BoolView v1(s->b_[i]);
        Int::BoolView v2(getCurrentSpace()->b_[i]);
        if ((v1.one() && !v2.one()) || (v1.zero() && !v2.zero()) || (v1.none() && !v2.none()))
        {
            std::cout << "NOT EQUAL" << std::endl;
            s->print(variables_);
            getCurrentSpace()->print(variables_);
            return false;
        }

    }*/
    // now check the assignment, it has to be assigned (only in the beginning of propagation)
    for (Clasp::LitVec::const_iterator i = assignment_.begin(); i != assignment_.end(); ++i)
        if (!s_->isTrue(*i) && !s_->isFalse(*i))
        {
            //std::cout << "Assignment NO LONGER ASSIGNED!" << std::endl;
            return false;
        }
    return true;
}


bool GecodeSolver::finishPropagation(bool onModel)
{
    unsigned int start = 0;
    unsigned int end = 0;
    bool firstTime = true;

//    std::cout << "Finish called on " << currentDL() << std::endl;
//    std::cout << "PropToDoLength: ";
//    for (std::list<size_t>::const_iterator i = propToDoLength_.begin(); i != propToDoLength_.end(); ++i)
//        std::cout << *i << " ";
//    std::cout << std::endl;
//    std::cout << "AssLength: ";
//    for (std::vector<size_t>::const_iterator i = assLength_.begin(); i != assLength_.end(); ++i)
//        std::cout << *i << " ";
//    std::cout << std::endl;
//    std::cout << "DL: ";
//    for (std::vector<size_t>::const_iterator i = dl_.begin(); i != dl_.end(); ++i)
//        std::cout << *i << " ";
//    std::cout << std::endl;

    while(propToDoLength_.size())
    {
        start = end;
        end = *propToDoLength_.begin();

        if (!firstTime) // not working on the current space
        {
            if (deepCopy_) ++deepCopyCounter_;
            if(deepCopyCounter_==deepCopy_ || spaces_.size()==1)
            {
                deepCopyCounter_=0;
                spaces_.push_back(static_cast<SearchSpace*>(getCurrentSpace()->clone()));
            }
            else
            {
                SearchSpace* t = spaces_.back();
                spaces_.back()=0;
                spaces_.push_back(t);
            }
            assLength_.push_back(assignment_.size());

        }
        firstTime=false;



        for(size_t i = start; i < end; ++i)
        {

            derivedLits_.clear();
            //std::cout << "Propagate " << propToDo_.begin()->sign() << " " << num2name(propToDo_.begin()->var()) << std::endl;
            if (s_->level(propToDo_.begin()->var())==dl_[spaces_.size()-1] && s_->isTrue(*propToDo_.begin()))
            {
                assignment_.push_back(*propToDo_.begin());
                ++propagated_;
            }
            recording_=!onModel;
            getCurrentSpace()->propagate(*(propToDo_.begin()));
            propToDo_.pop_front();
            --propToDoLength_.front();
            getCurrentSpace()->status();
            //if (getCurrentSpace()->failed())
            //    std::cout << "Failed" << std::endl;
            //getCurrentSpace()->print(variables_); std::cout << std::endl;

            recording_=true;
            assLength_.back()=propagated_;
//            std::cout << "propagated: " << std::endl;
//            for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
//                std::cout << i->sign() << " " << num2name(i->var()) << "@" << s_->level(i->var()) << " " << s_->isTrue(*i) << " " << s_->isFalse(*i) << " --- ";
//            std::cout << std::endl;
            if (!onModel)
            if(!propagateNewLiteralsToClasp(spaces_.size()-1))
            {
                //std::cout << "conflict while propagating" << std::endl;
                //propToDoLength_.push_front((end-i)-1);
                //propagated_ = assLength_[spaces_.size()-1];// we did do this propagation
                return false;
            }

            if (getCurrentSpace()->failed())
            {
                //std::cout << propagated_ << std::endl;
                //std::cout << assignment_.size() << std::endl;
                Clasp::LitVec ret(assignment_.begin(), assignment_.begin()+propagated_);
//                for (Clasp::LitVec::const_iterator i = assignment_.begin(); i != assignment_.end(); ++i)
//                    std::cout << i->sign() << " " << num2name(i->var()) << " " << s_->isTrue(*i) << " " << s_->isFalse(*i) << "  ";
//                std::cout << std::endl;
                setConflict(ret, false);
                //propToDoLength_.clear();
                //propToDoLength_.push_back(0);
                //propToDo_.clear();
                return false;
            }
        }
        propToDoLength_.pop_front();
    }
    propToDoLength_.clear();
    propToDoLength_.push_back(0);
    propToDo_.clear();
    return true;
}


 bool GecodeSolver::propagateMinimize()
 {
     updateOpt_ = false;
     for (size_t i = 0; i < spaces_.size(); ++i)
     {
         SearchSpace* current = spaces_[i];
         derivedLits_.clear();
         if (current)
         {
             current->updateOptValues();

             //PROBLEM, arbeite direkt auf dem space, dieser kann fehlschlagen!
             if (current->failed() || current->status()==SS_FAILED)
             {
                 if (!propagateNewLiteralsToClasp(i))
                     return false;
                 Clasp::LitVec conflict(assignment_.begin(), assignment_.begin()+ (i==spaces_.size()-1 ? propagated_  : assLength_[i] ));
                 propQueue_.clear();
                 setConflict(conflict, false);
                 return false;
             }
             else
             {
                 if (derivedLits_.size()>0)
                 {
                     if (!propagateNewLiteralsToClasp(i))
                         return false;
                     //if (oldDL > s_->decisionLevel()) //we backjumped
                     //    return true;
                 }
             }
         }
     }
     return propagate();
 }


/* backjump to decision level: unsigned int level
   for undoLevel one has to call with: undoUntil(s_-decisionLevel() - 1)
   remove level
*/
void GecodeSolver::undo(unsigned int level)
{
    assert(level!=0);

    propQueue_.clear();
    assert(level==currentDL());
//    std::cout << "undo level " << level << std::endl;
//    std::cout << "PropToDoLength: ";
//    for (std::list<size_t>::const_iterator i = propToDoLength_.begin(); i != propToDoLength_.end(); ++i)
//        std::cout << *i << " ";
//    std::cout << std::endl;
//    std::cout << "AssLength: ";
//    for (std::vector<size_t>::const_iterator i = assLength_.begin(); i != assLength_.end(); ++i)
//        std::cout << *i << " ";
//    std::cout << std::endl;
//    std::cout << "DL: ";
//    for (std::vector<size_t>::const_iterator i = dl_.begin(); i != dl_.end(); ++i)
//        std::cout << *i << " ";
//    std::cout << std::endl;


    dl_.pop_back();

    if (propToDoLength_.size()>1)
    {
        propToDoLength_.pop_back();
        propToDo_.resize(propToDoLength_.back());
        return;
    }
    else
    if (propToDoLength_.back()!=0)
    {
        propToDoLength_.back() = 0;
        propToDo_.resize(propToDoLength_.back());
        //return;
    }
    // only if we are state of the art

    assLength_.pop_back();
    assignment_.resize(assLength_.back());

    if (assignment_.size()<propagated_) // if assignment < propagated we have to backtrack, otherwise not
        propagated_=assignment_.size();
    if(spaces_.size()>dl_.size())
    {
        delete spaces_.back();
        spaces_.pop_back();
    }
    return;
}

void GecodeSolver::printStatistics()
{
    conflictAnalyzer_->printStatistics();
    reasonAnalyzer_->printStatistics();
}


bool GecodeSolver::propagateNewLiteralsToClasp(size_t level)
{
    size_t all = (level==spaces_.size()-1) ? propagated_  : assLength_[level];
/*    bool allTrue=true;
    for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
    {
        if (!s_->isTrue(*i))
        {
//            Clasp::Literal conf(~(*i));

//            derivedLits_.clear();
//            derivedLits_.reserve(propagated_+1);
//            derivedLits_.insert(derivedLits_.begin(), assignment_.begin(), assignment_.begin()+propagated_-(derivedLits_.end()-i));
//            derivedLits_.push_back(conf);// also add the conflicting literal, this can be on a higher decision level
//            setConflict(derivedLits_, true);
//            return false;
              allTrue=false;
              break;
        }
    }

    if (allTrue) // there where no undef literals
               // just remember all better reasons and avoid backtracking
    {
        for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
        {
            assert(s_->isTrue(*i));
            size_t oldImpl = s_->level(i->var());
            if (oldImpl <= dl_[level])
                continue;
            impliedLits_[oldImpl].push_back(ImpliedLiteral(*i,dl_[level],propagated_-derivedLits_.size()+(i-derivedLits_.begin())));
            std::cout << i->sign() << " " << num2name(i->var());
            std::cout << " used as impliedLits that was implied before on " << oldImpl << "and will hopefully in future be implied on " << level << std::endl;
        }
        std::cout << "allTrue" << std::endl;
        return true;
    }
*/
    uint32 implyon = dl_[level];
//    std::cout << "did some real work" << std::endl;
    if (lazyLearn_)
    {

        // derive undef and true lits because we have better(earlier) reasons
        assert(spaces_.size());

        //std::cout << "AssLength:" << assignment_.size() << std::endl;
        for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
        {
            //std::cout << "Try " << i->sign() << " " << num2name(i->var()) << std::endl;
            if (s_->isTrue(*i) && s_->level(i->var())==implyon)
                assert(false);
            if (s_->isTrue(*i) && s_->level(i->var())<implyon)
                assert(false);
            if (s_->isFalse(*i) && s_->level(i->var())<implyon)
                assert(false);

            //std::cout << propagated_ << " " << derivedLits_.size() << " " << i << std::endl;
            size_t num = level==0 ? 0 : all-derivedLits_.size()+(i-derivedLits_.begin());
            /*if (s_->isFalse(*i))
                {
                    assert(assignment_.size()==propagated_);
                    assert(num <= assignment_.size());
                    Clasp::Literal conf(~(*i));
                    derivedLits_.clear();
                    derivedLits_.reserve(num+1);
                    derivedLits_.insert(derivedLits_.begin(), assignment_.begin(), assignment_.begin()+num);
                    derivedLits_.push_back(conf);// also add the conflicting literal, this can be on a higher decision level

                    setConflict(derivedLits_, false);
                    return false;
                }*/

            litToAssPosition_[(*i)] = num;
//            std::cout << "Add to Implication on level " << implyon << " " << i->sign() << " " << num2name(i->var()) << " " << s_->isTrue(*i) << " " << s_->isFalse(*i) << std::endl;
//            std::cout << "litToAssPosition_[" << i->sign() << " " << num2name(i->var()) << "]=" << litToAssPosition_[(*i)] << std::endl;

//            std::cout << "Vor der addNewImplication auf s_->decisionLevel() " << s_->decisionLevel() << std::endl;
//            std::cout << "PropToDoLength: ";
//            for (std::list<size_t>::const_iterator i = propToDoLength_.begin(); i != propToDoLength_.end(); ++i)
//                std::cout << *i << " ";
//            std::cout << std::endl;
//            std::cout << "AssLength: ";
//            for (std::vector<size_t>::const_iterator i = assLength_.begin(); i != assLength_.end(); ++i)
//                std::cout << *i << " ";
//            std::cout << std::endl;
//            std::cout << "DL: ";
//            for (std::vector<size_t>::const_iterator i = dl_.begin(); i != dl_.end(); ++i)
//                std::cout << *i << " ";
//            std::cout << std::endl;
            if (!s_->addNewImplication(*i,implyon/*s_->decisionLevel()*/,&dummyReason_))
            {
                derivedLits_.clear();
                return false;
            }

            //if (s_->value(i->var())==value_free && s_->decisionLevel() > dl_[level])
            //   break;
            assert(s_->value(i->var())!=value_free);
        }

        derivedLits_.clear();
        return true;
    }
    else
    {

        ClauseCreator gc(s_);

        for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
        {
            if (s_->isTrue(*i) && s_->level(i->var())==implyon)
                assert(false);
            if (s_->isTrue(*i) && s_->level(i->var())<implyon)
                assert(false);
            if (s_->isFalse(*i) && s_->level(i->var())<implyon)
                assert(false);

            s_->undoUntil(implyon);

            gc.startAsserting(Constraint_t::learnt_conflict, *i);

            Clasp::LitVec reason;
            createReason(reason,*i,assignment_.begin(), assignment_.begin()+(level==0 ? 0 : all-derivedLits_.size()+(i-derivedLits_.begin())));

            for (Clasp::LitVec::const_iterator r = reason.begin(); r != reason.end(); ++r)
            {
                assert(s_->isTrue(*r));
                gc.add(~(*r));
            }
            if(!gc.end())
            {
                derivedLits_.clear();
                return false;
            }

            assert(s_->isTrue(*i));
            assert(s_->decisionLevel()>=implyon);
        }

        derivedLits_.clear();
        return true;
    }
    assert(false);
}

void GecodeSolver::createReason(Clasp::LitVec& reason, const Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end)
{
//    std::cout << "Reason for " << l.sign() << " " << num2name(l.var()) << std::endl;
//    for(Clasp::LitVec::const_iterator i = begin; i != end; ++i)
//        std::cout << i->sign() << " " << num2name(i->var()) << " ";
//    std::cout << std::endl;
//    std::cout << "Preset Reason with: " << std::endl;
//    for(Clasp::LitVec::const_iterator i = reason.begin(); i != reason.end(); ++i)
//        std::cout << i->sign() << " " << num2name(i->var()) << " ";
//    std::cout << std::endl;


    reasonAnalyzer_->generate(reason, l, begin, end);

//    std::cout << "Shrunken" << std::endl;
//    for(Clasp::LitVec::const_iterator i = reason.begin(); i != reason.end(); ++i)
//        std::cout << i->sign() << " " << num2name(i->var()) << " ";
//    std::cout << " ";
}

Clasp::ConstraintType GecodeSolver::CSPDummy::reason(const Literal& l, Clasp::LitVec& reason)
{
    //std::cout << l.sign() << " " << gecode_->num2name(l.var()) << " " << gecode_->getSolver()->isFalse(l) << std::endl;
    //assert(gecode_->s_->isTrue(l));
    //std::cout << "litToAssPosition_[" << l.sign() << " " << gecode_->num2name(l.var()) << "]=" << gecode_->litToAssPosition_[l] << std::endl;
    gecode_->createReason(reason, l, gecode_->assignment_.begin(), gecode_->assignment_.begin()+gecode_->litToAssPosition_[l]);
    return Clasp::Constraint_t::learnt_other;
}

GecodeSolver* GecodeSolver::SearchSpace::csps_ = 0;

IntVarArgs GecodeSolver::SearchSpace::iva_;


GecodeSolver::SearchSpace::SearchSpace(GecodeSolver* csps, unsigned int numVar, GecodeSolver::ConstraintMap& constraints,
                                       LParseGlobalConstraintPrinter::GCvec& gcvec) : Space(),
    x_(*this, numVar),
    b_()
{
    csps_ = csps;

    //initialize all variables with their domain
    for(size_t i = 0; i < csps_->getVariables().size(); ++i)
    {
        GecodeSolver::Domain def;
        def.unite(csps_->domain_);
        if (csps_->domains_.find(i)==csps_->domains_.end())
        {
            csps_->domains_[i]=def;
        }
        else
            csps_->domains_[i].intersect(def);
        typedef int Range[2];
        Range* array = new Range[csps_->domains_[i].size()];
        unsigned int count=0;
        for (GecodeSolver::Domain::ConstIterator j = csps_->domains_[i].begin(); j != csps_->domains_[i].end(); ++j)
        {
            array[count][0]=j->left;
            array[count++][1]=j->right-1;
        }

        // if we have  an empty domain
        if (csps_->domains_[i].size()==0)
        {
            fail();
            return;
        }

        IntSet is(array, csps_->domains_[i].size());
        delete[] array;
        x_[i] = IntVar(*this, is);
    }

    csps_->domains_.clear();


    // set static constraints
    int numReified = 0;
    for(GecodeSolver::ConstraintMap::iterator i = constraints.begin(); i != constraints.end(); ++i)
    {
        if (csps_->getSolver()->value(i->first.var()) == value_free)
            ++numReified;
        else
        {
            generateConstraint(i->second,csps_->getSolver()->isTrue(i->first) == value_true);
        }
    }

    //TODO: I think the BoolVarArray is now of dynamic size, so i could use this feature to save a loop
    b_ = BoolVarArray(*this,numReified,0,1);
    unsigned int counter = 0;

    Waitress* w = new (*this) Waitress(*this);

    for(GecodeSolver::ConstraintMap::iterator i = constraints.begin(); i != constraints.end(); ++i)
    {
        if (csps_->getSolver()->value(i->first.var()) == value_free)
         {
             csps_->addLitToIndex(i->first, counter);
             generateConstraint(i->second, counter);
             // be very careful here, we use vars to refer to constraints, but constraints are literals
             // and maybe negated literals due to equality preprocessing! Currently this is not the case
             w->init(*this,csps_, Int::BoolView(b_[counter]), csps_->indexToLit(counter).var());
             ++counter;
         }
    }


    std::map<unsigned int,std::vector<std::pair<GroundConstraint*,bool> > > optimize; // true means maximize, false means minimize
    for (size_t i = 0; i < gcvec.size(); ++i)
    {
        if (!(gcvec[i].type_ == MINIMIZE || gcvec[i].type_ == MINIMIZE_SET || gcvec[i].type_ == MAXIMIZE || gcvec[i].type_ == MAXIMIZE_SET))
            generateGlobalConstraint(gcvec[i]);
        else
        {
            LParseGlobalConstraintPrinter::GC& gc = gcvec[i];
            IndexedGroundConstraintVec& igcv = gc.heads_[0];
            for (size_t i = 0; i < igcv.size(); ++i)
            {
                bool max = false;
                if (gc.type_ == MAXIMIZE_SET || gc.type_ == MAXIMIZE)
                    max=true;
                optimize[igcv[i].b_.getInteger()].push_back(std::pair<GroundConstraint*,bool>(igcv[i].a_.get(), max));
                csps_->setOptimize(true);
            }
        }
    }

    //if (GecodeSolver::optValues.size()>optimize.size())
    GecodeSolver::optValues.resize(optimize.size(),Int::Limits::max-1);
    if (optValues.size()>0)
        ++optValues.back();
    opts_ = IntVarArray(*this, optimize.size(), Int::Limits::min, Int::Limits::max);


    size_t index = 0;
    for (std::map<unsigned int,std::vector<std::pair<GroundConstraint*,bool> > >::iterator i = optimize.begin(); i != optimize.end(); ++i)
    {
        LinExpr expr(generateSum(i->second));
        rel(*this, LinRel(opts_[index],IRT_EQ,expr), ICL);

        ++index;
    }

    iva_ << opts_;
    iva_ << x_;

    std::sort(iva_.begin(), iva_.end(), boost::bind(&IntVar::before,_1,_2));
    IntVarArgs::iterator newEnd = std::unique(iva_.begin(), iva_.end(), boost::bind(&IntVar::same,_1,_2));

    IntVarArgs temp;
    if (iva_.size())
        temp << iva_.slice(0,1,std::distance(iva_.begin(),newEnd));
    branch(*this, temp, branchVar, branchVal);
    iva_ = IntVarArgs();
}

GecodeSolver::SearchSpace::SearchSpace(bool share, SearchSpace& sp) : Space(share, sp)
{
    x_.update(*this, share, sp.x_);
    b_.update(*this, share, sp.b_);
    opts_.update(*this,share,sp.opts_);
}

GecodeSolver::SearchSpace* GecodeSolver::SearchSpace::copy(bool share)
{
    return new GecodeSolver::SearchSpace(share, *this);
}

//optimize function
void GecodeSolver::SearchSpace::constrain(const Space& _b)
{
    updateOptValues();
}

bool GecodeSolver::SearchSpace::updateOptValues()
{
    if (GecodeSolver::optValues.size()==0)
        return true;
    assert(opts_.size() <= GecodeSolver::optValues.size());
    if (GecodeSolver::optValues.size()>1)
    {
        rel(*this, opts_[0] <= GecodeSolver::optValues[0], ICL);
    }

    BoolVarArray lhs(*this,opts_.size()-1, 0,1);
    for (int i = 0; i < opts_.size()-1; ++i)
    {
        rel(*this, opts_[i], IRT_EQ, GecodeSolver::optValues[i], lhs[i], ICL);

        BoolVar rhs(*this,0,1);

        if (i+1==opts_.size()-1)
        {
            if (GecodeSolver::optAll)
                rel(*this, opts_[i+1], IRT_LQ, GecodeSolver::optValues[i+1], rhs, ICL);
            else
                rel(*this, opts_[i+1], IRT_LE, GecodeSolver::optValues[i+1], rhs, ICL);
        }
        else
            rel(*this, opts_[i+1], IRT_LQ, GecodeSolver::optValues[i+1], rhs, ICL);
        BoolVar allBefore(*this,0,1);
        rel(*this, BOT_AND, lhs.slice(0,1,i+1), allBefore, ICL);
        rel(*this, allBefore, BOT_IMP, rhs, 1, ICL);// changes to: "rel(home, BOT_IMP, x, y) where x is an array of Boolean variable now assumes implication to be right associative. See MPG for explanation. (minor)" in new version
    }

    if (opts_.size()==1)
    {
        if (GecodeSolver::optAll)
            rel(*this, opts_[0] <= GecodeSolver::optValues[0],ICL);
        else
            rel(*this, opts_[0] < GecodeSolver::optValues[0],ICL);
    }
    return !failed();

}

void GecodeSolver::SearchSpace::propagate(const Clasp::LitVec::const_iterator& lvstart, const Clasp::LitVec::const_iterator& lvend)
{
    for (Clasp::LitVec::const_iterator i = lvstart; i < lvend; ++i)
    {
        if (!propagate(*i))
            return;
    }
}

bool GecodeSolver::SearchSpace::propagate(const Clasp::Literal& i)
{
    std::pair<unsigned int, bool> index(csps_->litToIndex(Clasp::Literal::fromIndex(i.index())));

    Int::BoolView bv(b_[index.first]);
    if (index.second)
    {
        if(Gecode::Int::ME_BOOL_FAILED == bv.one(*this))
        {
            fail();
            return false;
        }
    }
    else
    {
        if(Gecode::Int::ME_BOOL_FAILED == bv.zero(*this))
        {
            fail();
            return false;
        }
    }
    return true;
}

void GecodeSolver::SearchSpace::print(const std::vector<std::string>& variables) const
{
    // i use ::operator due to gecode namespace bug
    for (int i = 0; i < x_.size(); ++i)
    {

        Int::IntView v(x_[i]);
        std::cout << variables[i] << "=";
        std::cout << v;
        std::cout << " ";
    }
//    for (int i = 0; i < b_.size(); ++i)
//    {
//        Int::BoolView v(b_[i]);
//        std::cout << csps_->indexToLit(i).sign() << " " << csps_->num2name(csps_->indexToLit(i).var()) << "=";
//        std::cout << v;
//        std::cout << " ";
//    }
}

GecodeSolver::SearchSpace::Value GecodeSolver::SearchSpace::getValueOfConstraint(const Clasp::Literal& i)
{
    std::pair<unsigned int, bool> index(csps_->litToIndex(Clasp::Literal::fromIndex(i.index())));
    Int::BoolView bv(b_[index.first]);
    if (bv.status() == Int::BoolVarImp::NONE)
        return BFREE;
    else
        if ((bv.status() == Int::BoolVarImp::ONE && index.second)  ||  (bv.status() == Int::BoolVarImp::ZERO && !index.second))
            return BTRUE;
	else
            return BFALSE;
}

void GecodeSolver::SearchSpace::generateConstraint(const Constraint* c, bool val)
{
    if (c->isSimple())
    {
        generateLinearRelation(c).post(*this,val,ICL);
    }
    else
    {
        Gecode::rel(*this, !generateBooleanExpression(c),ICL);
    }
}

void GecodeSolver::SearchSpace::generateConstraint(const Constraint* c, unsigned int boolvar)
{
    if (c->isSimple())
    {
        Gecode::rel(*this, generateLinearRelation(c) == b_[boolvar],ICL);
    }
    else
    {
        Gecode::rel(*this, generateBooleanExpression(c) == b_[boolvar],ICL);
    }
}

Gecode::LinRel GecodeSolver::SearchSpace::generateLinearRelation(const Constraint* c)
{
    assert(c);
    assert(c->isSimple());
    const GroundConstraint* x;
    const GroundConstraint* y;
    CSPLit::Type r = c->getRelations(x,y);
    Gecode::IntRelType ir;

    switch(r)
    {
    case CSPLit::EQUAL:
        ir = IRT_EQ;
        break;
    case CSPLit::INEQUAL:
        ir = IRT_NQ;
        break;
    case CSPLit::LOWER:
        ir = IRT_LE;
        break;
    case CSPLit::LEQUAL:
        ir = IRT_LQ;
        break;
    case CSPLit::GEQUAL:
        ir = IRT_GQ;
        break;
    case CSPLit::GREATER:
        ir = IRT_GR;
        break;
    case CSPLit::ASSIGN:
    default:
        assert(false);
    }
    return LinRel(generateLinearExpr(x), ir, generateLinearExpr(y));
}

Gecode::BoolExpr GecodeSolver::SearchSpace::generateBooleanExpression(const Constraint* c)
{
    assert(c);
    if(c->isSimple())
    {
        return generateLinearRelation(c);
    }
    //else

    const Constraint* x;
    const Constraint* y;
    CSPLit::Type r = c->getConstraints(x,y);

    BoolExpr a = generateBooleanExpression(x);
    BoolExpr b = generateBooleanExpression(y);

    switch(r)
    {
    case CSPLit::AND: return a && b;
    case CSPLit::OR:  return a || b;
    case CSPLit::XOR: return a ^ b;
    case CSPLit::EQ:  return a == b;
    default: assert(false);
    }
}


Gecode::LinExpr GecodeSolver::SearchSpace::generateSum(std::vector<std::pair<GroundConstraint*,bool> >& vec)
{
    return generateSum(vec,0);
}

Gecode::LinExpr GecodeSolver::SearchSpace::generateSum( std::vector<std::pair<GroundConstraint*,bool> >& vec, size_t i)
{
    if (i==vec.size()-1)
    {
        if (vec[i].second)
            return generateLinearExpr(vec[i].first)*-1;
        else
            return generateLinearExpr(vec[i].first);
    }
    else
        if (vec[i].second)
            return generateLinearExpr(vec[i].first)*-1 + generateSum(vec,i+1);
        else
            return generateLinearExpr(vec[i].first) + generateSum(vec,i+1);


}

Gecode::LinExpr GecodeSolver::SearchSpace::generateLinearExpr(const GroundConstraint* c)
{
    if (c->isInteger())
    {
        return LinExpr(c->getInteger());
    }
    else
        if (c->isVariable())
        {
            return LinExpr(x_[csps_->getVariable(c->getString())]);
        }

    GroundConstraint::Operator op = c->getOperator();

    switch(op)
    {
    case GroundConstraint::TIMES:
        return generateLinearExpr(&(c->a_[0]))*generateLinearExpr(&(c->a_[1]));
    case GroundConstraint::MINUS:
        return generateLinearExpr(&(c->a_[0]))-generateLinearExpr(&(c->a_[1]));
    case GroundConstraint::PLUS:
        return generateLinearExpr(&(c->a_[0]))+generateLinearExpr(&(c->a_[1]));
    case GroundConstraint::DIVIDE:
        return generateLinearExpr(&(c->a_[0]))/generateLinearExpr(&(c->a_[1]));
    case GroundConstraint::ABS:
        return Gecode::abs(generateLinearExpr(&(c->a_[0])));
    case GroundConstraint::MIN:
    {
        IntVarArgs args(c->a_.size());
        for (size_t i = 0; i != c->a_.size();++i)
            args[i] = expr(*this,generateLinearExpr(&(c->a_[i])), ICL);
        return min(args);
    }
    case GroundConstraint::MAX:
    {
        IntVarArgs args(c->a_.size());
        for (size_t i = 0; i != c->a_.size();++i)
            args[i] = expr(*this,generateLinearExpr(&(c->a_[i])), ICL);
        return max(args);
    }
    }
    assert(false);

}


void GecodeSolver::SearchSpace::generateGlobalConstraint(LParseGlobalConstraintPrinter::GC& gc)
{

    if (gc.type_==DISTINCT)
    {
        IntVarArgs z(gc.heads_[0].size());

        for (size_t i = 0; i < gc.heads_[0].size(); ++i)
        {
            z[i] = expr(*this,generateLinearExpr(gc.heads_[0][i].a_.get()),ICL);
        }

        iva_ << z;

        Gecode::distinct(*this,z,ICL);
        return;

    }
    if (gc.type_==BINPACK)
    {
        IntVarArgs l(gc.heads_[0].size());
        for (size_t i = 0; i < gc.heads_[0].size(); ++i)
        {
            l[i] = expr(*this,generateLinearExpr(gc.heads_[0][i].a_.get()),ICL);
        }

        IntVarArgs b(gc.heads_[1].size());
        for (size_t i = 0; i < gc.heads_[1].size(); ++i)
        {
            b[i] = expr(*this,generateLinearExpr(gc.heads_[1][i].a_.get()),ICL);
        }

        IntArgs  s(gc.heads_[2].size());
        for (size_t i = 0; i < gc.heads_[2].size(); ++i)
        {


            if (!gc.heads_[2][i].a_->isInteger())
                throw ASPmCSPException("Third argument of binpacking constraint must be a list of integers.");
            s[i] = gc.heads_[2][i].a_->getInteger();
        }
        iva_ << l;
        iva_ << b;

        Gecode::binpacking(*this,l,b,s,ICL);
        return;

    }
    if (gc.type_==COUNT)
    {
        IntVarArgs a(gc.heads_[0].size());
        IntArgs c(gc.heads_[0].size());

        for (size_t i = 0; i < gc.heads_[0].size(); ++i)
        {
            a[i] = expr(*this,generateLinearExpr(gc.heads_[0][i].a_.get()),ICL);

            assert(gc.heads_[0][i].b_.isInteger());
            c[i] = gc.heads_[0][i].b_.getInteger();

        }

        IntRelType cmp;
        switch(gc.cmp_)
        {
        case CSPLit::ASSIGN:assert(false);
        case CSPLit::GREATER:cmp=IRT_GR; break;
        case CSPLit::LOWER:cmp=IRT_LE; break;
        case CSPLit::EQUAL:cmp=IRT_EQ; break;
        case CSPLit::GEQUAL:cmp=IRT_GQ; break;
        case CSPLit::LEQUAL:cmp=IRT_LQ; break;
        case CSPLit::INEQUAL:cmp=IRT_NQ; break;
        default: assert(false);
        }

        iva_ << a;
        IntVar temp(expr(*this,generateLinearExpr(gc.heads_[1][0].a_.get()),ICL));
        iva_ << temp;

        Gecode::count(*this,a,c,cmp,temp,ICL);
        return;

    }
    if (gc.type_==COUNT_UNIQUE)
    {
        IntVarArgs a(gc.heads_[0].size());

        for (size_t i = 0; i < gc.heads_[0].size(); ++i)
        {
            a[i] = expr(*this, generateLinearExpr(gc.heads_[0][i].a_.get()),ICL);
        }

        IntRelType cmp;
        switch(gc.cmp_)
        {
        case CSPLit::ASSIGN:assert(false);
        case CSPLit::GREATER:cmp=IRT_GR; break;
        case CSPLit::LOWER:cmp=IRT_LE; break;
        case CSPLit::EQUAL:cmp=IRT_EQ; break;
        case CSPLit::GEQUAL:cmp=IRT_GQ; break;
        case CSPLit::LEQUAL:cmp=IRT_LQ; break;
        case CSPLit::INEQUAL:cmp=IRT_NQ; break;
        default: assert(false);
        }

        iva_ << a;
        IntVar temp1(expr(*this, generateLinearExpr(&gc.heads_[0][0].b_),ICL));
        iva_ << temp1;
        IntVar temp2(expr(*this, generateLinearExpr(gc.heads_[1][0].a_.get()),ICL));
        iva_ << temp2;
        Gecode::count(*this,a,temp1,cmp,temp2,ICL);
        return;
    }
    if (gc.type_==COUNT_GLOBAL)
    {
        IntVarArgs a(gc.heads_[0].size());
        for (size_t i = 0; i < gc.heads_[0].size(); ++i)
        {
            a[i] = expr(*this, generateLinearExpr(gc.heads_[0][i].a_.get()),ICL);
        }

        IntVarArgs b(gc.heads_[1].size());
        IntArgs c(gc.heads_[0].size());
        for (size_t i = 0; i < gc.heads_[1].size(); ++i)
        {
            b[i] = expr(*this, generateLinearExpr(gc.heads_[1][i].a_.get()),ICL);

            assert(gc.heads_[1][i].b_.isInteger());
            c[i] = gc.heads_[1][i].b_.getInteger();
        }

        iva_ << a;
        iva_ << b;
        Gecode::count(*this,a,b,c,ICL);

        return;
    }
    if (gc.type_==MINIMIZE_SET || gc.type_==MINIMIZE)
    {
        IntVarArgs a(gc.heads_[0].size());
        for (size_t i = 0; i < gc.heads_[0].size(); ++i)
        {
            a[i] = expr(*this, generateLinearExpr(gc.heads_[0][i].a_.get()),ICL);
        }

        IntVarArgs b(gc.heads_[1].size());
        IntArgs c(gc.heads_[0].size());
        for (size_t i = 0; i < gc.heads_[1].size(); ++i)
        {
            b[i] = expr(*this, generateLinearExpr(gc.heads_[1][i].a_.get()),ICL);

            assert(gc.heads_[1][i].b_.isInteger());
            c[i] = gc.heads_[1][i].b_.getInteger();
        }

        iva_ << a;
        iva_ << b;
        Gecode::count(*this,a,b,c,ICL);

        return;
    }
    assert(false);

}



}//namespace

