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
    t_.start();
    ++numCalls_;

    if (conflict.size()==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    original = g_->getRootSpace();
    if (!original)
    {
        conflict.clear();
        t_.stop();
        return;
    }

    GecodeSolver::SearchSpace* tester = 0;
    Clasp::LitVec::const_iterator end = conflict.end();

    g_->setRecording(false);

    Clasp::LitVec newConflict;
    size_t index = g_->spaces_.size()-1;
    // special case, the last literal is surely conflicting
    if (conflict.size()>g_->assLength(index))
    {
        assert(conflict.size()==g_->assLength(index)+1);
        newConflict.push_back(conflict.back());
        --end;
        original->propagate(conflict.back());
        if (original->failed() || original->status()==Gecode::SS_FAILED)
        {
            goto Ende;
        }
    }

    // we want to start one space below to save some propagation
    if (index) --index;


    do
    {
        Clasp::LitVec::const_iterator start = conflict.begin() + g_->assLength(index);
        while(end>start)
        {
            assert(!g_->spaces_[index]->failed());

            tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());
            tester->propagate(start,end-1);
            tester->propagate(newConflict.begin(),newConflict.end());
            props_++;
            if (tester->failed() || tester->status()==SS_FAILED)
            {
                // still conflict, throw it away, it did not contribute to the reason
            }
            else
            {
                original->propagate(*(end-1));
                newConflict.push_back(*(end-1));
                //props_++;
                //std::cout << "reason" << std::endl;
                //we propagate to find the shortest reason
                if (original->failed() || original->status() == SS_FAILED)
                {
                    delete tester;
                    tester=0;
                    end=conflict.begin(); // stop everything
                    break;
                } 
            }
            delete tester;
            --end;
        }
        --index;
    }
    while(end>conflict.begin());
    Ende:

    sumLength_+=newConflict.size();
    conflict.swap(newConflict);
    assert((delete original, original = g_->getRootSpace(), original->propagate(newConflict.begin(), newConflict.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
}



void LogIISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    t_.start();
    ++numCalls_;

    if (conflict.size()==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    original = g_->getRootSpace();
    if (!original)
    {
        conflict.clear();
        t_.stop();
        return;
    }

    GecodeSolver::SearchSpace* tester = 0;
    size_t upperBound=conflict.size();

    g_->setRecording(false);

    size_t num=1;

    bool unseenConflict=false;

    Clasp::LitVec newConflict;
    size_t index = g_->spaces_.size()-1;
    // special case, the last literal is surely conflicting
    if (conflict.size()>g_->assLength(index))
    {
        assert(conflict.size()==g_->assLength(index)+1);
        newConflict.push_back(conflict.back());
        --upperBound;
        original->propagate(conflict.back());
        if (original->failed() || original->status()==Gecode::SS_FAILED)
        {
            goto Ende;
        }
    }

    // we want to start one space below to save some propagation
    if (index) --index;
    //std::cout << "Reduced from " << conflict.size() << " to ";
    while(upperBound>0)
    {
        //recalculate the index
        while(g_->assLength(index) > upperBound-num) --index;
        while(g_->assLength(index+1) <= upperBound-num) ++index;

        size_t start = g_->assLength(index);
        assert(!g_->spaces_[index]->failed());

        tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());
        tester->propagate(conflict.begin()+start, conflict.begin()+upperBound-num);
        tester->propagate(newConflict.begin(),newConflict.end());

        ++props_;
        if (tester->failed() || tester->status()==Gecode::SS_FAILED)
        {
            // still failing, throw it away, it did not contribute to the conflict
            upperBound-=num;
        }
        else
        {
            if (num==1)
            {
                original->propagate(*(conflict.begin()+upperBound-num));
                newConflict.push_back(*(conflict.begin()+upperBound-num));
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                if (upperBound<=1 || (original->failed() || original->status()==Gecode::SS_FAILED))
                {
                    delete tester;
                    goto Ende;
                }
                unseenConflict=false;
                upperBound-=1;
            }
            else
            {
                unseenConflict=true;
            }
        }
        delete tester;
        num=std::min(num*2,upperBound);
        if (unseenConflict && num==upperBound)
            num=1;
    }
    Ende:

    conflict.swap(newConflict);
    assert((delete original, original = g_->getRootSpace(), original->propagate(newConflict.begin(), newConflict.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
    sumLength_+=conflict.size();
}


void FwdLinearIISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    t_.start();
    ++numCalls_;

    if (conflict.size()==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    original = g_->getRootSpace();
    if (!original)
    {
        conflict.clear();
        t_.stop();
        return;
    }

    GecodeSolver::SearchSpace* tester = 0;
    Clasp::LitVec::const_iterator begin = conflict.begin();
    Clasp::LitVec::const_iterator end = conflict.end();

    g_->setRecording(false);

    Clasp::LitVec newConflict;
    size_t index = g_->spaces_.size()-1;
    // special case, the last literal is surely conflicting
    if (conflict.size()>g_->assLength(index))
    {
        assert(conflict.size()==g_->assLength(index)+1);
        newConflict.push_back(conflict.back());
        --end;
        original->propagate(conflict.back());
        if (original->failed() || original->status()==Gecode::SS_FAILED)
        {
            goto Ende;
        }
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
        //we do not need to propagate newConflict here as we derive it from "original"

        ++props_;
        if (tester->failed() ||  tester->status()==SS_FAILED)
        {
            ++begin;
        }
        else
        {
            original->propagate(*(begin));
            //++props_;
            newConflict.push_back(*(begin));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (end-begin>1 && (original->failed() || original->status()==SS_FAILED))
            {
                delete tester;
                break;
            }
            ++begin;
        }
        delete tester;
    }

    Ende:

    g_->setRecording(true);
    sumLength_+=newConflict.size();
    conflict.swap(newConflict);
    assert((delete original, original = g_->getRootSpace(), original->propagate(newConflict.begin(), newConflict.end()), original->status()==SS_FAILED));
    delete original;
    t_.stop();
}



void LinearGroupedIISCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    t_.start();
    ++numCalls_;

    if (conflict.size()==0)
    {
        t_.stop();
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    original = g_->getRootSpace();
    if (!original)
    {
        conflict.clear();
        t_.stop();
        return;
    }

    GecodeSolver::SearchSpace* tester = 0;
    Clasp::LitVec::const_iterator end = conflict.end();

    g_->setRecording(false);

    Clasp::LitVec newConflict;
    size_t index = g_->spaces_.size()-1;
    // special case, the last literal is surely conflicting
    if (conflict.size()>g_->assLength(index))
    {
        assert(conflict.size()==g_->assLength(index)+1);
        newConflict.push_back(conflict.back());
        original->propagate(conflict.back());
        --end;
        if (original->failed() || original->status()==Gecode::SS_FAILED)
        {
            goto Ende;
        }
    }

    // we want to start one space below to save some propagation
    if (index) --index;

    while(end>conflict.begin())
    {
        Clasp::LitVec::const_iterator start = conflict.begin() + g_->assLength(index);
        tester = static_cast<GecodeSolver::SearchSpace*>(g_->spaces_[index]->clone());
        tester->propagate(newConflict.begin(),newConflict.end());

        props_++;

        if (tester->failed() || tester->status()==SS_FAILED)
        {
           end=start;
        }
        else
        {
            original->propagate(start, end);
            newConflict.insert(newConflict.end(), start, end);
            //props_++;
            //std::cout << "reason" << std::endl;
            //this is enough, but we have to propagate "original", to get a fresh clone out of it
            if (start > conflict.begin() && (original->failed() || original->status() == SS_FAILED))
            {

                delete tester;
                goto Ende;
            }
            end=start;
        }
        delete tester;
        --index;
    }
    Ende:

    sumLength_+=newConflict.size();
    //std::cout << sumLength_ << std::endl;
    conflict.swap(newConflict);
    assert((delete original, original = g_->getRootSpace(), original->propagate(newConflict.begin(), newConflict.end()), original->status()==SS_FAILED));
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

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    original = g_->getRootSpace();
    if (!original)
    {
        conflict.clear();
        t_.stop();
        return;
    }

    GecodeSolver::SearchSpace* tester = 0;

    g_->setRecording(false);

    int it = (conflict.size())-1;
    Clasp::LitVec newConflict;
    size_t index = g_->spaces_.size()-1;
    VarSet done(it+1);
    VarSet checker(varSets_[(conflict.end()-1)->var()]);
    Clasp::LitVec::const_iterator begin = conflict.begin();
     unsigned int checkCount = checker.count();
    // special case, the last literal is surely conflicting
    if (conflict.size()>g_->assLength(index))
    {
        assert(conflict.size()==g_->assLength(index)+1);
        newConflict.push_back(conflict.back());
        done.set(it);
        --it;
        original->propagate(conflict.back());
        if (original->failed() || original->status()==Gecode::SS_FAILED)
        {
            goto Ende;
        }
    }

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
                //props_++;
                //this is enough, but we have to propagate "original", to get a fresh clone out of it
                //original->status();
                //assert(!original->failed());
                if (original->failed() || original->status()==SS_FAILED)
                {
                     //std::cout << " yes and found" << std::endl;

                     delete tester;
                     goto Ende;
                }
                checker|=varSets_[(begin+it)->var()];
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
            //props_++;
            //assert(!original->failed());
            if (original->failed() || original->status() == SS_FAILED)
            {
                //std::cout << " yes and found" << std::endl;

                delete tester;
                goto Ende;
            }
            //std::cout << " yes" << std::endl;
        }
        delete tester;
    }
    newConflict.push_back(*(begin+it));


    Ende:
    sumLength_+=newConflict.size();
    //std::cout << sumLength_ << std::endl;
    //std::cout << "Ende" << std::endl;
    conflict.swap(newConflict);
    assert((delete original, original = g_->getRootSpace(), original->propagate(newConflict.begin(), newConflict.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
    return;
}


void RangeCA::shrink(Clasp::LitVec& conflict)
{
    oldLength_+=conflict.size();
    t_.start();
    ++numCalls_;

    if (conflict.size()==0)
    {
        t_.stop();
        return;
    }

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
    size_t index = g_->spaces_.size()-1;
    // special case, the last literal is surely conflicting
    if (conflict.size()>g_->assLength(index))
    {
        assert(conflict.size()==g_->assLength(index)+1);
        newConflict.push_back(conflict.back());
        --it;
        original->propagate(conflict.back());
        if (original->failed() || original->status()==Gecode::SS_FAILED)
        {
            goto Ende;
        }
    }

    //std::cout << "Reduced from " << conflict.size() << " to ";
    while(it>=conflict.begin())
    {
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
                goto Ende;
            }
        }

        original->propagate(*it);
        //++props_;
        newConflict.insert(newConflict.end(), *it);
        if (original->failed() || original->status()==Gecode::SS_FAILED)
        {
            //std::cout << "Failed" << std::endl;
            goto Ende;
            break;
        }
        --it;
    }
    Ende:

    conflict.swap(newConflict);
    assert((delete original, original = g_->getRootSpace(), original->propagate(newConflict.begin(), newConflict.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
    sumLength_+=conflict.size();
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

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    original = g_->getRootSpace();
    if (!original)
    {
        conflict.clear();
        t_.stop();
        return;
    }

    GecodeSolver::SearchSpace* tester = 0;

    g_->setRecording(false);

    int it = (conflict.size())-1;
    Clasp::LitVec newConflict;
    size_t index = g_->spaces_.size()-1;
    VarSet done(it+1);
    VarSet checker(varSets_[(conflict.end()-1)->var()]);
    Clasp::LitVec::const_iterator begin = conflict.begin();
     unsigned int checkCount = checker.count();
    // special case, the last literal is surely conflicting
    if (conflict.size()>g_->assLength(index))
    {
        assert(conflict.size()==g_->assLength(index)+1);
        newConflict.push_back(conflict.back());
        done.set(it);
        --it;
        original->propagate(conflict.back());
        if (original->failed() || original->status()==Gecode::SS_FAILED)
        {
            goto Ende;
        }
    }

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
        GecodeSolver::SearchSpace::Value val = original->getValueOfConstraint((conflict.begin()+it)->var());
        if(val!=GecodeSolver::SearchSpace::BFREE)
        {
            if ((  (conflict.begin()+it)->sign() && val==GecodeSolver::SearchSpace::BFALSE) ||
                ((!(conflict.begin()+it)->sign()) && val==GecodeSolver::SearchSpace::BTRUE) )
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
            else
            {
                //HIER HAT DIE VARIABLE DEN FALSCHEN WERT UND DAMIT EINEN KONFLIKT !!!
                newConflict.push_back(*(conflict.begin()+it));
                goto Ende;
            }

        }

        if (checker.intersects(varSets_[(conflict.begin()+it)->var()]))
        {

            original->propagate(*(conflict.begin()+it));
            newConflict.push_back(*(conflict.begin()+it));
            //this is enough, but we have to propagate "original", to get a fresh clone out of it

            //props_++;
            if (original->failed() || original->status()==SS_FAILED)
            {
                goto Ende;
            }
            checker|=varSets_[(conflict.begin()+it)->var()];
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
        else
            --it;
    }

    for (it = conflict.size()-1; it != 0; --it)
    {
        if (done.test(it))
        {
            continue;
        }

        GecodeSolver::SearchSpace::Value val = original->getValueOfConstraint((conflict.begin()+it)->var());
        if(val!=GecodeSolver::SearchSpace::BFREE)
        {
            if ((  (conflict.begin()+it)->sign() && val==GecodeSolver::SearchSpace::BFALSE) ||
                ((!(conflict.begin()+it)->sign()) && val==GecodeSolver::SearchSpace::BTRUE) )
            {
                //std::cout << "Already assigned " << g_->num2name((begin+it)->var()) << std::endl;
                continue;
            }
            else
            {
                newConflict.push_back(*(conflict.begin()+it));
                goto Ende;
            }
        }


        original->propagate(*(conflict.begin()+it));
        newConflict.push_back(*(conflict.begin()+it));
        //this is enough, but we have to propagate "original", to get a fresh clone out of it
        //props_++;
        // assert(!original->failed());
        //if (original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        if (original->failed() || original->status() == SS_FAILED)
        {
            goto Ende;
        }
    }
    newConflict.push_back(*(conflict.begin()+it));

    Ende:
    sumLength_+=newConflict.size();
    conflict.swap(newConflict);
    //std::cout << sumLength_ << std::endl;
    assert((delete original, original = g_->getRootSpace(), original->propagate(newConflict.begin(), newConflict.end()), original->status()==SS_FAILED));
    delete original;
    g_->setRecording(true);
    t_.stop();
    return;
}


}//namespace
