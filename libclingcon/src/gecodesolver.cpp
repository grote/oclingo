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
#include <gecode/minimodel.hh>
#include <exception>
#include <sstream>
#include <clingcon/cspconstraint.h>
#include <gringo/litdep.h>
#include <gecode/kernel/wait.hh>
#include <clingcon/gecodeconflict.h>
#include <clingcon/gecodereason.h>
#include <clingcon/gecodereifwait.h>

//#define DEBUGTEXT

using namespace Gecode;
using namespace Clasp;
namespace Clingcon {


Gecode::IntConLevel ICL;
Gecode::IntVarBranch branchVar; //integer branch variable
Gecode::IntValBranch branchVal; //integer branch value

std::string GecodeSolver::num2name( unsigned int var)
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
    //if (optValues.size()>0) ++optValues.back(); // last element must also be found

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

unsigned int GecodeSolver::assLength(unsigned int index) const
{
    assert(index<assLength_.size());
        return assLength_[index];
}

GecodeSolver::SearchSpace* GecodeSolver::getCurrentSpace()
{
    //return spaces_.back();//currentSpace_;
    size_t i = spaces_.size()-2;

 //   Ich bekomme unterschiedliche Ergebnisse (Anzahl der Choices) bei deepCopy 1 und 5,
    if (spaces_.back()!=0)
    {
        return spaces_.back();
    }
    while(i>=0)
    {

        if (spaces_[i]!=0)
        {
            //assert(assLength_[i]<=propagated_);
            //if (assLength_[i]<propagated_)
            //assert(i!=0);
            //assert(propagated_==assLength_.back()); // at least for the non delay case
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

    //if (domain_.left == std::numeric_limits<int>::min())
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

        /*
        if (s_->isTrue(Literal(newVar,false)))
            std::cout << "t ";
        if (s_->isTrue(Literal(newVar,true)))
            std::cout << "f ";
        if (s_->isFalse(Literal(newVar,false)))
            std::cout << "f ";
        if (s_->isFalse(Literal(newVar,true)))
            std::cout << "t ";
        std::cout << num2name(newVar) << std::endl;

*/

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

        //newConstraints.insert(std::make_pair(newVar, i->second));
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

    //TODO: 1. check if already failed through initialitation
    //for (GecodeSolver::ConstraintMap::iterator i = constraints_.begin(); i != constraints_.end(); ++i)
    //    delete i->second;
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
                                    //erivedLits_.clear();
                                    //propQueue_.push_back(negLit(0));
                                    return false;
                                }

                                //-x \/ y
                                /*
                                Gecode::BoolVarArg x;
                                if (test.sign()) //negative -> x
                                    x = !(currentSpace_->b_[varToIndex(test.var())]);
                                else
                                    x = currentSpace_->b_[varToIndex(test.var())];

                                Gecode::BoolExpr y;
                                if(j->sign()) //negative -> -y
                                    y = !(currentSpace_->b_[varToIndex(j->var())]);
                                else
                                    y = currentSpace_->b_[varToIndex(j->var())];

                                rel(*currentSpace_, !x || y, ICL);

                                */

                                /*
                                if (test.sign()) //negative -> x
                                    if(j->sign()) //negative -> -y
                                        rel(*currentSpace_, (currentSpace_->b_[varToIndex(test.var())]) || !(currentSpace_->b_[varToIndex(j->var())]), ICL);
                                    else
                                        rel(*currentSpace_, (currentSpace_->b_[varToIndex(test.var())]) || (currentSpace_->b_[varToIndex(j->var())]), ICL);
                                else
                                    if(j->sign())
                                        rel(*currentSpace_, !(currentSpace_->b_[varToIndex(test.var())]) || !(currentSpace_->b_[varToIndex(j->var())]), ICL);
                                    else
                                        rel(*currentSpace_, !(currentSpace_->b_[varToIndex(test.var())]) || (currentSpace_->b_[varToIndex(j->var())]), ICL);
                                */

                             }
                         }
                        delete tester;
                        test=~test;
                    }
                }
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
        derivedLits_.push_back(lit);
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
            //ret[i->first] = i->second;
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
    //assert(currentSpace_->stable());


    unsigned int oldDL = s_->decisionLevel();
    if (!finishPropagation())
    {
        return false;
    }
    if (oldDL > s_->decisionLevel()) //we backjumped
    {
        return false;
    }


    // currently using a depth first search, this could be changed by options
    if (!optimize_)
        dfsSearchEngine_ = new DFS<GecodeSolver::SearchSpace>(getCurrentSpace(), searchOptions_);
    else
        babSearchEngine_ = new BAB<GecodeSolver::SearchSpace>(getCurrentSpace(), searchOptions_);
#ifdef DEBUGTEXT
    //std::cout << "created new SearchEngine_:" << searchEngine_ << std::endl;
    currentSpace_->print(variables_);std::cout << std::endl;
#endif
    if (enumerator_)
    {
        delete enumerator_;
        enumerator_ = 0;
    }
    if (!optimize_)
        enumerator_ = dfsSearchEngine_->next();
    else
        enumerator_ = babSearchEngine_->next();
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
    //assert(conflict.size()==0 || conflict.size() == assLength(spaces_.size()-1) || conflict.size() == assLength(spaces_.size()-1) +1);
    if (shrink)
        conflictAnalyzer_->shrink(conflict, last);


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

    uint32 oldDL = s_->decisionLevel();
    if (!propagateOldLits())
        return false;
    if (oldDL > s_->decisionLevel()) // we backjumped
    {
        propQueue_.clear();
        return true;
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
            clits.clear();
            clits.assign(assignment_.begin(), assignment_.begin()+propagated_/*assLength_[spaces_.size()-1]*/);
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
            return false;
        }
    }
    return true;
}

bool GecodeSolver::propagateOldLits()
{
    for (std::map<size_t,ImplList>::reverse_iterator i = impliedLits_.rbegin(); i != impliedLits_.rend(); ++i)
    {
        if (i->first > s_->decisionLevel())
        {
            ClauseCreator gc(s_);
            // we backjumped and have something todo
            ImplList::iterator j = i->second.begin();
            while(j != i->second.end())
            {
                if (s_->value(j->x_.var())==value_free)
                {
                    if (j->level_ > s_->decisionLevel())
                    {
                        j = i->second.erase(j); // we can not imply this here, we backjumped too far
                        continue;
                    }
                    uint32 oldDL = s_->decisionLevel();
                    if (lazyLearn_)
                    {

                        litToAssPosition_[j->x_] = j->reasonLength_;
                        if (!s_->addNewImplication(j->x_,j->level_,&dummyReason_))
                        {
                            i->second.erase(j);
                            return false;
                        }

                    }
                    else // early learn
                    {
                        //std::cout << "unassigned " << j->x_.var() << " now becomes true with asl " << j->reasonLength_ << std::endl;
                        Clasp::LitVec reason;
                        createReason(reason,j->x_,assignment_.begin(), assignment_.begin()+j->reasonLength_);
                        gc.startAsserting(Constraint_t::learnt_conflict, j->x_);
                        for (Clasp::LitVec::const_iterator r = reason.begin(); r != reason.end(); ++r)
                        {
                            assert(s_->isTrue(*r));
                            gc.add(~(*r));
                        }
                        if(!gc.end())
                        {
                            i->second.erase(j);
                            return false;
                        }

                    }

                    j = i->second.erase(j);
                    if (oldDL > s_->decisionLevel())
                    {
                        // we backjumped, restart
                        j = i->second.begin();
                    }
                    continue;
                }
                else // some value
                {
                    if (s_->decisionLevel() > j->level_)
                    {
                        if (s_->isTrue(j->x_))
                        {

                            // the literal is true again but on a lower level, but still higher than ours
                            if (s_->level(j->x_.var()) > j->level_)
                            {
                                //reinsert it on a new position
                                //std::cout << "reinsert " << j->x_.var() << std::endl;
                                impliedLits_[s_->level(j->x_.var())].push_back(ImpliedLiteral(j->x_, j->level_, j->reasonLength_));
                            }
                            //else
                            //{
                            //    j = i->second.remove(j); // we can not imply this here, we backjumped too far
                            //    continue;
                            //}
                            j = i->second.erase(j);
                            continue;
                        }
                        else
                        {
                            assert(s_->isFalse(j->x_));
                            // we have a conflict ?
                            if (s_->level(j->x_.var())<j->level_)
                            {// we backjumped too far, continue
                                j = i->second.erase(j);
                                continue;
                            }


                            //std::cout << "Found Conflict " << j->x_.var() << " with asl " << j->reasonLength_ << std::endl;
                            Clasp::LitVec conflict(assignment_.begin(), assignment_.begin()+j->reasonLength_);
                            conflict.push_back(~(j->x_));

                            for (Clasp::LitVec::const_iterator k = conflict.begin(); k != conflict.end(); ++k)
                            {
                                assert(s_->isTrue(*k));
                            }
                            setConflict(conflict, true);
                            i->second.erase(j);
                            return false;
                        }
                    }
                    else
                    {
                        j = i->second.erase(j); // we can not imply this here, we backjumped too far
                        continue;
                    }

                }
                assert(true);

            }
        }else break;
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
        // currentSpace_->status() == FAILED
        else
        {
            // could save a bit of conflict size if conflict resultet from propagate function, which does singular propagation
            //clits.clear();
            //clits.reserve(assignment_.size()+clits.size());
            //clits.insert(clits.begin(), assignment_.begin(), assignment_.end()); // this is the complete conflict
            //on decision level 0 we only can have root conflict
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

        unsigned int oldDL = s_->decisionLevel();
        if (!finishPropagation())
            return false;
        if (oldDL > s_->decisionLevel()) // we backjumped!
        {
            return true;
        }

        assignment_.insert(assignment_.end(), clits.begin(), clits.end());


        if (s_->decisionLevel() != currentDL())
        {
            if (deepCopy_) ++deepCopyCounter_;
            if(deepCopyCounter_==deepCopy_ || spaces_.size()==1)
            {
                deepCopyCounter_=0;
                spaces_.push_back(static_cast<SearchSpace*>(getCurrentSpace()->clone()));
                //currentSpace_ = spaces_.back();
            }
            else
            {
                SearchSpace* t = spaces_.back();
                spaces_.back()=0;
                spaces_.push_back(t);
                getCurrentSpace(); //debug
            }
            //register in solver for undo event
            s_->addUndoWatch(s_->decisionLevel(),clingconPropagator_);
            // this is important to first do propagation and then set assLength, because getCurrentSpace CAN
            // redo propagation
            getCurrentSpace()->propagate(clits.begin(), clits.end());
            dl_.push_back(s_->decisionLevel());
            assLength_.push_back(assignment_.size());
        }
        else
        {
            getCurrentSpace()->propagate(clits.begin(), clits.end());
        }


        propagated_=assignment_.size();
        assLength_.back()=propagated_;
        if (!getCurrentSpace()->failed() && getCurrentSpace()->status() != SS_FAILED)
        {
            //if (dynamicProp_) {++temp;}
            // this function avoids propagating already decided literals
            unsigned int oldDL = s_->decisionLevel();
            if(!propagateNewLiteralsToClasp(spaces_.size()-1))
                return false;
            if (oldDL > s_->decisionLevel()) //we backjumped
                return true;
        }
        // currentSpace_->status() == FAILED
        else
        {
            //assignment already has clits included!
            clits.clear();
            clits.insert(clits.begin(), assignment_.begin(), assignment_.begin() + propagated_);
            setConflict(clits, false);
            return false;
        }
    }
    else
    {
        assignment_.insert(assignment_.end(), clits.begin(), clits.end());
        if (s_->decisionLevel() != currentDL())
        {
            dl_.push_back(s_->decisionLevel());
            assLength_.push_back(assignment_.size());
            //register in solver for undo event
            s_->addUndoWatch(s_->decisionLevel(),clingconPropagator_);
        }
        else
        {
            assLength_.back() = assignment_.size();
        }

    }

    return true;
}


bool GecodeSolver::finishPropagation()
{
    //while(spaces_.size() <= dl_.size())
    while(propagated_ < assLength_.back())
    {
        unsigned int last = spaces_.size()-1;    // the last space index
        unsigned int next = last+1 == assLength_.size() ? last : last+1; // the index before


        unsigned int start = assLength_[last];
        unsigned int end = assLength_[next];
        if (start > propagated_) // redo missing propagation for current space
        {
            end =  start;
            start = propagated_;
        }
        else
        {
            if (deepCopy_) ++deepCopyCounter_;
            if(deepCopyCounter_==deepCopy_ || spaces_.size()==1)
            {
                deepCopyCounter_=0;
                spaces_.push_back(static_cast<SearchSpace*>(getCurrentSpace()->clone()));
                //currentSpace_ = spaces_.back();
            }
            else
            {
                SearchSpace* t = spaces_.back();
                spaces_.back()=0;
                spaces_.push_back(t);
            }
        }


        //currentSpace_ = spaces_.back();
        derivedLits_.clear();

        getCurrentSpace()->propagate(assignment_.begin()+start, assignment_.begin()+end);
        if (end>propagated_)
            propagated_=end;
        if (!getCurrentSpace()->failed() && getCurrentSpace()->status() != SS_FAILED)
        {
            //if (dynamicProp_) {++temp;}
            // this function avoids propagating already decided literals
            unsigned int oldDL = s_->decisionLevel();
            if(!propagateNewLiteralsToClasp(spaces_.size()-1))
            {
                //undo the last space as it might not be fully propagated to clasp
                delete spaces_.back();
                spaces_.pop_back();
                // go back to last real space
                //currentSpace_ = spaces_.back(); // might be zero
                propagated_ = assLength_[spaces_.size()-1];// we did not do this propagation
                return false;
            }
            if (oldDL > s_->decisionLevel()) // we backjumped!
            {
                return true;
            }
        }
        // currentSpace_->status() == FAILED
        else
        {
            //nur alles was currentSpace bisher mitbekommen hat in den Konflikt einbringen !!!
            Clasp::LitVec ret(assignment_.begin(), assignment_.begin()+end);    // this is the old conflict
            setConflict(ret, false);
            return false;
        }
    }
    return true;
}


 bool GecodeSolver::propagateMinimize()
 {
     impliedLits_.clear(); // dont know if this cant be done more clever
     //Clasp::LitVec oldDerivedLits;
     //oldDerivedLits.swap(derivedLits_); // maybe something was already propagated
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
                 Clasp::LitVec conflict(assignment_.begin(), assignment_.begin()+ (i==spaces_.size()-1 ? propagated_  : assLength_[i] ));
                 propQueue_.clear();
                 setConflict(conflict, false);
                 return false;
             }
             else
             {
                 if (derivedLits_.size()>0)
                 {
                     unsigned int oldDL = s_->decisionLevel();
                     if (!propagateNewLiteralsToClasp(i))
                         return false;
                     //if (oldDL > s_->decisionLevel()) //we backjumped
                     //    return true;
                     if (i+1==spaces_.size() || dl_[i+1] > s_->decisionLevel())
                        break;
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


    dl_.pop_back();
    assLength_.pop_back();
    assignment_.resize(assLength_.back());

    if (assignment_.size()<propagated_) // if assignment < propagated we have to backtrack, otherwise not
        propagated_=assignment_.size();
    if(spaces_.size()>dl_.size())
    {
        delete spaces_.back();
        spaces_.pop_back();
    }
    //currentSpace_ = spaces_.back();
    return;
}

void GecodeSolver::printStatistics()
{
    conflictAnalyzer_->printStatistics();
    reasonAnalyzer_->printStatistics();
}


bool GecodeSolver::propagateNewLiteralsToClasp(size_t level)
{
    bool back=false;
    unsigned int size = (level == dl_.size()-1 ? propagated_ : assLength_[level]);
    for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
    {
        if (s_->isFalse(*i))
        {
            Clasp::Literal conf(~(*i));

            derivedLits_.clear();
            derivedLits_.reserve(size+1);
            derivedLits_.insert(derivedLits_.begin(), assignment_.begin(), assignment_.begin()+size);
            derivedLits_.push_back(conf);// also add the conflicting literal, this can be on a higher decision level
            for (Clasp::LitVec::const_iterator j = derivedLits_.begin(); j != derivedLits_.end(); ++j)
            {
                assert(s_->isTrue(*j));
            }
            setConflict(derivedLits_, true);
            return false;
        }
        if (!s_->isTrue(*i))  // if the literal is not already true
            back=true;
    }

    //if (!back)
    //   return true;

    if (!back) // there where no undef literals, so we do not want to backtrack
               // just remember all better reasons
    {
        for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
        {

            assert(s_->isTrue(*i));
            size_t oldImpl = s_->level(i->var());
            if (oldImpl <= dl_[level])
                continue;
            /*std::map<size_t, ImplList>::iterator m = impliedLits_.find(oldImpl);
            if (m == impliedLits_.end())
            {
                m = (impliedLits_.insert(std::make_pair(oldImpl,ImplList()))).first;
            }*/
            impliedLits_[oldImpl].push_back(ImpliedLiteral(*i,dl_[level],size));


        }

        return true;
    }

    if (lazyLearn_)
    {

        // derive undef and true lits because we are better
        assert(spaces_.size());
        bool first = true;
        while(true)
        {
            for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
            {
                if (s_->isTrue(*i) && s_->level(i->var())<=dl_[level])
                    continue; // do not derive literals that we are not the cause of

                litToAssPosition_[(*i)] = size;
                if (!s_->addNewImplication(*i,dl_[level]/*s_->decisionLevel()*/,&dummyReason_))
                {
                    derivedLits_.clear();
                    return false;
                }

                if (s_->value(i->var())==value_free && s_->decisionLevel() > dl_[level])
                    break;
            }
            if (!first)
                break;
            first = false;
        }
        derivedLits_.clear();
        return true;
    }
    else
    {
        uint32 implyon = dl_[level];
        bool first = true;
        ClauseCreator gc(s_);
        while(true)
        {
        for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
        {
            if (s_->value(i->var())==value_free) // propagate the yet undeffed ones
            {
                //uint32 dl = s_->decisionLevel();
                Clasp::LitVec reason;
                createReason(reason,*i,assignment_.begin(), assignment_.begin()+size);
                //std::cout << "Start asserting " << i->sign() << " " << i->var() << std::endl;
                gc.startAsserting(Constraint_t::learnt_conflict, *i);
                for (Clasp::LitVec::const_iterator r = reason.begin(); r != reason.end(); ++r)
                {
                    //std::cout << s_->isTrue(*r) << " level:" << s_->level(r->var()) << " var:" << r->var() << " ... ";
                    assert(s_->isTrue(*r));
                    gc.add(~(*r));
                }
                if(!gc.end())
                {
                    derivedLits_.clear();
                    return false;
                }
                //std::cout << std::endl;
                assert(s_->isTrue(*i));

                if (s_->decisionLevel() < implyon) // we backjumped too far, the propagation is not valid anymore
                {
                    derivedLits_.clear();
                    return true;
                }
                if (first)
                {
                    break;
                }
            }


/*
            //if (!s_->isTrue(*i))
            {


                uint32 max = 0;
                for (Clasp::LitVec::const_iterator j = reason.begin(); j != reason.end(); ++j)
                {
                     max = s_->level(j->var()) > max ? s_->level(j->var()) : max;
                }

                //if (s_->isTrue(*i) && s_->level(i->var())<=max)
                //    continue;

                if (dl_[level]==max)
                    gc.startAsserting(Constraint_t::learnt_conflict, *i);
                else
                {
                    gc.start(Constraint_t::learnt_other);
                    gc.add(*i);
                }

                for (Clasp::LitVec::const_iterator r = reason.begin(); r != reason.end(); ++r)
                {
                    gc.add(~(*r));
                }
                if(!gc.end())
                {
                    derivedLits_.clear();
                    return false;
                }

                if (dl>s_->decisionLevel())
                {
                    // we backjumped
                    if (dl_[level]==s_->decisionLevel())
                    {
                        // we just applied it on the implication level and can go on
                        //int a = 0;
                    }
                    else
                    {
                        // we applied it on an even lower level due to shrinking or late propagation
                        // so all further implications are not relevant any more
                        derivedLits_.clear();
                        return true;
                        //break;
                    }
                    //%we backjumped, so the derived literals are maybe no longer derived !
                    //%if they are derived on the last level where we have a space/propagated this is ok,
                    //%        the others can be derived there true, otherwise we have to break
                //    assert(false && "Shouldn't occur here");
                }
            }
            //else
            {

            }*/
        }
        if (!first)
            break;
        first = false;
        }
        derivedLits_.clear();
        return true;
    }
    assert(false);
}

void GecodeSolver::createReason(Clasp::LitVec& reason, const Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end)
{
    assert(reason.size()==0);
    reasonAnalyzer_->generate(reason, l, begin, end);
}

Clasp::ConstraintType GecodeSolver::CSPDummy::reason(const Literal& l, Clasp::LitVec& reason)
{
    assert(gecode_->s_->isTrue(l));

    gecode_->createReason(reason, l, gecode_->assignment_.begin(), gecode_->assignment_.begin()+gecode_->litToAssPosition_[l]);
    return Clasp::Constraint_t::learnt_other;
}

GecodeSolver* GecodeSolver::SearchSpace::csps_ = 0;

IntVarArgs GecodeSolver::SearchSpace::iva_;


GecodeSolver::SearchSpace::SearchSpace(GecodeSolver* csps, unsigned int numVar, GecodeSolver::ConstraintMap& constraints,
                                       LParseGlobalConstraintPrinter::GCvec& gcvec) : Space(),
    x_(*this, numVar),
    //b_(*this, constraints.size(), 0, 1),
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
        //std::cout << csps_->domains_[i] << " " << std::endl;

        //std::cout << csps_->domains_[i].size() << " " << std::endl;
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
        //std::cout << is << " " << std::endl;

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
            //std::cout << "Constraint " << csps_->num2name(i->first) << " is static" << std::endl;
            //generateConstraint(i->second,csps_->getSolver()->value(i->first) == value_true);
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
             //reifwait(*this,csps_,b_[counter], csps_->indexToLit(counter).var(), ICL);
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
        std::pair<unsigned int, bool> index(csps_->litToIndex(Clasp::Literal::fromIndex(i->index())));

        Int::BoolView bv(b_[index.first]);
        if (index.second)
        {
            if(Gecode::Int::ME_BOOL_FAILED == bv.one(*this))
            {
                fail();
                return;
            }
        }
        else
        {
            if(Gecode::Int::ME_BOOL_FAILED == bv.zero(*this))
            {
                fail();
                return;
            }
        }
    }
}

void GecodeSolver::SearchSpace::propagate(const Clasp::Literal& i)
{
    std::pair<unsigned int, bool> index(csps_->litToIndex(Clasp::Literal::fromIndex(i.index())));

    Int::BoolView bv(b_[index.first]);
    if (index.second)
    {
        if(Gecode::Int::ME_BOOL_FAILED == bv.one(*this))
        {
            fail();
        }
    }
    else
    {
        if(Gecode::Int::ME_BOOL_FAILED == bv.zero(*this))
        {
            fail();
        }
    }
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

