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
    ++props_;
    if (!original)
    {
        t_.stop();
        return;
    }


    //unsigned int lindex = g_->varToIndex(l.var());
    //reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);

    original->propagate(~l);
    if (original->status()==SS_FAILED)
        goto Ende;

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
        ++props_;


        //tester->propagate(begin+1, end);
        for (Clasp::LitVec::const_iterator it = begin+1; it < end; ++it)
        {
            tester->propagate(*it);
            if (tester->status()==SS_FAILED)
            {
                // still reason, throw it away, it did not contribute to the reason
                ++begin;
                original->propagate(*(it));
                reason.push_back(*(it));
                end=it;
                if (original->status()==SS_FAILED)
                {
                    delete tester;
                    goto Ende;
                }
                break;
            }

        }


        // if reason still derives l we can remove the last one
        if (tester->status()==SS_FAILED)
        {
        }
        else
        {
            original->propagate(*(begin));
            //++props_;
            reason.push_back(*(begin));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (end-begin==1 || original->status()==SS_FAILED)
            {
                delete tester;
                goto Ende;
            }
            ++begin;
        }
        delete tester;
    }

    Ende:
    assert((delete original, original = g_->getRootSpace(), original->propagate(~l), original->propagate(reason.begin(), reason.end()), original->status()==SS_FAILED));
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


    //unsigned int lindex = g_->varToIndex(l.var());
    //reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);


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
            if(original->getValueOfConstraint((begin+it)->var())!=GecodeSolver::SearchSpace::BFREE)
            {
                if ( (original->getValueOfConstraint((begin+it)->var())==GecodeSolver::SearchSpace::BTRUE && (begin+it)->sign()) ||
                     (original->getValueOfConstraint((begin+it)->var())==GecodeSolver::SearchSpace::BFALSE && !(begin+it)->sign()) )
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
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(~l), original->propagate(reason.begin(), reason.end()), original->status()==SS_FAILED));
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
    ++props_;
    if (!original)
    {
        t_.stop();
        return;
    }


    //unsigned int lindex = g_->varToIndex(l.var());
    //reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);

    original->propagate(~l);
    if (original->status()==SS_FAILED)
        goto Ende;

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
        if (end-begin==1 || original->status()== SS_FAILED)
        {
            goto Ende;
        }
        --end;
    }

    Ende:
    sumLength_+=reason.size();
    g_->setRecording(true);
    //std::cout << sumLength_ << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(~l), original->propagate(reason.begin(), reason.end()), original->status()==SS_FAILED));
    delete original;

    t_.stop();
}


void LinearIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
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
    GecodeSolver::SearchSpace* tester = 0;
    original = g_->getRootSpace();
    ++props_;
    if (!original)
    {
        t_.stop();
        return;
    }

    unsigned int lindex = g_->varToIndex(l.var());
    //reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal


    g_->setRecording(false);

    size_t index = g_->spaces_.size()-1;

    original->propagate(~l);
    if (original->status()==SS_FAILED)
        goto Ende;

    while (g_->assLength_[index]>(ends-begin)) --index;
    //if (index) --index;

    do
    {
        Clasp::LitVec::const_iterator start = begin+g_->assLength(index);
        while(end>start)
        {
            tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());
            props_++;
            //reiffail2(*tester,tester->b_[lindex]);
            tester->propagate(~l);
            tester->propagate(reason.begin(),reason.end());

            if (tester->status()==SS_FAILED)
            {
                // still reason, throw it away, it did not contribute to the reason
                end=start;
                delete tester;
                break;
            }

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
        --index;
    }
    while(end>begin);
    Ende:
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(reason.begin(), reason.end()), original->propagate(~l), original->status()==SS_FAILED));
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
    ++props_;
    if (!original)
    {
        t_.stop();
        return;
    }

    //unsigned int lindex = g_->varToIndex(l.var());
    //reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);

    size_t index = g_->spaces_.size()-1;

    original->propagate(~l);
    if (original->status()==SS_FAILED)
        goto Ende;

    while (g_->assLength_[index]>(ends-begin)) --index;

    do
    {
        Clasp::LitVec::const_iterator start = begin+g_->assLength(index);
        tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());
        //reiffail2(*tester,tester->b_[lindex]);
        tester->propagate(~l);
        tester->propagate(reason.begin(),reason.end());
        props_++;

        if (tester->status()==SS_FAILED)
        {
            // still reason, throw it away, it did not contribute to the reason
            end=start;
        }
        else
        {
            for (Clasp::LitVec::const_iterator it = start; it < end; ++it)
            {
                tester->propagate(*it);
                if (tester->status()==SS_FAILED)
                {
                    original->propagate(start, it+1);
                    reason.insert(reason.end(), start, it+1);
                    //props_++;
                    //std::cout << "reason" << std::endl;
                    //this is enough, but we have to propagate "original", to get a fresh clone out of it
                    if (start > begin && original->status() == SS_FAILED)
                    {
                        delete tester;
                        goto Ende;
                    }
                    end=start;
                }
            }
            assert(true);
        }
        delete tester;
        --index;  
    }
    while(end>begin);
    Ende:
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(reason.begin(), reason.end()), original->propagate(~l), original->status()==SS_FAILED));
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

    unsigned int lindex = g_->varToIndex(l.var());
    //reiffail2(*original,original->b_[lindex]); // fail if we propagate the literal

    g_->setRecording(false);


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
            if(original->getValueOfConstraint((begin+it)->var())!=GecodeSolver::SearchSpace::BFREE)
            {
                if ( (original->getValueOfConstraint((begin+it)->var())==GecodeSolver::SearchSpace::BTRUE && (begin+it)->sign()) ||
                     (original->getValueOfConstraint((begin+it)->var())==GecodeSolver::SearchSpace::BFALSE && !(begin+it)->sign()) )
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
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(~l), original->propagate(reason.begin(), reason.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
    return;
}



}//namespace
