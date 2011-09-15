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
#ifndef CLASP_GECODECONFLICT_H_INCLUDED
#define CLASP_GECODECONFLICT_H_INCLUDED

#include <clasp/literal.h>
#include <../app/clingcon/timer.h>

namespace Clingcon
{
    class GecodeSolver;

    class ConflictAnalyzer
    {
    public:
        virtual ~ConflictAnalyzer(){}
        // does change the conflict and maybe shrinks it
        virtual void shrink(Clasp::LitVec& conflict) = 0;
    };

    class SimpleCA : public ConflictAnalyzer
    {
    public:
        SimpleCA() : numCalls_(0), sumLength_(0){}
        ~SimpleCA()
        {
            std::cout << 0 << " propagations in simple Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
        }
        virtual void shrink(Clasp::LitVec& conf)
        {
            /*
            std::cout << "Conflicting with: " << std::endl;
            for (Clasp::LitVec::const_iterator i = conflict.begin(); i != conflict.end(); ++i)
            {
                if (i->sign()) std::cout << "f ";
                else std::cout << "t ";
                std::cout << g_->num2name(i->var()) << std::endl;
            }
        */
            ++numCalls_;
            t_.start();
            sumLength_+=conf.size();
            t_.stop();
        }
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
    };

    class LinearIISCA : public ConflictAnalyzer
    {
    public:
        LinearIISCA(GecodeSolver* g) : g_(g), props_(0), numCalls_(0), sumLength_(0){}
        ~LinearIISCA()
        {
            std::cout << props_ << " propagations in linear Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
        }

        virtual void shrink(Clasp::LitVec& conflict);
    private:
        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
    };

    class ExpIISCA : public ConflictAnalyzer
    {
    public:
        ExpIISCA(GecodeSolver* g) : g_(g),  props_(0), numCalls_(0), sumLength_(0)
        {}
        ~ExpIISCA()
        {
            std::cout << props_ << " propagations in exp Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
        }
        virtual void shrink(Clasp::LitVec& conflict);

    private:
        GecodeSolver* g_;
        Timer         t_;
        unsigned int  props_;
        unsigned int numCalls_;
        unsigned int sumLength_;

    };

    class FwdLinearIISCA : public ConflictAnalyzer
    {
    public:
        FwdLinearIISCA(GecodeSolver* g) : g_(g),  props_(0), numCalls_(0), sumLength_(0){}
        ~FwdLinearIISCA()
        {
            std::cout << props_ << " propagations in fwdlin Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
        }
        virtual void shrink(Clasp::LitVec& conflict);

    private:
        GecodeSolver* g_;
        Timer         t_;
        unsigned int  props_;
        unsigned int numCalls_;
        unsigned int sumLength_;
    };

    class UnionIISCA : public ConflictAnalyzer
    {
    public:
        UnionIISCA(GecodeSolver* g) : g_(g), props_(0), numCalls_(0), sumLength_(0){}
        ~UnionIISCA()
        {
            std::cout << props_ << " propagations in linear Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
        }

        virtual void shrink(Clasp::LitVec& conflict);
    private:
        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
    };


    class RangeCA : public ConflictAnalyzer
    {
    public:
        RangeCA(GecodeSolver* g) : g_(g), props_(0), numCalls_(0), sumLength_(0){}
        ~RangeCA()
        {
            std::cout << props_ << " propagations in range Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
        }

        virtual void shrink(Clasp::LitVec& conflict);
    private:
        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
    };


    class RangeLinearCA : public ConflictAnalyzer
    {
    public:
        RangeLinearCA(GecodeSolver* g) : g_(g), range_(g), linear_(g) {}
        ~RangeLinearCA()
        {
        }

        virtual void shrink(Clasp::LitVec& conflict)
        {
            Clasp::LitVec temp = conflict;
            range_.shrink(conflict);
            std::reverse(conflict.begin(), conflict.end());
            linear_.shrink(temp);
            std::swap(temp,conflict);
        }

    private:
        GecodeSolver* g_;
        RangeCA range_;
        LinearIISCA linear_;
    };

}
#endif
