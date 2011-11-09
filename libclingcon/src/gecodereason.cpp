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



class ReifFailOn : public Propagator {
protected:
  /// View to wait for becoming assigned
  Int::BoolView x;
  /// Continuation to execute
  //void (*c)(Space&);
  /// Constructor for creation
  ReifFailOn(Space& home, Int::BoolView x);
  /// Constructor for cloning \a p
  ReifFailOn(Space& home, bool shared, ReifFailOn& p);

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
  static ExecStatus post(Space& home, Int::BoolView x);
  /// Delete propagator and return its size
  virtual size_t dispose(Space& home);
};


void reiffail2(Space& home, BoolVar x) {
    if (home.failed()) return;
    GECODE_ES_FAIL(ReifFailOn::post(home, x));
}


ReifFailOn::ReifFailOn(Space& home, Int::BoolView x0)
    : Propagator(home), x(x0) {
    x.subscribe(home,*this,PC_GEN_ASSIGNED);
}

ReifFailOn::ReifFailOn(Space& home, bool shared, ReifFailOn& p)
    : Propagator(home,shared,p) {
    x.update(home,shared,p.x);
}

Actor*
ReifFailOn::copy(Space& home, bool share) {
    return new (home) ReifFailOn(home,share,*this);
}
PropCost
ReifFailOn::cost(const Space&, const ModEventDelta&) const {
    return PropCost::unary(PropCost::LO);
}

ExecStatus
ReifFailOn::propagate(Space& home, const ModEventDelta&) {
    assert(x.assigned());
    home.fail();
    return home.failed() ? ES_FAILED : home.ES_SUBSUMED(*this);
}
ExecStatus
ReifFailOn::post(Space& home, Int::BoolView x) {
    //solver = &home;
    if (x.assigned()) {
        home.fail();
        return home.failed() ? ES_FAILED : ES_OK;
    } else {
        (void) new (home) ReifFailOn(home,x);
        return ES_OK;
    }
}

size_t
ReifFailOn::dispose(Space& home) {
    x.cancel(home,*this,PC_GEN_ASSIGNED);
    (void) Propagator::dispose(home);
    return sizeof(*this);
}


void LogIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    oldLength_+=ends-begin;
    t_.start();
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

    unsigned int lindex = g_->varToIndex(l.var());
    reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);

    size_t index = g_->spaces_.size()-1;
    if (index) --index;

    //how many constraints to leave out
    unsigned int num=1;
    unsigned int upperBound=end-begin;
    while(end-begin!=0)
    {
        size_t start = 0;
        while(original->getValueOfConstraint((end-1)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            --end;
            if (end-begin==0)
            {
                goto Ende;
            }
            --upperBound;
            if (upperBound==1)
            {
                tester=0;
                goto Add;
            }
            if (num==upperBound)
            {
                //num=1;
                num/=2;
            }
            continue;
        }

        //recalculate the index
        while(g_->assLength(index) > (end-num)-begin) --index;
        while(g_->assLength(index+1) <= (end-num)-begin) ++index;

        start = g_->assLength(index);
        assert(!g_->spaces_[index]->failed());

        tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());
        reiffail2(*tester,tester->b_[lindex]);
        tester->propagate(begin+start, end-num);
        tester->propagate(reason.begin(),reason.end());

        props_++;


        //assert(!tester->failed());
        //tester->status();
        //assert(!tester->failed());



        // if reason still derives l we can remove all
        if (tester->failed() || tester->status()==SS_FAILED)
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
                //assert(!original->failed());
                reason.push_back(*(end-1));
                //assert(!original->failed());
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                if (end-begin<=1  || (original->failed() || original->status()==SS_FAILED))
                {
                    delete tester;
                    goto Ende;
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

    Ende:
    assert((delete original, original = g_->getRootSpace(), original->propagate(reason.begin(), reason.end()), reiffail2(*original,original->b_[lindex]), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
    sumLength_+=reason.size();
}




void FwdLinearIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begins, const Clasp::LitVec::const_iterator& ends)
{
    oldLength_+=ends-begins;
    t_.start();
    ++numCalls_;
    Clasp::LitVec::const_iterator end = ends;
    Clasp::LitVec::const_iterator begin = begins;
    if (end-begins==0)
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


    unsigned int lindex = g_->varToIndex(l.var());
    reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);

    while(end-begin!=0)
    {
        // if the literal is already derived, we do not need to add it to the reason
        while(original->getValueOfConstraint((begin)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            //clasp propagated something that gecode would have also found out
            ++begin;
            if (end-begin==0)
            {
                goto Ende;
            }
        }

        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());
        tester->propagate(begin+1, end);
        ++props_;

        // if reason still derives l we can remove the last one
        if (tester->failed() || tester->status()==SS_FAILED)
        {
            // still reason, throw it away, it did not contribute to the reason
            ++begin;
        }
        else
        {
            original->propagate(*(begin));
            //++props_;
            reason.push_back(*(begin));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (end-begin==1 || (original->failed() || original->status()==SS_FAILED))
            {
                delete tester;
                goto Ende;
            }
            ++begin;
        }
        delete tester;
    }

    Ende:
    assert((delete original, original = g_->getRootSpace(), reiffail2(*original,original->b_[lindex]), original->propagate(reason.begin(), reason.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    sumLength_+=reason.size();
    t_.stop();
}



SCCIRSRA::SCCIRSRA(GecodeSolver *g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0)
{
    for (GecodeSolver::ConstraintMap::iterator i =  g->constraints_.begin(); i != g->constraints_.end(); ++i)
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
    oldLength_+=ends-begin;
    t_.start();
    ++numCalls_;
    int it = (ends-begin)-1;
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


    unsigned int lindex = g_->varToIndex(l.var());
    reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);


    VarSet checker(varSets_[l.var()]);
    unsigned int checkCount = checker.count();
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
            props_++;
            done.set(it);

            // if reason still derives l we can remove the last one
            if (tester->failed() || tester->status()==SS_FAILED)
            {
                // still reason, throw it away, it did not contribute to the reason
                --it;
            }
            else
            {
                original->propagate(*(begin+it));
                reason.push_back(*(begin+it));
                //this is enough, but we have to propagate "original", to get a fresh clone out of it

                if (original->failed() || original->status()==SS_FAILED)
                {
                    delete tester;
                    goto Ende;
                }
                checker|=varSets_[(begin+it)->var()];
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
        props_++;

        // if reason still derives l we can remove the last one
        if (tester->failed() || tester->status()==SS_FAILED)
        {
            // still reason, throw it away, it did not contribute to the reason
        }
        else
        {
            original->propagate(*(begin+it));
            reason.push_back(*(begin+it));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (original->failed() || original->status()==SS_FAILED)
            {
                //std::cout << " yes and found" << std::endl;

                delete tester;
                goto Ende;
            }
            //std::cout << " yes" << std::endl;
        }
        delete tester;
    }

    reason.push_back(*(begin+it));
    Ende:
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    assert((delete original, original = g_->getRootSpace(), reiffail2(*original,original->b_[lindex]), original->propagate(reason.begin(), reason.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
    return;
}



void RangeIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    oldLength_+=ends-begin;
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
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }


    unsigned int lindex = g_->varToIndex(l.var());
    reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);

    while(end-begin!=0)
    {
        // if the last literal is already derived, we do not need to add it to the reason
        if(original->getValueOfConstraint((end-1)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            --end;
            if (end-begin==0)
            {
                goto Ende;
            }
            continue;
        }

        original->propagate(*(end-1));
        reason.push_back(*(end-1));
        //props_++;
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
        if (end-begin==1 || (original->failed() || original->status()== SS_FAILED))
        {
            goto Ende;
        }
        --end;
    }

    Ende:
    sumLength_+=reason.size();
    g_->setRecording(true);
    //std::cout << sumLength_ << std::endl;
    assert((delete original, original = g_->getRootSpace(), reiffail2(*original,original->b_[lindex]), original->propagate(reason.begin(), reason.end()), original->status()==SS_FAILED));
    delete original;

    t_.stop();
}


void LinearIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    oldLength_+=ends-begin;
    t_.start();
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

    unsigned int lindex = g_->varToIndex(l.var());
    reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);

    size_t index = g_->spaces_.size()-1;
    if (index) --index;

    do
    {
        Clasp::LitVec::const_iterator start = begin+g_->assLength(index);
        while(end>start)
        {
            tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());
            reiffail2(*tester,tester->b_[lindex]);
            tester->propagate(start,end-1);
            tester->propagate(reason.begin(),reason.end());
            props_++;

            if (tester->failed() ||  tester->status()==SS_FAILED)
            {
                // still reason, throw it away, it did not contribute to the reason
                --end;
            }
            else
            {
                original->propagate(*(end-1));
                reason.push_back(*(end-1));
                //props_++;
                //std::cout << "reason" << std::endl;
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                if (end-begin>1 && original->status() == SS_FAILED)// we found the literal because we failed
                { 
                    delete tester;
                    goto Ende;
                }
                --end;
            }
            delete tester;
        }
        --index;
    }
    while(end>begin);
    Ende:
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(reason.begin(), reason.end()), reiffail2(*original,original->b_[lindex]), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
}


void LinearGroupedIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    oldLength_+=ends-begin;
    t_.start();
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

    unsigned int lindex = g_->varToIndex(l.var());
    reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);

    size_t index = g_->spaces_.size()-1;
    if (index) --index;

    do
    {
        Clasp::LitVec::const_iterator start = begin+g_->assLength(index);
        tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());
        reiffail2(*tester,tester->b_[lindex]);
        tester->propagate(reason.begin(),reason.end());
        props_++;

        if (tester->failed() || tester->status()==SS_FAILED)
        {
            // still reason, throw it away, it did not contribute to the reason
            end=start;
        }
        else
        {
            original->propagate(start, end);
            reason.insert(reason.end(), start, end);
            //props_++;

            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (start > begin && (original->failed() || original->status() == SS_FAILED))
            {
                delete tester;
                goto Ende;
            }
            end=start;
        }
        delete tester;
        --index;  
    }
    while(end>begin);
    Ende:
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(reason.begin(), reason.end()), reiffail2(*original,original->b_[lindex]), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
}


SCCRangeRA::SCCRangeRA(GecodeSolver *g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0)
{
    for (GecodeSolver::ConstraintMap::iterator i =  g->constraints_.begin(); i != g->constraints_.end(); ++i)
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

void SCCRangeRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    oldLength_+=ends-begin;
    t_.start();
    ++numCalls_;
    int it = (ends-begin)-1;
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


    unsigned int lindex = g_->varToIndex(l.var());
    reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);

    VarSet checker(varSets_[l.var()]);
    unsigned int checkCount = checker.count();
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
            original->propagate(*(begin+it));
            reason.push_back(*(begin+it));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (original->failed() || original->status()==SS_FAILED)
            {
                goto Ende;
            }
            checker|=varSets_[(begin+it)->var()];
            //std::cout << " yes" << std::endl;
            //RESTART
            if (checkCount<checker.count())
            {
                //delete tester;
                checkCount=checker.count();
                it = (ends-begin)-1;
                continue;
            }
            else//end
                --it;
            //delete tester;
        }
        else
            --it;
    }

    for (it = (ends-begin)-1; it != 0; --it)
    {
        if (done.test(it))
        {
            continue;
        }

        if(original->getValueOfConstraint((begin+it)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            continue;
        }

        original->propagate(*(begin+it));
        reason.push_back(*(begin+it));
        //this is enough, but we have to propagate "original", to get a fresh clone out of it
        if (original->failed() || original->status()==SS_FAILED)
        {
            goto Ende;
        }
    }

    reason.push_back(*(begin+it));
    Ende:

    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(reason.begin(), reason.end()), reiffail2(*original,original->b_[lindex]), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
    return;
}



}//namespace
