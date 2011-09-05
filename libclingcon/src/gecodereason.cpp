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
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin, end-1);

        tester->status();
        props_++;

        assert(!tester->failed());
        // if reason still derives l we can remove the last one
        if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
            // still reason, throw it away, it did not contribute to the reason
            --end;
        }
        else
        {
            original->propagate(*(end-1));
            reason.push_back(*(end-1));
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

    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    delete original;
    g_->setRecording(true);
    t_.stop();
}


void ExpIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
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
                    props_++;
                    assert(!original->failed());
                    delete tester;
                    tester=0;
                    //we already checked that we can derive the literal, this is enough, save the rest of the work
                    break;
                }
                props_++;
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
}



void FwdLinearIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begins, const Clasp::LitVec::const_iterator& ends)
{
    Clasp::LitVec::const_iterator begin = begins;
    Clasp::LitVec::const_iterator end = ends;
    if (end-begin==0)
    {
        return;
    }

    // copy the very first searchspace
    GecodeSolver::SearchSpace* original;
    GecodeSolver::SearchSpace* tester;
    original = g_->getRootSpace();
    if (!original)
    {
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
                return;
            }
        }
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());

        tester->propagate(begin+1, end);

        tester->status();

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




void TestIRSRA::generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& ends)
{
    if (l.var()==96)
        int a = 9; // on the second time
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
        tester = static_cast<GecodeSolver::SearchSpace*>(original->clone());
        /*tester->print(g_->getVariables());std::cout << std::endl;

        tester->propagate(begin, end-1);
        tester->print(g_->getVariables());std::cout << std::endl;

        tester->status();
        tester->print(g_->getVariables());std::cout << std::endl;
        */
        props_++;

        assert(!tester->failed());
        // if reason still derives l we can remove the last one
        if (tester->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
        {
            // still reason, throw it away, it did not contribute to the reason
            --end;
        }
        else
        {
            original->propagate(*(end-1));
            reason.push_back(*(end-1));
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

    sumLength_+=reason.size();
    //std::cout << sumLength_ << std::endl;
    delete original;
    g_->setRecording(true);
    t_.stop();
/*

    original = g_->getRootSpace();
    original->print(g_->getVariables());
    original->propagate(~l);
    original->status();

    std::cout << "Reason compare: " << std::endl;
    for (Clasp::LitVec::const_iterator i = reason.begin(); i != reason.end(); ++i)
    {
        if (i->sign())
            std::cout << "f ";
        else
            std::cout << "t ";
        std::cout << g_->num2name(i->var()) << "\t";
        if (original->getValueOfConstraint(i->var())!=GecodeSolver::SearchSpace::BFREE)
        {
            if (original->getValueOfConstraint(i->var())==GecodeSolver::SearchSpace::BTRUE)
                std::cout << "t ";
            else
                std::cout << "f ";
            std::cout << g_->num2name(i->var()) << "\t";
        }
        std::cout << std::endl;
    }
    */

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
        if (end-begin>1 && original->status() && original->getValueOfConstraint(l.var())!=GecodeSolver::SearchSpace::BFREE)
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



}//namespace
