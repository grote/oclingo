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


}//namespace
