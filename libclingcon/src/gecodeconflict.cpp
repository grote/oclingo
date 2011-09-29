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
#include <clingcon/gecodeconflict.h>
#include <clingcon/gecodesolver.h>
#include <clasp/solver.h>




namespace Clingcon
{

void LinearIISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    ++numCalls_;
    t_.start();
    if (conflict.size()==0)
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
        conflict.clear();
        t_.stop();
        return;
    }

    g_->setRecording(false);
    Clasp::LitVec newConflict;

    //std::cout << "Reduced from " << conflict.size() << " to ";
    while(conflict.size())
    {
        //THIS WHILE LOOP STILL CHANGES THE NUMBER OF CONFLICTS, AS there can be multiple conflicts, but should be faster this way

        while(original->getValueOfConstraint((conflict.end()-1)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            GecodeSolver::SearchSpace::Value val(original->getValueOfConstraint((conflict.end()-1)->var()));
            // if same sign
            if (((conflict.end()-1)->sign() && val==GecodeSolver::SearchSpace::BFALSE) || (!((conflict.end()-1)->sign()) && val==GecodeSolver::SearchSpace::BTRUE))
            {
                conflict.erase(conflict.end()-1);
                if (conflict.size()==0)
                {
                    delete original;
                    g_->setRecording(true);
                    conflict.swap(newConflict);
                    t_.stop();
                    sumLength_+=conflict.size();
                    return;
                }
            }
            else
            {
                //debug=true;
                //break;

                //opposite sign, this is a conflict
                newConflict.insert(newConflict.end(), conflict.end()-1, conflict.end());
                delete original;
                g_->setRecording(true);
                conflict.swap(newConflict);
                t_.stop();
                sumLength_+=conflict.size();
                return;

            }
        }

        /*
        if ((conflict.end()-1)->sign())
            std::cout << "neg ";
        else
            std::cout << "pos ";
        std::cout << g_->num2name((conflict.end()-1)->var()) << std::endl;
        */

        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());
        tester->propagate(conflict.begin(), conflict.end()-1);

        ++props_;
        if (tester->failed() || tester->status()==Gecode::SS_FAILED)
        {
            // still failing, throw it away, it did not contribute to the conflict
            conflict.erase(conflict.end()-1);
            //std::cout << "skip" << std::endl;
        }
        else
        {
            //if (original->getValueOfConstraint((conflict.end()-1)->var())!=GecodeSolver::SearchSpace::BFREE)
            //    std::cout << "WARNING" << std::endl;
            //std::cout << "add" << std::endl;
            original->propagate(*(conflict.end()-1));
            newConflict.insert(newConflict.end(), conflict.end()-1, conflict.end());
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (conflict.size()>1 && (original->failed() || original->status()==Gecode::SS_FAILED))
            {
                delete original;
                original=0;
                delete tester;
                break;
            }
            conflict.erase(conflict.end()-1);
        }
        delete tester;
    }
    delete original;
    conflict.swap(newConflict);
    g_->setRecording(true);
    t_.stop();
    sumLength_+=conflict.size();
}



size_t Linear2IISCA::getIndexBelowDL(uint32 level)
{
    std::vector<unsigned int>::iterator ind = std::lower_bound(g_->dl_.begin(), g_->dl_.end(),level);
    assert(ind != g_->dl_.end());
    --ind;
    return ind-g_->dl_.begin();
}


void Linear2IISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    t_.start();
    assert(conflict.size()==0);
    ++numCalls_;
    //t_.start();
    Clasp::LitVec::const_iterator end = conflict.end();
    if (conflict.size()==0)
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

    Clasp::LitVec newConflict;
    assert(end>=conflict.begin());
    uint32 highestLevel = g_->getSolver()->level((end-1)->var());
    Clasp::LitVec::const_iterator i = end-1;
    while (i>=conflict.begin() && g_->getSolver()->level(i->var())==highestLevel)
        --i;
    Clasp::LitVec::const_iterator start = i+1;
    // start sollte auf niedrigstem literal stehen auf höchstem dl, und end hinter letztem
    size_t index = getIndexBelowDL(highestLevel);

    do
    {
        while(end>start)
        {
            tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());
            //reifwait2(*tester,tester->b_[g_->varToIndex(l.var())],&Space::fail);
            tester->propagate(start,end-1);
            tester->propagate(newConflict.begin(),newConflict.end());

            tester->status();

            props_++;
            //assert(!tester->failed());
           // if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
            if (tester->failed())
            {
                // still conflict, throw it away, it did not contribute to the reason
                //std::cout << "no reason" << std::endl;
                --end;
            }
            else
            {
                Found:

                original->propagate(*(end-1));
                newConflict.push_back(*(end-1));
                //std::cout << "reason" << std::endl;
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                if (original->failed() || original->status() == SS_FAILED)
                {
                    props_++;
                    delete tester;
                    tester=0;
                    end=conflict.begin(); // stop everything
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
        while (i>=conflict.begin() && g_->getSolver()->level(i->var())==highestLevel)
            --i;
        start = i+1;
    }
    while(end>conflict.begin());
    ///
    sumLength_+=newConflict.size();
    conflict.swap(newConflict);
    //std::cout << sumLength_ << std::endl;
    delete original;
    g_->setRecording(true);
    t_.stop();
}



void ExpIISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    ++numCalls_;
    t_.start();

    if (conflict.size()==0)
    {
        t_.stop();
        return;
    }
    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)// geht nicht, da gerrade kopiert wurde ist er stable
    {
        conflict.clear();
        t_.stop();
        return;
    }

    g_->setRecording(false);
    Clasp::LitVec newConflict;

    unsigned int num=1;
    unsigned int upperBound=conflict.size();
    bool unseenConflict=false;
    //std::cout << "Reduced from " << conflict.size() << " to ";
    while(conflict.size())
    {

        /*THIS IS WRONG, look at Linear to make it right
        while(original->getValueOfConstraint((conflict.end()-1)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            conflict.erase(conflict.end()-1,conflict.end());
            if (conflict.size()==0)
            {
                delete original;
                g_->setRecording(true);
                conflict.swap(newConflict);
                return;
            }
            --upperBound;
            if (num==upperBound)
            {
                num=1;
                //num/=2;
            }
            continue;
        }*/
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());
        tester->propagate(conflict.begin(), conflict.end()-num);

        ++props_;
        if (tester->failed() || tester->status()==Gecode::SS_FAILED)
        {
            // still failing, throw it away, it did not contribute to the conflict
            conflict.erase(conflict.end()-num,conflict.end());
            upperBound-=num;
        }
        else
        {
            if (num==1)
            {
                original->propagate(*(conflict.end()-1));
                newConflict.push_back(*(conflict.end()-1));
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                if (conflict.size()>1 && (original->failed() || original->status()==Gecode::SS_FAILED))
                {
                    delete original;
                    original=0;
                    delete tester;
                    break;
                }
                conflict.erase(conflict.end()-1);
                unseenConflict=false;
                upperBound=conflict.size();
            }
            else
            {
                unseenConflict=true;
                upperBound=num;
                num=1;
                delete tester;
                continue;
            }
        }
        delete tester;
        num=std::min(num*2,upperBound);
        if (unseenConflict && num==upperBound)
            num=1;
    }
    delete original;
    conflict.swap(newConflict);
    g_->setRecording(true);
    t_.stop();
    sumLength_+=conflict.size();
}


void FwdLinearIISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    ++numCalls_;
    t_.start();
    if (conflict.size()==0)
    {
        t_.stop();
        return;
    }
    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)// geht nicht, da gerrade kopiert wurde ist er stable
    {
        conflict.clear();
        t_.stop();
        return;
    }

    g_->setRecording(false);
    Clasp::LitVec newConflict;

    //std::cout << "Reduced from " << conflict.size() << " to ";
    while(conflict.size())
    {
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());
        tester->propagate(conflict.begin()+1, conflict.end());

        ++props_;
        if (tester->failed() || tester->status()==Gecode::SS_FAILED)
        {
            // still failing, throw it away, it did not contribute to the conflict
            conflict.erase(conflict.begin());
        }
        else
        {
            original->propagate(*(conflict.begin()));
            newConflict.insert(newConflict.end(), conflict.begin(), conflict.begin()+1);
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (conflict.size()>1 && (original->failed() || original->status()==Gecode::SS_FAILED))
            {
                delete original;
                original=0;
                delete tester;
                break;
            }
            conflict.erase(conflict.begin());
        }
        delete tester;
    }
    delete original;
    conflict.swap(newConflict);
    g_->setRecording(true);
    t_.stop();
    sumLength_+=conflict.size();
}



void FwdLinear2IISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    ++numCalls_;
    t_.start();
    Clasp::LitVec::const_iterator begin = conflict.begin();
    Clasp::LitVec::const_iterator end = conflict.end();
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
    Clasp::LitVec newConflict;


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
            if ( (original->getValueOfConstraint((end-1)->var())==GecodeSolver::SearchSpace::BTRUE && (end-1)->sign()) ||
                 (original->getValueOfConstraint((end-1)->var())==GecodeSolver::SearchSpace::BFALSE && !(end-1)->sign()) )
            {
                newConflict.push_back(*(end-1));
                goto Ende;
            }
            //clasp propagated something that gecode would have also found out
            continue;
        }

        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin, end-1);

        if (!tester->failed())
        {
            tester->status();
            ++props_;
        }

        //assert(!tester->failed());

        if (tester->failed())
        {
            // still reason, throw it away, it did not contribute to the reason
            //--end;
        }
        else
        {
            original->propagate(*(end-1));
            newConflict.push_back(*(end-1));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (end-begin>1 && (original->failed() || original->status() == SS_FAILED))
            {
                delete tester;
                 ++props_;
                tester=0;
                goto Ende;
            }
            //--end;
             ++props_;
            delete tester;
            break;
        }
        delete tester;
    }


    while(end-begin!=0)
    {
        // if the last literal is already derived, we do not need to add it to the reason
        while(original->getValueOfConstraint((begin)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            if ( (original->getValueOfConstraint((begin)->var())==GecodeSolver::SearchSpace::BTRUE && (begin)->sign()) ||
                 (original->getValueOfConstraint((begin)->var())==GecodeSolver::SearchSpace::BFALSE && !(begin)->sign()) )
            {
                newConflict.push_back(*(begin));
                goto Ende;
            }
            //clasp propagated something that gecode would have also found out
            ++begin;
            if (end-begin==0)
            {
                goto Ende;
            }
        }
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin+1, end);

        if (!tester->failed())
        {
            tester->status();
            ++props_;
        }

        //assert(!tester->failed());
        // if reason still derives l we can remove the last one
        if (tester->failed())
        {
            // still reason, throw it away, it did not contribute to the reason
            ++begin;
        }
        else
        {
            original->propagate(*(begin));
             ++props_;
            newConflict.push_back(*(begin));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (end-begin>1 && (original->failed() || original->status()==SS_FAILED))
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
    sumLength_+=newConflict.size();
    conflict.swap(newConflict);
    t_.stop();
}



size_t Linear2GroupedIISCA::getIndexBelowDL(uint32 level)
{
    std::vector<unsigned int>::iterator ind = std::lower_bound(g_->dl_.begin(), g_->dl_.end(),level);
    assert(ind != g_->dl_.end());
    --ind;
    return ind-g_->dl_.begin();
}


void Linear2GroupedIISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    t_.start();
    assert(conflict.size()==0);
    ++numCalls_;
    //t_.start();
    Clasp::LitVec::const_iterator end = conflict.end();
    if (conflict.size()==0)
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

    assert(end>=conflict.begin());
    uint32 highestLevel = g_->getSolver()->level((end-1)->var());
    Clasp::LitVec::const_iterator i = end-1;
    while (i>=conflict.begin() && g_->getSolver()->level(i->var())==highestLevel)
        --i;
    Clasp::LitVec::const_iterator start = i+1;
    // start sollte auf niedrigstem literal stehen auf höchstem dl, und end hinter letztem
    size_t index = getIndexBelowDL(highestLevel);

    Clasp::LitVec newConflict;
    do
    {
        //while(end>start)
        {
            //if (end==ends && end-start==1)//first time and only one variable on highest level, this one must be be inside
            //    goto Found;

            tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());


            //tester->propagate(start,end-1);
            tester->propagate(newConflict.begin(),newConflict.end());


            tester->status();
            props_++;

            if (tester->failed())
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
                newConflict.insert(newConflict.end(), start, end);
                //std::cout << "reason" << std::endl;
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                if (start > conflict.begin() && (original->failed() || original->status() == SS_FAILED))
                {
                    props_++;
                    delete tester;
                    tester=0;
                    end=conflict.begin(); // stop everything
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
        while (i>=conflict.begin() && g_->getSolver()->level(i->var())==highestLevel)
            --i;
        start = i+1;
    }
    while(end>conflict.begin());
    ///
    sumLength_+=newConflict.size();
    //std::cout << sumLength_ << std::endl;
    delete original;
    g_->setRecording(true);
    t_.stop();
}



SCCIISCA::SCCIISCA(GecodeSolver *g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0)
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

void SCCIISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    t_.start();
    ++numCalls_;
    if (conflict.size()==0)
    {
        t_.stop();
        return;
    }
    int it = (conflict.size())-1;


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

    VarSet checker(varSets_[(conflict.end()-1)->var()]);
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

    Clasp::LitVec newConflict;
    Clasp::LitVec::const_iterator begin = conflict.begin();

    while(true)
    {
        //should not happen, otherwise i would have found a reason before and exit
        assert(done.count()!=conflict.size());

        // if one round passed
        if(it<0)
        {
            // check if knew knowledge was derived, if yes, repeat from beginning, else leave loop
            if (checkCount<checker.count())
            {
                checkCount=checker.count();
                it = (conflict.size())-1;
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
            if ( (original->getValueOfConstraint((begin+it)->var())==GecodeSolver::SearchSpace::BTRUE && (begin+it)->sign()) ||
                 (original->getValueOfConstraint((begin+it)->var())==GecodeSolver::SearchSpace::BFALSE && !(begin+it)->sign()) )
            {
                newConflict.push_back(*(begin+it));
                goto Ende;
            }
            //std::cout << "Already assigned " << g_->num2name((begin+it)->var()) << std::endl;
            checker|=varSets_[(begin+it)->var()];
            done.set(it);

            //RESTART
            if (checkCount<checker.count())
            {
                checkCount=checker.count();
                it = (conflict.size())-1;
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

            if (!tester->failed())
            {
                tester->status();
                props_++;
            }
            done.set(it);

            //assert(!tester->failed());
            // if reason still derives l we can remove the last one
            if (tester->failed())
            {
                // still failing, throw it away, it did not contribute to the conflict
                //std::cout << " no" << std::endl;
                --it;
            }
            else
            {
                original->propagate(*(begin+it));
                newConflict.push_back(*(begin+it));
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                //original->status();
                //assert(!original->failed());
                if (original->failed() || original->status()==SS_FAILED)
                {
                     //std::cout << " yes and found" << std::endl;
                     props_++;
                     delete tester;

                     Ende:
                     sumLength_+=newConflict.size();
                     //std::cout << sumLength_ << std::endl;
                     //std::cout << "Ende" << std::endl;
                     delete original;
                     g_->setRecording(true);
                     conflict.swap(newConflict);
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
                    it = (conflict.size())-1;
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
    for (it = (conflict.size())-1; it != 0; --it)
    {
        if (done.test(it))
        {
            //std::cout << "Skip watched " << g_->num2name((begin+it)->var()) << std::endl;
            continue;
        }

        if(original->getValueOfConstraint((begin+it)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            if ( (original->getValueOfConstraint((begin+it)->var())==GecodeSolver::SearchSpace::BTRUE && (begin+it)->sign()) ||
                 (original->getValueOfConstraint((begin+it)->var())==GecodeSolver::SearchSpace::BFALSE && !(begin+it)->sign()) )
            {
                newConflict.push_back(*(begin+it));
                goto Ende;
            }
            //std::cout << "Already assigned " << g_->num2name((begin+it)->var()) << std::endl;
            continue;
        }

        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin, begin+it);
        //std::cout << "Testing " << g_->num2name((begin+it)->var());

        if (!tester->failed())
        {
            tester->status();
            props_++;
        }

       // assert(!tester->failed());
        // if reason still derives l we can remove the last one
        if (tester->failed())
        {
            // still reason, throw it away, it did not contribute to the reason
            //std::cout << " no" << std::endl;
        }
        else
        {
            original->propagate(*(begin+it));
            newConflict.push_back(*(begin+it));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            //original->status();
            props_++;
            //assert(!original->failed());
            if (original->failed() || original->status() == SS_FAILED)
            {
                //std::cout << " yes and found" << std::endl;

                delete tester;
                sumLength_+=newConflict.size();
                //std::cout << sumLength_ << std::endl;
                //std::cout << "Ende" << std::endl;
                delete original;
                g_->setRecording(true);
                conflict.swap(newConflict);
                t_.stop();
                return;
            }
            //std::cout << " yes" << std::endl;
        }
        delete tester;
    }

    newConflict.push_back(*(begin+it));
    sumLength_+=newConflict.size();
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    g_->setRecording(true);
    delete original;
    conflict.swap(newConflict);
    t_.stop();
    return;
}

void UnionIISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    ++numCalls_;
    t_.start();
    if (conflict.size()==0)
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
        conflict.clear();
        t_.stop();
        return;
    }
    delete original;

    g_->setRecording(false);

    //std::cout << "Reduced from " << conflict.size() << " to ";
    unsigned int i = 1;
    while(conflict.end()-i>=conflict.begin())
    {
        tester = g_->getRootSpace();
        tester->propagate(conflict.begin(), conflict.end()-i);
        tester->propagate(conflict.end()-i+1, conflict.end());


        ++props_;
        if (tester->failed() || tester->status()==Gecode::SS_FAILED)
        {

            std::swap(*(conflict.end()-i), *(conflict.end()-1));
            conflict.pop_back();
        }
        else
        {
            //if (original->getValueOfConstraint((conflict.end()-1)->var())!=GecodeSolver::SearchSpace::BFREE)
            //    std::cout << "WARNING" << std::endl;
            //std::cout << "add" << std::endl;
            //original->propagate(*(conflict.end()-1));
            //newConflict.insert(newConflict.end(), conflict.end()-1, conflict.end());
            //newConflict.push_back(*i);
             ++i;
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
        }
        delete tester;

    }
    //conflict.swap(newConflict);
    g_->setRecording(true);
    t_.stop();
    sumLength_+=conflict.size();
}



void RangeCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    //std::cout << "Enter " << numCalls_ << std::endl;
    ++numCalls_;
    t_.start();
    if (conflict.size()==0)
    {
        t_.stop();
        return;
    }


    /*
    std::cout << "Conflicting with: " << std::endl;
    for (Clasp::LitVec::const_iterator i = conflict.begin(); i != conflict.end(); ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }
*/
    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    original = g_->getRootSpace();
    if (!original)
    {
        conflict.clear();
        t_.stop();
        return;
    }

    g_->setRecording(false);
    Clasp::LitVec newConflict;

    Clasp::LitVec::const_iterator it=conflict.end()-1;
    //std::cout << "Reduced from " << conflict.size() << " to ";
    while(it>=conflict.begin())
    {

        /*
        std::cout << "Testing with ";
        if (it->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(it->var()) << std::endl;
*/
        if(original->getValueOfConstraint((it)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            //std::cout << "non free -> ";
            GecodeSolver::SearchSpace::Value val(original->getValueOfConstraint((it)->var()));
            if (((it)->sign() && val==GecodeSolver::SearchSpace::BFALSE) || ((!(it->sign())) && val==GecodeSolver::SearchSpace::BTRUE))
            {
                //std::cout << "same sign, skip" << std::endl;
                --it;
                continue;
            }
            else
            {
                //std::cout << "different sign, finished" << std::endl;
                newConflict.insert(newConflict.end(), *it);
                //std::cout << "Shortcut" << std::endl;
                break;
            }
        }

        original->propagate(*it);
         ++props_;
        newConflict.insert(newConflict.end(), *it);
        if (original->failed() || original->status()==Gecode::SS_FAILED)
        {
            //std::cout << "Failed" << std::endl;
            break;
        }
        --it;
    }
    //if (!original->failed())
    //    std::cout << "PROBLEM" << std::endl;//muss nicht failed sein, kann auch different sign sein und wäre danach failed !
    delete original;
    conflict.swap(newConflict);
    g_->setRecording(true);
    t_.stop();
    sumLength_+=conflict.size();

    /*
    std::cout << "Results in: " << std::endl;
    for (Clasp::LitVec::const_iterator i = conflict.begin(); i != conflict.end(); ++i)
    {
        if (i->sign()) std::cout << "f ";
        else std::cout << "t ";
        std::cout << g_->num2name(i->var()) << std::endl;
    }*/
}

SCCRangeCA::SCCRangeCA(GecodeSolver *g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0)
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

void SCCRangeCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    t_.start();
    ++numCalls_;
    if (conflict.size()==0)
    {
        t_.stop();
        return;
    }

    int it = conflict.size()-1;//(ends-begin)-1;

    assert(conflict.size()==0);


    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    //GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)
    {
        t_.stop();
        return;
    }

    //if (original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
    //    std::cout << "INITIALLY SATISFIED" << std::endl;

    VarSet checker(varSets_[(conflict.end()-1)->var()]);
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
    Clasp::LitVec newConflict;

    while(true)
    {
        //should not happen, otherwise i would have found a reason before and exit
        assert(done.count()!=conflict.size());

        // if one round passed
        if(it<0)
        {
            // check if knew knowledge was derived, if yes, repeat from beginning, else leave loop
            if (checkCount<checker.count())
            {
                checkCount=checker.count();
                it = conflict.size()-1;
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
        if(original->getValueOfConstraint((conflict.begin()+it)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            //std::cout << "Already assigned " << g_->num2name((begin+it)->var()) << std::endl;
            checker|=varSets_[(conflict.begin()+it)->var()];
            done.set(it);

            //RESTART
            if (checkCount<checker.count())
            {
                checkCount=checker.count();
                it = conflict.size()-1;
                continue;
            }
            else//end
            --it;
            continue;
        }

        if (checker.intersects(varSets_[(conflict.begin()+it)->var()]))
        {
            /*tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

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
            */
            {
                original->propagate(*(conflict.begin()+it));
                newConflict.push_back(*(conflict.begin()+it));
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                original->status();
                //assert(!original->failed());
                //if (original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
                if (original->failed() || original->status()==SS_FAILED)
                {
                     //std::cout << " yes and found" << std::endl;
                     props_++;
                     //delete tester;

                     sumLength_+=newConflict.size();
                     conflict.swap(newConflict);
                     //std::cout << sumLength_ << std::endl;
                     //std::cout << "Ende" << std::endl;
                     delete original;
                     g_->setRecording(true);
                     t_.stop();
                     return;
                }
                checker|=varSets_[(conflict.begin()+it)->var()];
                props_++;
                //std::cout << " yes" << std::endl;
                //RESTART
                if (checkCount<checker.count())
                {
                    //delete tester;
                    checkCount=checker.count();
                    it = conflict.size()-1;
                    continue;
                }
                else//end
                --it;
            }
            //delete tester;
        }
        else
            --it;
    }

/*
    reason=Clasp::LitVec(begin,ends);
    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    g_->setRecording(true);
    delete original;
    t_.stop();
    return;
  */

    for (it = conflict.size()-1; it != 0; --it)
    {
        if (done.test(it))
        {
            //std::cout << "Skip watched " << g_->num2name((begin+it)->var()) << std::endl;
            continue;
        }

        if(original->getValueOfConstraint((conflict.begin()+it)->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            //std::cout << "Already assigned " << g_->num2name((begin+it)->var()) << std::endl;
            continue;
        }

        /*tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

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
        else*/
        {
            original->propagate(*(conflict.begin()+it));
            newConflict.push_back(*(conflict.begin()+it));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            original->status();
            props_++;
            // assert(!original->failed());
            //if (original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
            if (original->failed() || original->status() == SS_FAILED)
            {
                //std::cout << " yes and found" << std::endl;

                //delete tester;
                sumLength_+=newConflict.size();
                conflict.swap(newConflict);
                //std::cout << sumLength_ << std::endl;
                //std::cout << "Ende" << std::endl;
                delete original;
                g_->setRecording(true);
                t_.stop();
                return;
            }
            //std::cout << " yes" << std::endl;
        }
        //delete tester;
    }

    newConflict.push_back(*(conflict.begin()+it));
    sumLength_+=newConflict.size();
    conflict.swap(newConflict);
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    g_->setRecording(true);
    delete original;
    t_.stop();
    return;
}


}//namespace
