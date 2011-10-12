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

GecodeSolver::GecodeSolver(bool lazyLearn, bool useCDG, bool weakAS, int numAS,
                           const std::string& ICLString, const std::string& branchVarString,
                           const std::string& branchValString, std::vector<int> optValueVec,
                           bool optAllPar, bool initialLookahead, const std::string& reduceReason, const std::string& reduceConflict) :
    currentSpace_(0), lazyLearn_(lazyLearn),
    useCDG_(useCDG), weakAS_(weakAS), numAS_(numAS), enumerator_(0), dfsSearchEngine_(0), babSearchEngine_(0),
    dummyReason_(this), updateOpt_(false), conflictAnalyzer_(0), reasonAnalyzer_(0), recording_(true),
    initialLookahead_(initialLookahead)
{
    optValues.insert(optValues.end(),optValueVec.begin(), optValueVec.end());
    if (optValues.size()>0) ++optValues.back(); // last element must also be found

    optAll=optAllPar;

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
    if (reduceReason == "linear")         reduceReason_ = LINEAR;
    if (reduceReason == "linear-fwd")     reduceReason_ = LINEAR_FWD;
    if (reduceReason == "linear-grouped") reduceReason_ = LINEAR_GROUPED;
    if (reduceReason == "scc")            reduceReason_ = SCC;
    if (reduceReason == "log")            reduceReason_ = LOG;
    if (reduceReason == "range")          reduceReason_ = RANGE;
    if (reduceReason == "sccrange")       reduceReason_ = SCCRANGE;


    if (reduceConflict == "simple")         reduceConflict_ = SIMPLE;
    if (reduceConflict == "linear")         reduceConflict_ = LINEAR;
    if (reduceConflict == "linear-fwd")     reduceConflict_ = LINEAR_FWD;
    if (reduceConflict == "linear-grouped") reduceConflict_ = LINEAR_GROUPED;
    if (reduceConflict == "log")            reduceConflict_ = LOG;
    if (reduceConflict == "scc")            reduceConflict_ = SCC;
    if (reduceConflict == "range")          reduceConflict_ = RANGE;
    if (reduceConflict == "sccrange")       reduceConflict_ = SCCRANGE;

}

GecodeSolver::~GecodeSolver()
{

    for (std::vector<SearchSpace*>::iterator i = spaces_.begin(); i != spaces_.end(); ++i)
        delete *i;

    // if we have not done initialization
    //for (std::map<int, Constraint*>::iterator i = constraints_.begin(); i != constraints_.end(); ++i)
    //    delete i->second;
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

/*
    if (l.sign())
        std::cout << "F ";
    else
        std::cout << "T ";
    std::cout << num2name(l.var()) << std::endl;
*/

    propQueue_.push_back(l);
    //propQueue_.back().clearWatch();

}

void GecodeSolver::reset()
{
    propQueue_.clear();
}

void GecodeSolver::addVarToIndex(unsigned int var, unsigned int index)
{
    varToIndex_[var] = index;
    indexToVar_[index] = var;
}

unsigned int GecodeSolver::currentDL() const
{
    assert(dl_.size());
    return dl_.back();
}

unsigned int GecodeSolver::assLength(unsigned int index) const
{
    if (index < assLength_.size())
        return assLength_[index];
    else
        return assignment_.size();
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
    //for (GecodeSolver::ConstraintMap::iterator i = constraints_.begin(); i != constraints_.end(); ++i)
    GecodeSolver::ConstraintMap::iterator i = constraints_.begin();
    while(i != constraints_.end())
    {
        //std::cout << i->first << std::endl;
        //int newVar = s_->strategies().symTab->find(i->first)->lit.var();
        //begin = std::lower_bound(begin, s_->strategies().symTab->end(),i->first);
        begin = s_->strategies().symTab->lower_bound(begin, i->first);

        int newVar = begin->second.lit.var();


        // convert uids to solver literal ids 
        //std::cout << num2name(newVar) << std::endl;



        i->second->registerAllVariables(this);

        //guess domains of already decided constraints
        if (s_->isTrue(posLit(newVar)))
        {
            guessDomains(i->second, true);
        }
        else
        if (s_->isFalse(posLit(newVar)))
        {
            guessDomains(i->second, false);
        }
        else
        {
            s_->addWatch(posLit(newVar), clingconPropagator_,0);
            s_->addWatch(negLit(newVar), clingconPropagator_,0);
        }

        //newConstraints.insert(std::make_pair(newVar, i->second));
        newConstraints.insert(newVar, constraints_.release(i).release());
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
    currentSpace_ = new SearchSpace(this, variables_.size(), constraints_, globalConstraints_);
    spaces_.push_back(currentSpace_);
    dl_.push_back(0);



    switch(reduceConflict_)
    {
        case SIMPLE:         conflictAnalyzer_ = new SimpleCA(); break;
        case LINEAR:         conflictAnalyzer_ = new Linear2IISCA(this); break;
        case LINEAR_FWD:     conflictAnalyzer_ = new FwdLinear2IISCA(this); break;
        case LINEAR_GROUPED: conflictAnalyzer_ = new Linear2GroupedIISCA(this); break;
        case SCC:            conflictAnalyzer_ = new SCCIISCA(this); break; break;
        case LOG:            conflictAnalyzer_ = new ExpIISCA(this); break;
        case RANGE:          conflictAnalyzer_ = new RangeCA(this); break;
        case SCCRANGE:       conflictAnalyzer_ = new SCCRangeCA(this); break;
        default: assert(false);
    };

    switch(reduceReason_)
    {
        case SIMPLE:         reasonAnalyzer_ = new SimpleRA(); break;
        case LINEAR:         reasonAnalyzer_ = new Linear2IRSRA(this); break;
        case LINEAR_FWD:     reasonAnalyzer_ = new FwdLinear2IRSRA(this); break;
        case LINEAR_GROUPED: reasonAnalyzer_ = new Linear2GroupedIRSRA(this); break;
        case SCC:            reasonAnalyzer_ = new SCCIRSRA(this); break;
        case LOG:            reasonAnalyzer_ = new ExpIRSRA(this); break;
        case RANGE:          reasonAnalyzer_ = new RangeIRSRA(this); break;
        case SCCRANGE:       reasonAnalyzer_ = new SCCRangeRA(this); break;
        default: assert(false);
    };

/*
    //conflictAnalyzer_ = new UnionIISCA(this);
    //conflictAnalyzer_ = new FwdLinearIISCA(this);
    //conflictAnalyzer_ = new ExpIISCA(this);
    //conflictAnalyzer_ = new LinearIISCA(this);
    conflictAnalyzer_ = new SimpleCA();

    //conflictAnalyzer_ = new RangeCA(this);
    //conflictAnalyzer_ = new RangeLinearCA(this);

    //reasonAnalyzer_ = new Union2IRSRA(this);//faster
    //reasonAnalyzer_ = new UnionIRSRA(this);
    //reasonAnalyzer_ = new SCCIRSRA(this);
    //reasonAnalyzer_ = new FwdLinearIRSRA(this);
    //reasonAnalyzer_ = new ExpIRSRA(this);
    //reasonAnalyzer_ = new LinearIRSRA(this);
    reasonAnalyzer_ = new SimpleRA();

    //reasonAnalyzer_ = new Approx1IRSRA(this);

    //reasonAnalyzer_ = new RangeIRSRA(this);
    //reasonAnalyzer_ = new RangeLinearIRSRA(this);
    //reasonAnalyzer_ = new Linear2IRSRA(this);
    //reasonAnalyzer_ = new Linear2GroupedIRSRA(this);

    //reasonAnalyzer_ = new FwdLinear2IRSRA(this);
*/

    //TODO: 1. check if already failed through initialitation
    //for (GecodeSolver::ConstraintMap::iterator i = constraints_.begin(); i != constraints_.end(); ++i)
    //    delete i->second;
    constraints_.clear();
    globalConstraints_.clear();

    //std::cout << "status" << std::endl;
    if(!currentSpace_->failed() && currentSpace_->updateOptValues() && !currentSpace_->failed() && currentSpace_->status() != SS_FAILED)
    {
        // std::cout << "done" << std::endl;
        if (!propagateNewLiteralsToClasp())
        {
            propQueue_.push_back(negLit(0));
            return false;
        }
        else
        {
            if (initialLookahead_)
            {
                //initial lookahead

                for (std::map<unsigned int, unsigned int>::const_iterator i = varToIndex_.begin(); i != varToIndex_.end(); ++i)
                {
                    Clasp::Literal test(i->first,false);

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
                            currentSpace_->propagate(~test);
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
                        test=Literal(i->first,true);
                    }
                }
            }
            //std::cout << "initialized" << std::endl;
            variableMap_.clear();
            //currentSpace_->print(getVariables());
            return true;
        }
    }
    else
    {
        //std::cout << "initialized" << std::endl;
        variableMap_.clear();
        propQueue_.push_back(negLit(0));
        return false;
    }
}


void GecodeSolver::newlyDerived(int index)
{
    if (recording_)
    {
        if (index>0)
        {
            derivedLits_.push_back(Literal(indexToVar(index-1),false));
        }
        else
        {
            derivedLits_.push_back(Literal(indexToVar(-(index+1)),true));
        }
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

    //else
    //std::vector<unsigned int> vec;
    //c->getAllVariables(vec, this);
    //unary constraint
    //if (vec.size()==1 )
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
            //
            //domain.second =
            //GecodeSolver::Domain inter = i->second;
            //inter.intersect(found->second);
            //ret[i->first]=inter;
            last = found;
        }
    }

    /*
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
    */
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
    // currently using a depth first search, this could be changed by options
    if (!optimize_)
        dfsSearchEngine_ = new DFS<GecodeSolver::SearchSpace>(currentSpace_, searchOptions_);
    else
        babSearchEngine_ = new BAB<GecodeSolver::SearchSpace>(currentSpace_, searchOptions_);
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
        setConflict(assignment_);
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
    //if (spaces_.size())
        return (spaces_[0]->failed() ? 0 : static_cast<GecodeSolver::SearchSpace*>(spaces_[0]->clone()));
    //else
     //   return (currentSpace_->failed() ? 0 : static_cast<GecodeSolver::SearchSpace*>(currentSpace_->clone()));
}

void GecodeSolver::setRecording(bool r)
{
    recording_ = r;
}

void GecodeSolver::setConflict(Clasp::LitVec conflict)
{
    conflictAnalyzer_->shrink(conflict);

    unsigned int maxDL = 0;
    for (Clasp::LitVec::const_iterator j = conflict.begin(); j != conflict.end(); ++j)
        maxDL = std::max(s_->level(j->var()), maxDL);
    s_->setBacktrackLevel(std::min(maxDL,s_->backtrackLevel()));
    if (maxDL < s_->decisionLevel())
        s_->undoUntil(maxDL);
    if (conflict.size()==0) // if root level conflict due to global constraints
        conflict.push_back(posLit(0));
    s_->setConflict(conflict);
    return;
}


unsigned int GecodeSolver::varToIndex(unsigned int var)
{
    return varToIndex_[var];
}

unsigned int GecodeSolver::indexToVar(unsigned int index)
{
    //if (indexToVar_.find(index) == indexToVar_.end())
    //    int error=5;
    return indexToVar_[index];
}


void GecodeSolver::printAnswer()
{
    assert(enumerator_);
    assert(currentSpace_);
    if (weakAS_)
        currentSpace_->print(variables_);
    else
        enumerator_->print(variables_);
}

bool GecodeSolver::propagate()
{

    if (updateOpt_)
    {
        return propagateMinimize();
    }


    /*
    std::cout << "Assignment:@" << currentDL() << std::endl;
    for (Clasp::LitVec::const_iterator i = assignment_.begin(); i != assignment_.end(); ++i)
    {
        if (i->sign())
            std::cout << "neg ";
        else
            std::cout << "pos ";
        std::cout << num2name(i->var()) << " ";
    }
    std::cout << std::endl;
*/
    // if already failed, create conflict, this may be on a lower level
    if (currentSpace_->failed())
    {
        setConflict(assignment_);
        return false;
    }
    else // everything is fine
    {
        /*
        // if we need to update the minimize function, do this
        if (updateOpt_ && GecodeSolver::optValues.size()>0)
        { 
            // this is still the old space, no new knowledge has been integrated
            derivedLits_.clear();
            currentSpace_->updateOptValues();
            updateOpt_=false;

            if (!currentSpace_->failed() && currentSpace_->status() != SS_FAILED)
            {
                // there was a problem due to the update function
                if (derivedLits_.size()>0)
                {
                    uint32 dl = s_->decisionLevel();
                    if (!propagateNewLiteralsToClasp())
                        return false;
                    // if we backtracked during assigning, clear propQueue
                    if (dl>s_->decisionLevel())
                        propQueue_.clear();
                }
            }
            else
            {
                setConflict(assignment_);
                return false;
            }
        }*/

        // no updateOpt or no problem with this

        // remove already assigned literals, this happens if we have propagated a new literal to clasp
        Clasp::LitVec clits;
        for (Clasp::LitVec::const_iterator i = propQueue_.begin(); i != propQueue_.end(); ++i)
        {
            //assert(s_->isTrue(*i));
            //if (!s_->isTrue(*i))
            //{
                //this literal was once propagated but then backtracked without any notice of me, so we ignore it
            //    continue;
            //}
            // add if free
            // conflict if contradicting
            GecodeSolver::SearchSpace::Value constr = currentSpace_->getValueOfConstraint(i->var());
            if ( constr == SearchSpace::BFREE)
                clits.push_back(*i);
            if (( constr == SearchSpace::BFALSE && i->sign()==false ) ||
                ( constr == SearchSpace::BTRUE  && i->sign()==true  )
                )
            {
                //std::cout << "Conflicting due to wrong assignment of clasp variable" << std::endl;
                setConflict(assignment_);
                return false;
            }
        }
        propQueue_.clear();

        // we have something to propagate
        if (clits.size())
        {
            // if we have a new decision level, create a new space
            if (s_->decisionLevel() > currentDL())
            {
                //register in solver for undo event
                assert(s_->decisionLevel()!=0);
                s_->addUndoWatch(s_->decisionLevel(),clingconPropagator_);

                //std::cout << "store a new space" << std::endl;
                // store old space
                /*if (dl_.size() && currentDL_ == dl_.back())
                {
                    delete spaces_.back();
                    spaces_.back() = static_cast<SearchSpace*>(currentSpace_->clone());
                    assLength_.back() = assignment_.size();
                    propsPerLevel_.back() = propsPerLevelCounter_;
                }
                else*/
                //{
                // this will be the new space and dl
                spaces_.push_back(static_cast<SearchSpace*>(currentSpace_->clone()));
                dl_.push_back(s_->decisionLevel());
                currentSpace_ = spaces_.back();

                //this is for the old one
                assLength_.push_back(assignment_.size());
                //}

            }

            derivedLits_.clear();
            currentSpace_->propagate(clits.begin(),clits.end()); // can lead to failed space

            if (!currentSpace_->failed() && currentSpace_->status() != SS_FAILED)
            {
                assignment_.insert(assignment_.end(), clits.begin(), clits.end());

                // this function avoids propagating already decided literals
                if(!propagateNewLiteralsToClasp())
                    return false;
            }
            // currentSpace_->status() == FAILED
            else
            {
                // could save a bit of conflict size if conflict resultet from propagate function, which does singular propagation

//#pragma message "This is not so good, conflicting literals will be in front, change and test"
                clits.insert(clits.begin(), assignment_.begin(), assignment_.end());
                //clits.insert(clits.end(), assignment_.begin(), assignment_.end());
                setConflict(clits);
                return false;
            }
            return true;
        }
    return true;
    }
}


 bool GecodeSolver::propagateMinimize()
 {
     //Clasp::LitVec oldDerivedLits;
     //oldDerivedLits.swap(derivedLits_); // maybe something was already propagated
     updateOpt_ = false;
     //TODO: what if i have no space
     size_t i = 0;
     assert(spaces_.size());
     SearchSpace* current = spaces_[0];
     while (true)
     {
         derivedLits_.clear();
         current->updateOptValues();

         //PROBLEM, arbeite direkt auf dem space, dieser kann fehlschlagen!
         if (current->failed() || current->status()==SS_FAILED)
         {
             Clasp::LitVec conflict(assignment_.begin(), assignment_.begin()+assLength(i));
             propQueue_.clear();
             setConflict(conflict);
             return false;
         }
         else
         {
             //monotone
             /*if (current == currentSpace_)
             {
                 if (derivedLits_.size()>0)
                 {
                     unsigned int dl = currentDL();
                     s_->setBacktrackLevel(std::min(dl,s_->backtrackLevel()));
                     if (dl < s_->decisionLevel())
                         s_->undoUntil(dl);
                     if (!propagateNewLiteralsToClasp())
                         return false;
                     break;
                 }
             }
             else*/
             if (derivedLits_.size()>0)
             {
                 unsigned int dl = dl_[i];
                 s_->setBacktrackLevel(std::min(dl,s_->backtrackLevel()));
                 if (dl < s_->decisionLevel())
                     s_->undoUntil(dl);
                 if (!propagateNewLiteralsToClasp())
                     return false;
                 break;
             }
         }
         if (current == currentSpace_)
             break;// nothing found
         else
             if (++i < spaces_.size())
                 current = spaces_[i];
  //           else
 //                current = currentSpace_;
     }
     //derivedLits_.swap(oldDerivedLits);
     return propagate();
     // for each decisionLevel check if new propagation takes place ?

 }


/* backjump to decision level: unsigned int level
   for undoLevel one has to call with: undoUntil(s_-decisionLevel() - 1)
   remove level
*/
void GecodeSolver::undo(unsigned int level)
{
    assert(level!=0);
    //assert(level <= currentDL());
    assert(level==currentDL());

    //std::cout << "undo level" << level << "/" << currentDL() << std::endl;

    /*
    unsigned int index;
    std::vector<unsigned int>::iterator ind = std::lower_bound(dl_.begin(), dl_.end(),level);
    assert(ind != dl_.end());
    --ind;
    index = ind-dl_.begin();
    */
    //std::cout << "backtrack to index " << index << std::endl;



    delete spaces_.back();
    spaces_.pop_back();
    currentSpace_ = spaces_.back();

    //currentSpace_ = spaces_[index];//static_cast<GecodeSolver::SearchSpace*>(spaces_[index]->clone());
    propQueue_.clear();

    //erase all spaces above
   /* ++index;
    for (unsigned int j = index; j != spaces_.size(); ++j)
    {
        delete spaces_[j];
    }
    spaces_.resize(index);*/
    //dl_.resize(index);
    dl_.pop_back();
    //assignment_.resize(assLength_[index-1]);
    assignment_.resize(assLength_.back());
    //assLength_.resize(index-1);
    assLength_.pop_back();



    //currentDL_ = level-1;
    //currentDL_ = dl_[index-1];
    return;
}

bool GecodeSolver::propagateNewLiteralsToClasp()
{
    unsigned int size = assignment_.size();

    if (lazyLearn_)
    {
        for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
        {

            // if not already decided
            //if (s_->value(i->var())==Clasp::value_free)
            if (!s_->isTrue(*i))  // if the literal is not already true (undecided or false)          
            {
                litToAssPosition_[(*i).var()] = size;
                //assignment_.push_back(*i);
                if (!s_->force(*i, &dummyReason_))
                {

                    derivedLits_.clear();
                    return false;
                }

            }
        }
        derivedLits_.clear();
        return true;
    }
    else
    {
        ClauseCreator gc(s_);
        for (Clasp::LitVec::const_iterator i = derivedLits_.begin(); i != derivedLits_.end(); ++i)
        {
            if (!s_->isTrue(*i))
            {
                uint32 dl = s_->decisionLevel();
                //assignment_.push_back(*i);
                gc.startAsserting(Constraint_t::learnt_conflict, *i);
                Clasp::LitVec reason;
                createReason(reason,*i,assignment_.begin(), assignment_.begin()+size);

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
                    assert(false && "Shouldn't occur here");
            }
            else
            {

            }
        }
        derivedLits_.clear();
        return true;
    }
    assert(false);
}

void GecodeSolver::createReason(Clasp::LitVec& reason, const Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end)
{
    reasonAnalyzer_->generate(reason, l, begin, end);
}

Clasp::ConstraintType GecodeSolver::CSPDummy::reason(const Literal& l, Clasp::LitVec& reason)
{
    gecode_->createReason(reason, l, gecode_->assignment_.begin(), gecode_->assignment_.begin()+gecode_->litToAssPosition_[l.var()]);
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


    // set static constraints
    int numReified = 0;
    for(GecodeSolver::ConstraintMap::iterator i = constraints.begin(); i != constraints.end(); ++i)
    {
        if (csps_->getSolver()->value(i->first) == value_free)
            ++numReified;
        else
        {
            //std::cout << "Constraint " << csps_->num2name(i->first) << " is static" << std::endl;
            generateConstraint(i->second,csps_->getSolver()->value(i->first) == value_true);
        }
    }

    //TODO: I think the BoolVarArray is now of dynamic size, so i could use this feature to save a loop
    b_ = BoolVarArray(*this,numReified,0,1);
    unsigned int counter = 0;
    for(GecodeSolver::ConstraintMap::iterator i = constraints.begin(); i != constraints.end(); ++i)
    {
         if (csps_->getSolver()->value(i->first) == value_free)
         {
             csps_->addVarToIndex(i->first, counter);
             generateConstraint(i->second, counter);
             reifwait(*this,csps_,b_[counter],&GecodeSolver::newlyDerived, counter, ICL);
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
        rel(*this, opts_[0] < GecodeSolver::optValues[0],ICL);
    }
    return !failed();

}

void GecodeSolver::SearchSpace::propagate(const Clasp::LitVec::const_iterator& lvstart, const Clasp::LitVec::const_iterator& lvend)
{
    for (Clasp::LitVec::const_iterator i = lvstart; i < lvend; ++i)
    {
        Int::BoolView bv(b_[csps_->varToIndex(i->var())]);
        if (!i->sign())
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
    Int::BoolView bv(b_[csps_->varToIndex(i.var())]);
    if (!i.sign())
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

GecodeSolver::SearchSpace::Value GecodeSolver::SearchSpace::getValueOfConstraint(const Clasp::Var& i)
{
/*
    std::cout << "Asking for " << csps_->num2name(i) << std::endl;
    for (std::map<unsigned int, unsigned int>::const_iterator j = csps_->indexToVar_.begin(); j != csps_->indexToVar_.end(); ++j)
    {
        std::cout << csps_->num2name(j->second) << "=";
        Int::BoolView bv(b_[j->first]);
        if (bv.status()== Int::BoolVarImp::NONE)
            std::cout << "free  ";
        if (bv.status()== Int::BoolVarImp::ONE)
            std::cout << "true  ";
        if (bv.status()== Int::BoolVarImp::ZERO)
            std::cout << "false ";

    }
    std::cout << std::endl;
*/

    Int::BoolView bv(b_[csps_->varToIndex(i)]);
    if (bv.status() == Int::BoolVarImp::NONE)
        return BFREE;
    else
	if (bv.status() == Int::BoolVarImp::ONE)
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

