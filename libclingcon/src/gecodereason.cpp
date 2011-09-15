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
#include <clingcon/gecodereason.h>
#include <clingcon/gecodesolver.h>




#include <clasp/solver.h>



namespace Clingcon
{



class ReifWait2 : public Propagator {
public:
    typedef void (Space::* MemFun)(void);
protected:
  /// View to wait for becoming assigned
  Int::BoolView x;
  /// Continuation to execute

  MemFun c;
  //void (*c)(Space&);
  /// Constructor for creation
  ReifWait2(Space& home, Int::BoolView x, MemFun c0);
  /// Constructor for cloning \a p
  ReifWait2(Space& home, bool shared, ReifWait2& p);

  static Space* solver;
  // index to the boolvar array, index > 0 means boolvar is true,
  // index < 0 means boolvar is false
  // if true, index-1 is index
  // if false, index+1 is index
public:
  /// Perform copying during cloning
  virtual Actor* copy(Space& home, bool share);
  /// Const function (defined as low unary)
  virtual PropCost cost(const Space& home, const ModEventDelta& med) const;
  /// Perform propagation
  virtual ExecStatus propagate(Space& home, const ModEventDelta& med);
  /// Post propagator that waits until \a x becomes assigned and then executes \a c
  static ExecStatus post(Space& home, Int::BoolView x, MemFun c);
  /// Delete propagator and return its size
  virtual size_t dispose(Space& home);
};

Space* ReifWait2::solver = 0;

void reifwait2(Space& home, BoolVar x, ReifWait2::MemFun c) {
    if (home.failed()) return;
    GECODE_ES_FAIL(ReifWait2::post(home, x,c));
}


ReifWait2::ReifWait2(Space& home, Int::BoolView x0, MemFun c0)
    : Propagator(home), x(x0), c(c0) {
    x.subscribe(home,*this,PC_GEN_ASSIGNED);
}

ReifWait2::ReifWait2(Space& home, bool shared, ReifWait2& p)
    : Propagator(home,shared,p), c(p.c){
    x.update(home,shared,p.x);
}

Actor*
ReifWait2::copy(Space& home, bool share) {
    return new (home) ReifWait2(home,share,*this);
}
PropCost
ReifWait2::cost(const Space&, const ModEventDelta&) const {
    return PropCost::unary(PropCost::LO);
}

ExecStatus
ReifWait2::propagate(Space& home, const ModEventDelta&) {
    assert(x.assigned());
    if (x.status() == Int::BoolVarImp::ONE)
        (solver->*c)();
    else
        (solver->*c)();
    return home.failed() ? ES_FAILED : home.ES_SUBSUMED(*this);
}
ExecStatus
ReifWait2::post(Space& home, Int::BoolView x, MemFun c) {
    solver = &home;
    if (x.assigned()) {
        if (x.status() == Int::BoolVarImp::ONE)
            (solver->*c)();
        else
            (solver->*c)();
        return home.failed() ? ES_FAILED : ES_OK;
    } else {
        (void) new (home) ReifWait2(home,x,c);
        return ES_OK;
    }
}

size_t
ReifWait2::dispose(Space& home) {
    x.cancel(home,*this,PC_GEN_ASSIGNED);
    (void) Propagator::dispose(home);
    return sizeof(*this);
}

void LinearIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    ++numCalls_;
    t_.start();
    Clasp::LitVec::const_iterator end = ends;
    if (end-begin==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }



    /*
    std::cout << "Reason for ";
    if (l.sign()) std::cout << "f ";
    else std::cout << "t ";
    std::cout << g_->num2name(l.var()) << "(" << l.var() << ")" << std::endl;

    std::cout << "Out of ";
    for (Clasp::LitVec::const_iterator i = begin; i != ends; ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }
*/
    g_->setRecording(false);

    while(end-begin!=0)
    {
        //std::cout << "testing " << (end-1)->sign() << " " << g_->num2name((end-1)->var()) << std::endl;
        // if the last literal is already derived, we do not need to add it to the reason
        if(original->getValueOfConstraint((end-1)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            --end;
            if (end-begin==0)
            {
                //delete original;
                //g_->setRecording(true);
                //t_.stop();
                //return;
                break;
            }
            continue;
        }
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin, end-1);

        tester->status();
        //tester->print(g_->getVariables());
        props_++;

        assert(!tester->failed());
        // if reason still derives l we can remove the last one
        if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
            // still reason, throw it away, it did not contribute to the reason
            //std::cout << "no reason" << std::endl;
            --end;
        }
        else
        {
            original->propagate(*(end-1));
            reason.push_back(*(end-1));
            //std::cout << "reason" << std::endl;
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (end-begin>1 && original->status() && original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
            {
                props_++;
                delete tester;
                tester=0;
                break;
            }
            props_++;
            --end;
        }
        delete tester;
    }
    ///
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    delete original;
    g_->setRecording(true);
    t_.stop();
}


void ExpIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    t_.start();
    ++numCalls_;
    Clasp::LitVec::const_iterator end = ends;
    if (end-begin==0)
    {
        t_.stop();
        return;
    }
    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }

    g_->setRecording(false);

    //how many constraints to leave out
    unsigned int num=1;
    unsigned int upperBound=end-begin;
    while(end-begin!=0)
    {

        while(original->getValueOfConstraint((end-1)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            --end;
            if (end-begin==0)
            {
                delete original;
                g_->setRecording(true);
                t_.stop();
                return;
            }
            --upperBound;
            if (num==upperBound)
            {
                //num=1;
                num/=2;
            }
            continue;
        }
        /*
        unsigned int free = 0;
        //solange in diesem Bereich den wir testen (end-num..end) keine freie Variable ist weiter verdoppeln unn löschen
        for (Clasp::LitVec::const_iterator i = end-num; i != end; ++i)
        {
            if (original->getValueOfConstraint(i->var())==GecodeSolver::SearchSpace::BFREE)
            {
                ++free;
                //if (free==2)
                    break;
            }
        }

        if (free==0)
        {
            end-=num;
            upperBound-=num;
            num=std::min(num*2,upperBound);
            if (num==upperBound)
            {
                //num=1;
                num/=2;
            }
            continue;
        }
*/

        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin, end-num);

        assert(!tester->failed());
        tester->status();
        assert(!tester->failed());
        props_++;


        // if reason still derives l we can remove all
        if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
            // still reason without it, throw it away, it did not contribute to the reason
            end -=num;
            upperBound-=num;
            if (upperBound==1)
            {
                goto Add;
            }

            delete tester;
            num=std::min(num*2,upperBound);
            // if we are expecting a reason we do not need to check the whole next (bigger) part
            // (as there is a reason in it)
            // we just start counting to it again
            if (num==upperBound)
            {
                //num=1;
                num/=2;
            }
            continue;
        }
        else
        {
            if (num==1)
            {
                Add:
                original->propagate(*(end-1));
                assert(!original->failed());
                reason.push_back(*(end-1));
                assert(!original->failed());
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                if (end-begin>1 && original->status() && original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
                {
                    assert(!original->failed());
                    delete tester;
                    tester=0;
                    //we already checked that we can derive the literal, this is enough, save the rest of the work
                    break;
                }
                --end;
                upperBound=end-begin;
                num=1;
                delete tester;
                continue;
            }
            else
            {
                upperBound=num;
                //num=1;
                num/=2;
                delete tester;
                continue;
            }
        }
    }

    delete original;
    g_->setRecording(true);
    t_.stop();
    sumLength_+=reason.size();
}



void FwdLinearIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begins, const Clasp::LitVec::const_iterator& ends)
{
    t_.start();
    ++numCalls_;
    Clasp::LitVec::const_iterator begin = begins;
    Clasp::LitVec::const_iterator end = ends;
    if (end-begin==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }


    g_->setRecording(false);

    while(end-begin!=0)
    {
        // if the last literal is already derived, we do not need to add it to the reason
        while(original->getValueOfConstraint((begin)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            //clasp propagated something that gecode would have also found out
            ++begin;
            if (end-begin==0)
            {
                delete original;
                g_->setRecording(true);
                t_.stop();
                sumLength_+=reason.size();
                return;
            }
        }
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin+1, end);

        tester->status();
        ++props_;

        assert(!tester->failed());
        // if reason still derives l we can remove the last one
        if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
            // still reason, throw it away, it did not contribute to the reason
            ++begin;
        }
        else
        {
            original->propagate(*(begin));
            reason.push_back(*(begin));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (end-begin>1 && original->status() && original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
            {
                delete tester;
                tester=0;
                break;
            }
            ++begin;
        }
        delete tester;
    }

    delete original;
    g_->setRecording(true);
    t_.stop();
    sumLength_+=reason.size();
}



void FwdLinear2IRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begins, const Clasp::LitVec::const_iterator& ends)
{
    ++numCalls_;
    t_.start();
    Clasp::LitVec::const_iterator begin = begins;
    Clasp::LitVec::const_iterator end = ends;
    if (end-begin==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }


    g_->setRecording(false);


    assert(end>=begin);
    uint32 highestLevel = g_->getSolver()->level((end-1)->var());
    Clasp::LitVec::const_iterator i = end-1;
    while (i>=begin && g_->getSolver()->level(i->var())==highestLevel)
        --i;
    Clasp::LitVec::const_iterator start = i+1;

    for (; end > start; --end)
    {
        if(original->getValueOfConstraint((end-1)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            //clasp propagated something that gecode would have also found out
            continue;
        }

        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin, end-1);

        tester->status();
        ++props_;

        assert(!tester->failed());

        if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
            // still reason, throw it away, it did not contribute to the reason
            //--end;
        }
        else
        {
            original->propagate(*(end-1));
            reason.push_back(*(end-1));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (end-begin>1 && original->status() != SS_FAILED && original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
            {
                delete tester;
                 ++props_;
                tester=0;
                goto Ende;
            }
            //--end;
             ++props_;
        }
        delete tester;
    }


    while(end-begin!=0)
    {
        // if the last literal is already derived, we do not need to add it to the reason
        while(original->getValueOfConstraint((begin)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            //clasp propagated something that gecode would have also found out
            ++begin;
            if (end-begin==0)
            {
                delete original;
                g_->setRecording(true);
                sumLength_+=reason.size();
                t_.stop();
                return;
            }
        }
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin+1, end);

        tester->status();
         ++props_;

        assert(!tester->failed());
        // if reason still derives l we can remove the last one
        if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
            // still reason, throw it away, it did not contribute to the reason
            ++begin;
        }
        else
        {
            original->propagate(*(begin));
             ++props_;
            reason.push_back(*(begin));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (end-begin>1 && original->status() && original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
            {
                delete tester;
                tester=0;
                break;
            }
            ++begin;
        }
        delete tester;
    }

    Ende:
    delete original;
    g_->setRecording(true);
    sumLength_+=reason.size();
    t_.stop();
}



SCCIRSRA::SCCIRSRA(GecodeSolver *g) : g_(g), props_(0), numCalls_(0), sumLength_(0)
{
    for (std::map<int, Constraint*>::iterator i =  g->constraints_.begin(); i != g->constraints_.end(); ++i)
    {
        varSets_.insert(std::make_pair(i->first,VarSet(g->getVariables().size())));
        std::vector<unsigned int> vec;
        i->second->getAllVariables(vec,g);
        for (std::vector<unsigned int>::const_iterator v = vec.begin(); v != vec.end(); ++v)
            varSets_[i->first].set(*v);
        /*
        std::cout << "Var " << g_->num2name(i->first) << "(" << i->first << ")" << std::endl;
        for(size_t x = 0; x < varSets_[i->first].size(); ++x)
            std::cout << x << "\t:" << varSets_[i->first][x] << std::endl;
            */
    }
}

void SCCIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    t_.start();
    ++numCalls_;
    if (ends-begin==0)
    {
        t_.stop();
        return;
    }
    int it = (ends-begin)-1;


    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }

    //if (original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
    //    std::cout << "INITIALLY SATISFIED" << std::endl;

    VarSet checker(varSets_[l.var()]);
    unsigned int checkCount = checker.count();

/*
    std::cout << "Reason for ";
    if (l.sign()) std::cout << "f ";
    else std::cout << "t ";
    std::cout << g_->num2name(l.var()) << "(" << l.var() << ")" << std::endl;

    for(size_t i = 0; i < checker.size(); ++i)
        std::cout << i << "\t:" << checker[i] << std::endl;

    std::cout << "Out of ";
    for (Clasp::LitVec::const_iterator i = begin; i != begin+it+1; ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }
*/
    g_->setRecording(false);

    VarSet done(it+1);

    while(true)
    {
        //should not happen, otherwise i would have found a reason before and exit
        assert(done.count()!=ends-begin);

        // if one round passed
        if(it<0)
        {
            // check if knew knowledge was derived, if yes, repeat from beginning, else leave loop
            if (checkCount<checker.count())
            {
                checkCount=checker.count();
                it = (ends-begin)-1;
                continue;
            }
            break;
        }

        // already visited this literal
        if (done.test(it))
        {
            //std::cout << "Skip watched " << g_->num2name((begin+it)->var()) << std::endl;
            --it;
            continue;
        }

        // if the last literal is already derived, we do not need to add it to the reason
        if(original->getValueOfConstraint((begin+it)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            //std::cout << "Already assigned " << g_->num2name((begin+it)->var()) << std::endl;
            checker|=varSets_[(begin+it)->var()];
            done.set(it);

            //RESTART
            if (checkCount<checker.count())
            {
                checkCount=checker.count();
                it = (ends-begin)-1;
                continue;
            }
            else//end
            --it;
            continue;
        }

        if (checker.intersects(varSets_[(begin+it)->var()]))
        {
            tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

            tester->propagate(begin, begin+it);
            //std::cout << "Testing " << g_->num2name((begin+it)->var());

            tester->status();
            props_++;
            done.set(it);

            assert(!tester->failed());
            // if reason still derives l we can remove the last one
            if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
            {
                // still reason, throw it away, it did not contribute to the reason
                //std::cout << " no" << std::endl;
                --it;
            }
            else
            {
                original->propagate(*(begin+it));
                reason.push_back(*(begin+it));
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                original->status();
                assert(!original->failed());
                if (original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
                {
                     //std::cout << " yes and found" << std::endl;
                     props_++;
                     delete tester;

                     sumLength_+=reason.size();
                     //std::cout << sumLength_ << std::endl;
                     //std::cout << "Ende" << std::endl;
                     delete original;
                     g_->setRecording(true);
                     t_.stop();
                     return;
                }
                checker|=varSets_[(begin+it)->var()];
                props_++;
                //std::cout << " yes" << std::endl;
                //RESTART
                if (checkCount<checker.count())
                {
                    delete tester;
                    checkCount=checker.count();
                    it = (ends-begin)-1;
                    continue;
                }
                else//end
                --it;
            }
            delete tester;
        }
        else
            --it;
    }

    //std::cout << "Final watch from beginning" << std::endl;
    //for (it = 0; it != (ends-begin)-1; ++it)
    for (it = (ends-begin)-1; it != 0; --it)
    {
        if (done.test(it))
        {
            //std::cout << "Skip watched " << g_->num2name((begin+it)->var()) << std::endl;
            continue;
        }

        if(original->getValueOfConstraint((begin+it)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            //std::cout << "Already assigned " << g_->num2name((begin+it)->var()) << std::endl;
            continue;
        }

        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin, begin+it);
        //std::cout << "Testing " << g_->num2name((begin+it)->var());

        tester->status();
        props_++;

        assert(!tester->failed());
        // if reason still derives l we can remove the last one
        if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
            // still reason, throw it away, it did not contribute to the reason
            //std::cout << " no" << std::endl;
        }
        else
        {
            original->propagate(*(begin+it));
            reason.push_back(*(begin+it));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            original->status();
            props_++;
            assert(!original->failed());
            if (original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
            {
                //std::cout << " yes and found" << std::endl;

                delete tester;
                sumLength_+=reason.size();
                //std::cout << sumLength_ << std::endl;
                //std::cout << "Ende" << std::endl;
                delete original;
                g_->setRecording(true);
                t_.stop();
                return;
            }
            //std::cout << " yes" << std::endl;
        }
        delete tester;
    }

    reason.push_back(*(begin+it));
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    g_->setRecording(true);
    delete original;
    t_.stop();
    return;
}



void UnionIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    ++numCalls_;
    t_.start();
    Clasp::LitVec::const_iterator end = ends;
    if (end-begin==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }
    delete original;

/*
    std::cout << "Reason for ";
    if (l.sign()) std::cout << "f ";
    else std::cout << "t ";
    std::cout << g_->num2name(l.var()) << "(" << l.var() << ")" << std::endl;

    std::cout << "Out of ";
    for (Clasp::LitVec::const_iterator i = begin; i != ends; ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }
*/
    g_->setRecording(false);

    while(end-begin!=0)
    {

        tester = g_->getRootSpace();


/*
        std::cout << "Propagate:" << std::endl;
        for (Clasp::LitVec::const_iterator i = begin; i < end-1; ++i)
        {
            if (i->sign()) std::cout << "f ";
            else std::cout << "t ";
            std::cout << g_->num2name(i->var()) << std::endl;
        }
        for (Clasp::LitVec::const_iterator i = reason.begin(); i < reason.end(); ++i)
        {
            if (i->sign()) std::cout << "f ";
            else std::cout << "t ";
            std::cout << g_->num2name(i->var()) << std::endl;
        }*/
        tester->propagate(begin, end-1);
        //tester->propagate(end, ends);
        tester->propagate(reason.begin(), reason.end());


        //if (tester->getValueOfConstraint(l.var())==GecodeSolver::SearchSpace::BFREE)// does not seem to work
        //{
            tester->status();
            props_++;
        //}

        assert(!tester->failed());
        // if reason still derives l we can remove the last one
        if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
           // std::cout << "Throw it away" << std::endl;
            // still reason, throw it away, it did not contribute to the reason
            --end;
        }
        else
        {
/*
            std::cout << "use ";
            if ((end-1)->sign()) std::cout << "f ";
            else std::cout << "t ";
            std::cout << g_->num2name((end-1)->var()) << "(" << (end-1)->var() << ")" << std::endl;
*/
            reason.push_back(*(end-1));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            --end;
        }
        delete tester;
    }

    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    g_->setRecording(true);
    t_.stop();
    /*
    std::cout << "Result ";
    for (Clasp::LitVec::const_iterator i = reason.begin(); i != reason.end(); ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }
    */

}


void Union2IRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    ++numCalls_;
    t_.start();
    Clasp::LitVec::const_iterator end = ends;
    if (end-begin==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }


/*
    std::cout << "Reason for ";
    if (l.sign()) std::cout << "f ";
    else std::cout << "t ";
    std::cout << g_->num2name(l.var()) << "(" << l.var() << ")" << std::endl;

    std::cout << "Out of ";
    for (Clasp::LitVec::const_iterator i = begin; i != ends; ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }
*/
    g_->setRecording(false);

    while(end-begin!=0)
    {

        //tester = g_->getRootSpace();
         tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());



         //propagate
         /*
         std::cout << "Propagate:" << std::endl;
         for (Clasp::LitVec::const_iterator i = begin; i < end-1; ++i)
         {
             if (i->sign()) std::cout << "f ";
             else std::cout << "t ";
             std::cout << g_->num2name(i->var()) << std::endl;
         }
         */
        tester->propagate(begin, end-1);
        //tester->propagate(end+1, ends);


        //if (tester->getValueOfConstraint(l.var())==GecodeSolver::SearchSpace::BFREE)// does not seem to work
        //{
            tester->status();
            props_++;
        //}

        assert(!tester->failed());
        // if reason still derives l we can remove the last one
        if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
            // still reason, throw it away, it did not contribute to the reason
            //std::cout << "Throw it away" << std::endl;
            --end;
        }
        else
        {

            /*
            std::cout << "use ";
            if ((end-1)->sign()) std::cout << "f ";
            else std::cout << "t ";
            std::cout << g_->num2name((end-1)->var()) << "(" << (end-1)->var() << ")" << std::endl;
*/
            reason.push_back(*(end-1));
            original->propagate(*(end-1));
            original->status();

            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            --end;
        }
        delete tester;
    }

    delete original;
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    g_->setRecording(true);

    /*
    std::cout << "Result ";
    for (Clasp::LitVec::const_iterator i = reason.begin(); i != reason.end(); ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }*/

    t_.stop();
}



Approx1IRSRA::Approx1IRSRA(GecodeSolver *g) : g_(g), props_(0), numCalls_(0), sumLength_(0), full_(0)
{
    for (std::map<int, Constraint*>::iterator i =  g->constraints_.begin(); i != g->constraints_.end(); ++i)
    {
        varSets_.insert(std::make_pair(i->first,VarSet(g->getVariables().size())));
        std::vector<unsigned int> vec;
        i->second->getAllVariables(vec,g);
        for (std::vector<unsigned int>::const_iterator v = vec.begin(); v != vec.end(); ++v)
            varSets_[i->first].set(*v);
        /*
        std::cout << "Var " << g_->num2name(i->first) << "(" << i->first << ")" << std::endl;
        for(size_t x = 0; x < varSets_[i->first].size(); ++x)
            std::cout << x << "\t:" << varSets_[i->first][x] << std::endl;
            */
    }
}

void Approx1IRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    t_.start();
    ++numCalls_;
    if (ends-begin==0)
    {
        t_.stop();
        return;
    }
    int it = (ends-begin)-1;

    Clasp::LitVec::const_iterator reasonstart = reason.begin();

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }

    //if (original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
    //    std::cout << "INITIALLY SATISFIED" << std::endl;

    VarSet checker(varSets_[l.var()]);
    unsigned int checkCount = checker.count();

/*
    std::cout << "Reason for ";
    if (l.sign()) std::cout << "f ";
    else std::cout << "t ";
    std::cout << g_->num2name(l.var()) << "(" << l.var() << ")" << std::endl;

    for(size_t i = 0; i < checker.size(); ++i)
        std::cout << i << "\t:" << checker[i] << std::endl;

    std::cout << "Out of ";
    for (Clasp::LitVec::const_iterator i = begin; i != begin+it+1; ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }
*/
    g_->setRecording(false);

    VarSet done(it+1);

    while(true)
    {
        //should not happen, otherwise i would have found a reason before and exit
        assert(done.count()!=ends-begin);

        // if one round passed
        if(it<0)
        {
            // check if knew knowledge was derived, if yes, repeat from beginning, else leave loop
            if (checkCount<checker.count())
            {
                checkCount=checker.count();
                it = (ends-begin)-1;
                continue;
            }
            break;
        }

        // already visited this literal
        if (done.test(it))
        {
            //std::cout << "Skip watched " << g_->num2name((begin+it)->var()) << std::endl;
            --it;
            continue;
        }

        if (checker.intersects(varSets_[(begin+it)->var()]))
        {

            done.set(it);
            reason.push_back(*(begin+it));

            checker|=varSets_[(begin+it)->var()];
            //RESTART
            if (checkCount<checker.count())
            {
                checkCount=checker.count();
                it = (ends-begin)-1;
                continue;
            }
            else//end
                --it;
        }
        else
            --it;
    }

    // be careful, must have been empty before
    original->propagate(reasonstart, reason.end());
    assert(!original->failed());
    original->status();
    ++props_;
    assert(!original->failed());
    if (original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
    {
        goto ende;
    }
    else
    {
        ++full_;
        for (int i = 0; i < ends-begin; ++i)
        {
            if (!done.test(i))
            {
                reason.push_back(*(begin+i));
            }
        }
    }



    ende:
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    g_->setRecording(true);
    delete original;
    t_.stop();
    return;
}



void RangeIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    ++numCalls_;
    t_.start();
    Clasp::LitVec::const_iterator end = ends;
    if (end-begin==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }

    /*
    for (int i = 0; i < 6; ++i)
    {
    tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

    tester->propagate(begin, end-1);
    tester->status();
    delete tester;
    }
    */
/*
    std::cout << "Reason for ";
    if (l.sign()) std::cout << "f ";
    else std::cout << "t ";
    std::cout << g_->num2name(l.var()) << "(" << l.var() << ")" << std::endl;

    std::cout << "Out of ";
    for (Clasp::LitVec::const_iterator i = begin; i != ends; ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }
*/
    g_->setRecording(false);

    while(end-begin!=0)
    {
        // if the last literal is already derived, we do not need to add it to the reason
        if(original->getValueOfConstraint((end-1)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            --end;
            if (end-begin==0)
            {
                //delete original;
                //g_->setRecording(true);
                //t_.stop();
                //return;
                break;
            }
            continue;
        }

        original->propagate(*(end-1));
        reason.push_back(*(end-1));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
        if (end-begin>1 && original->status() != SS_FAILED && original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
            props_++;
            break;
        }
        props_++;
        --end;
    }

    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    delete original;
    g_->setRecording(true);
    t_.stop();
}




void RangeLinearIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end)
{
    Clasp::LitVec temp;
    range_.generate(temp,l,begin,end);
    //std::reverse(temp.begin(),temp.end());
    linear_.generate(reason,l,begin,end);
}


size_t Linear2IRSRA::getIndexBelowDL(uint32 level)
{
    std::vector<unsigned int>::iterator ind = std::lower_bound(g_->dl_.begin(), g_->dl_.end(),level);
    assert(ind != g_->dl_.end());
    --ind;
    return ind-g_->dl_.begin();
}


void Linear2IRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    t_.start();
    assert(reason.size()==0);
    ++numCalls_;
    //t_.start();
    Clasp::LitVec::const_iterator end = ends;
    if (end-begin==0)
    {
        t_.stop();
        return;
    }



    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester = 0;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }


/*

    std::cout << "Reason for ";
    if (l.sign()) std::cout << "f ";
    else std::cout << "t ";
    std::cout << g_->num2name(l.var()) << "(" << l.var() << ")" << std::endl;

    std::cout << "Out of ";
    for (Clasp::LitVec::const_iterator i = begin; i != ends; ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }
*/
    g_->setRecording(false);

    assert(end>=begin);
    uint32 highestLevel = g_->getSolver()->level((end-1)->var());
    Clasp::LitVec::const_iterator i = end-1;
    while (i>=begin && g_->getSolver()->level(i->var())==highestLevel)
        --i;
    Clasp::LitVec::const_iterator start = i+1;
    // start sollte auf niedrigstem literal stehen auf höchstem dl, und end hinter letztem
    size_t index = getIndexBelowDL(highestLevel);

    do
    {
        while(end>start)
        {
            tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());
            reifwait2(*tester,tester->b_[g_->varToIndex(l.var())],&Space::fail);
            tester->propagate(start,end-1);
            tester->propagate(reason.begin(),reason.end());

            tester->status();

            props_++;
            //assert(!tester->failed());
           // if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
            if (tester->failed())
            {
                // still reason, throw it away, it did not contribute to the reason
                //std::cout << "no reason" << std::endl;
                --end;
            }
            else
            {
                Found:

                original->propagate(*(end-1));
                reason.push_back(*(end-1));
                //std::cout << "reason" << std::endl;
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                if (end-begin>1 && original->status() != SS_FAILED && original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
                {
                    props_++;
                    delete tester;
                    tester=0;
                    end=begin; // stop everything
                    break;
                }
                props_++;
                --end;
            }
            delete tester;
        }
        --index;
        highestLevel = g_->getSolver()->level((end-1)->var());
        Clasp::LitVec::const_iterator i = end-1;
        while (i>=begin && g_->getSolver()->level(i->var())==highestLevel)
            --i;
        start = i+1;
    }
    while(end>begin);
    ///
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    delete original;
    g_->setRecording(true);
    t_.stop();
}



size_t Linear2GroupedIRSRA::getIndexBelowDL(uint32 level)
{
    std::vector<unsigned int>::iterator ind = std::lower_bound(g_->dl_.begin(), g_->dl_.end(),level);
    assert(ind != g_->dl_.end());
    --ind;
    return ind-g_->dl_.begin();
}


void Linear2GroupedIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    t_.start();
    assert(reason.size()==0);
    ++numCalls_;
    //t_.start();
    Clasp::LitVec::const_iterator end = ends;
    if (end-begin==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester = 0;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }


/*

    std::cout << "Reason for ";
    if (l.sign()) std::cout << "f ";
    else std::cout << "t ";
    std::cout << g_->num2name(l.var()) << "(" << l.var() << ")" << std::endl;

    std::cout << "Out of ";
    for (Clasp::LitVec::const_iterator i = begin; i != ends; ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }
*/
    g_->setRecording(false);

    assert(end>=begin);
    uint32 highestLevel = g_->getSolver()->level((end-1)->var());
    Clasp::LitVec::const_iterator i = end-1;
    while (i>=begin && g_->getSolver()->level(i->var())==highestLevel)
        --i;
    Clasp::LitVec::const_iterator start = i+1;
    // start sollte auf niedrigstem literal stehen auf höchstem dl, und end hinter letztem
    size_t index = getIndexBelowDL(highestLevel);

    do
    {
        //while(end>start)
        {
            //if (end==ends && end-start==1)//first time and only one variable on highest level, this one must be be inside
            //    goto Found;

            tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());


            //tester->propagate(start,end-1);
            tester->propagate(reason.begin(),reason.end());


            tester->status();
            props_++;
            assert(!tester->failed());
            if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
            {
                // still reason, throw it away, it did not contribute to the reason
                //std::cout << "no reason" << std::endl;
                //--end;
                end=start;
            }
            else
            {
                Found:
                original->propagate(start, end);
                reason.insert(reason.end(), start, end);
                //std::cout << "reason" << std::endl;
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                if (start > begin && original->status() != SS_FAILED && original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
                {
                    props_++;
                    delete tester;
                    tester=0;
                    end=begin; // stop everything
                    break;
                }
                props_++;
                //--end;
                end=start;
            }
            delete tester;
        }
        --index;
        highestLevel = g_->getSolver()->level((end-1)->var());
        Clasp::LitVec::const_iterator i = end-1;
        while (i>=begin && g_->getSolver()->level(i->var())==highestLevel)
            --i;
        start = i+1;
    }
    while(end>begin);
    ///
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    delete original;
    g_->setRecording(true);
    t_.stop();
}



}//namespace
