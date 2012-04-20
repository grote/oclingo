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



void LinearIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    unsigned int before = reason.size();
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
    GecodeSolver::SearchSpace* tester = 0;
    original = g_->getRootSpace();


    ++props_;
    if (!original)
    {
        t_.stop();
        return;
    }

    g_->setRecording(false);

    assert((original->propagate(~l), original->propagate(begin, end), original->status()==SS_FAILED));
    assert((delete original, original = g_->getRootSpace(), true));

    Clasp::LitVec::const_iterator start = begin;//+g_->assLength(index);

    original->propagate(~l);
    if (original->status()==SS_FAILED)
        goto Ende;

    while(end>start)
    {
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());
        props_++;

        for (Clasp::LitVec::const_iterator it = end-1; it >= start; --it)
        {
            tester->propagate(*it);
            if (tester->status()==SS_FAILED)
            {
                delete tester;
                original->propagate(*(it));
                reason.push_back(*(it));
                if (original->status() == SS_FAILED)
                {
                    goto Ende;
                }
                // still conflict, throw it away, it did not contribute to the reason
                start=it;
            }
            else
                if (it==start+1)
                {
                    original->propagate(*(start));
                    reason.push_back(*(start));
                    delete tester;
                    //props_++;
                    //std::cout << "reason" << std::endl;
                    //we propagate to find the shortest reason
                    if (original->status() == SS_FAILED)
                    {
                        goto Ende;
                    }
                    ++start;
                }
        }

    }
    Ende:
    sumLength_+=reason.size()-before;
    assert((delete original, original = g_->getRootSpace(), original->propagate(reason.begin()+before, reason.end()), original->propagate(~l), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();

}



void FwdLinearIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    unsigned int before = reason.size();
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
    GecodeSolver::SearchSpace* tester = 0;
    original = g_->getRootSpace();
    ++props_;
    if (!original)
    {
        t_.stop();
        return;
    }

    g_->setRecording(false);

    assert((original->propagate(~l), original->propagate(begin, end), original->status()==SS_FAILED));
    assert((delete original, original = g_->getRootSpace(), true));

    Clasp::LitVec::const_iterator start = begin;

    original->propagate(~l);
    if (original->status()==SS_FAILED)
        goto Ende;

    while(end>start)
    {
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());
        props_++;

        //tester->propagate(start,end-1);
        for (Clasp::LitVec::const_iterator it = start; it < end; ++it)
        {
            tester->propagate(*it);
            if (tester->status()==SS_FAILED)
            {
                delete tester;
                original->propagate(*(it));
                reason.push_back(*(it));
                if (original->status() == SS_FAILED)
                {
                    goto Ende;
                }
                // still conflict, throw it away, it did not contribute to the reason
                end=it;
            }
            else
                if (it==end-2)
                {
                    original->propagate(*(end-1));
                    reason.push_back(*(end-1));
                    delete tester;
                    //props_++;
                    //std::cout << "reason" << std::endl;
                    //we propagate to find the shortest reason
                    if (original->status() == SS_FAILED)
                    {
                        goto Ende;
                    }
                    --end;
                }
        }

    }
    Ende:
    sumLength_+=reason.size()-before;
    //std::cout << sumLength_ << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(reason.begin()+before, reason.end()), original->propagate(~l), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();

}




void RangeIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    unsigned int before = reason.size();
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
    ++props_;
    if (!original)
    {
        t_.stop();
        return;
    }

    g_->setRecording(false);

    assert((original->propagate(~l), original->propagate(begin, end), original->status()==SS_FAILED));
    assert((delete original, original = g_->getRootSpace(), true));

    original->propagate(~l);
    if (original->status()==SS_FAILED)
        goto Ende;

    while(end-begin!=0)
    {
        // if the last literal is already derived, we do not need to add it to the reason
        /*if(original->getValueOfConstraint((end-1)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            --end;
            if (end-begin==0)
            {
                goto Ende;
            }
            continue;
        }*/

        original->propagate(*(end-1));
        reason.push_back(*(end-1));
        //props_++;
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
        if (end-begin==1 || original->status()== SS_FAILED)
        {
            goto Ende;
        }
        --end;
    }

    Ende:
    sumLength_+=reason.size()-before;
    assert((delete original, original = g_->getRootSpace(), original->propagate(~l), original->propagate(reason.begin()+before, reason.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);

    t_.stop();
}



CCIRSRA::CCIRSRA(GecodeSolver *g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0)
{
    for (GecodeSolver::ConstraintMap::iterator i =  g->constraints_.begin(); i != g->constraints_.end(); ++i)
    {
        if (varSets_.find(i->first.var()) == varSets_.end())
            varSets_.insert(std::make_pair(i->first.var(),VarSet(g->getVariables().size())));
        std::vector<unsigned int> vec;
        i->second->getAllVariables(vec,g);
        for (std::vector<unsigned int>::const_iterator v = vec.begin(); v != vec.end(); ++v)
            varSets_[i->first.var()].set(*v);
        /*
        std::cout << "Var " << g_->num2name(i->first) << "(" << i->first << ")" << std::endl;
        for(size_t x = 0; x < varSets_[i->first].size(); ++x)
            std::cout << x << "\t:" << varSets_[i->first][x] << std::endl;
            */
    }
}

void CCIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    unsigned int before = reason.size();
    oldLength_+=ends-begin;
    t_.start();
    ++numCalls_;
    //Clasp::LitVec::const_iterator end = ends;
    if (ends-begin==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester = 0;
    original = g_->getRootSpace();
    ++props_;
    if (!original)
    {
        t_.stop();
        return;
    }

    g_->setRecording(false);

    assert((original->propagate(~l), original->propagate(begin, ends), original->status()==SS_FAILED));
    assert((delete original, original = g_->getRootSpace(), true));

    VarSet checker(varSets_[l.var()]);
    VarSet reasonChecker(g_->getVariables().size());
    unsigned int checkCount = checker.count();
    VarSet done(ends-begin);
    VarSet reasonDone(ends-begin);

    original->propagate(~l);
    if (original->status()==SS_FAILED)
        goto Ende;


    Start:
    done = reasonDone;
    tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());
    props_++;
    do
    {
        checkCount=checker.count();
        for(int it=(ends-begin)-1; it >=0; --it)
        {
            //should not happen, otherwise i would have found a reason before and exit
            assert(done.count()!=ends-begin);

            // already visited this literal
            if (done.test(it)) continue;


            // if the last literal is already derived, we do not need to add it to the reason
            if(original->getValueOfConstraint(*(begin+it))!=GecodeSolver::SearchSpace::BFREE)
            {
                if (original->getValueOfConstraint(*(begin+it))==GecodeSolver::SearchSpace::BFALSE)
                {
                    reason.push_back(*(begin+it));
                    delete tester;
                    goto Ende;
                }
                //std::cout << "Already assigned " << g_->num2name((begin+it)->var()) << std::endl;
                checker|=varSets_[(begin+it)->var()];
                done.set(it);
                reasonDone.set(it);

                continue;
            }
            if (checker.intersects(varSets_[(begin+it)->var()]))
            {
                tester->propagate(*(begin+it));

                done.set(it);

                // if reason still derives l we can remove the last one
                if (tester->status()==SS_FAILED)
                {
                    // still failing
                    original->propagate(*(begin+it));
                    reason.push_back(*(begin+it));
                    reasonDone.set(it);

                    if (original->status()==SS_FAILED)
                    {
                         delete tester;
                         goto Ende;
                    }
                    //checker|=varSets_[(begin+it)->var()];
                    reasonChecker|=varSets_[(begin+it)->var()];
                    checker=reasonChecker;
                    reasonDone|=~done; //dont check any other yet unchecked literals
                    delete tester;
                    goto Start;
                }
                else
                {
                    checker|=varSets_[(begin+it)->var()];
                    continue;
                }
            }
        }

        if (checkCount<checker.count()) // check if knew knowledge was derived, if yes, repeat from beginning,
            continue;
        else
        {
            checker.set();
            continue;
        }
        assert(true);
    }
    while (true);

    Ende:
    sumLength_+=reason.size()-before;
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(~l), original->propagate(reason.begin()+before, reason.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
    return;
}


CCRangeRA::CCRangeRA(GecodeSolver *g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0)
{
    for (GecodeSolver::ConstraintMap::iterator i =  g->constraints_.begin(); i != g->constraints_.end(); ++i)
    {
        if (varSets_.find(i->first.var()) == varSets_.end())
            varSets_.insert(std::make_pair(i->first.var(),VarSet(g->getVariables().size())));
        std::vector<unsigned int> vec;
        i->second->getAllVariables(vec,g);
        for (std::vector<unsigned int>::const_iterator v = vec.begin(); v != vec.end(); ++v)
            varSets_[i->first.var()].set(*v);
        /*
        std::cout << "Var " << g_->num2name(i->first) << "(" << i->first << ")" << std::endl;
        for(size_t x = 0; x < varSets_[i->first].size(); ++x)
            std::cout << x << "\t:" << varSets_[i->first][x] << std::endl;
            */
    }
}

void CCRangeRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    unsigned int before = reason.size();
    oldLength_+=ends-begin;
    t_.start();
    ++numCalls_;
    //Clasp::LitVec::const_iterator end = ends;
    if (ends-begin==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    original = g_->getRootSpace();
    ++props_;
    if (!original)
    {
        t_.stop();
        return;
    }

    g_->setRecording(false);

    assert((original->propagate(~l), original->propagate(begin, ends), original->status()==SS_FAILED));
    assert((delete original, original = g_->getRootSpace(), true));


    VarSet checker(varSets_[l.var()]);
    unsigned int checkCount = checker.count();
    VarSet done(ends-begin);

    original->propagate(~l);
    if (original->status()==SS_FAILED)
        goto Ende;

    do
    {
        checkCount=checker.count();
        for(int it=(ends-begin)-1; it >=0; --it)
        {
            //should not happen, otherwise i would have found a reason before and exit
            assert(done.count()!=ends-begin);

            // already visited this literal
            if (done.test(it)) continue;


            // if the last literal is already derived, we do not need to add it to the reason
            if(original->getValueOfConstraint(*(begin+it))!=GecodeSolver::SearchSpace::BFREE)
            {
                if (original->getValueOfConstraint(*(begin+it))==GecodeSolver::SearchSpace::BFALSE)
                {
                    reason.push_back(*(begin+it));
                    goto Ende;
                }
                //std::cout << "Already assigned " << g_->num2name((begin+it)->var()) << std::endl;
                checker|=varSets_[(begin+it)->var()];
                done.set(it);

                continue;
            }

            if (checker.intersects(varSets_[(begin+it)->var()]))
            {
                original->propagate(*(begin+it));
                reason.push_back(*(begin+it));

                done.set(it);

                // if reason still derives l we can remove the last one
                if (original->status()==SS_FAILED)
                {
                    goto Ende;
                }
                else
                {
                    checker|=varSets_[(begin+it)->var()];
                    continue;
                }
            }
        }

        if (checkCount<checker.count()) // check if knew knowledge was derived, if yes, repeat from beginning,
            continue;
        else
        {
            checker.set();
            continue;
        }
        assert(true);
    }
    while (true);

    Ende:
    sumLength_+=reason.size()-before;
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(~l), original->propagate(reason.begin()+before, reason.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
    return;
}



}//namespace
