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
#include <boost/dynamic_bitset.hpp>
#include <map>

namespace Clingcon
{
    class GecodeSolver;

    class ConflictAnalyzer
    {
    public:
        virtual ~ConflictAnalyzer(){}
        virtual void printStatistics() = 0;
        // does change the conflict and maybe shrinks it
        virtual void shrink(Clasp::LitVec& conflict, bool last) = 0;
    };

    class SimpleCA : public ConflictAnalyzer
    {
    public:
        SimpleCA() : numCalls_(0), sumLength_(0), oldLength_(0){}
        ~SimpleCA()
        {
        }

        virtual void printStatistics()
        {
            std::cout << 0 << " copys in simple Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << float(0)/numCalls_ << " propsC per call" << std::endl;
            std::cout << "ReducedToC " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedC " << (float(0)/float(oldLength_))*100 << " %" << std::endl;
        }

        virtual void shrink(Clasp::LitVec& conf, bool last)
        {
            oldLength_+=conf.size();
            ++numCalls_;
            t_.start();
            sumLength_+=conf.size();
            t_.stop();
        }
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };

    class LinearIISCA : public ConflictAnalyzer
    {
    public:
        LinearIISCA(GecodeSolver* g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0){}
        ~LinearIISCA()
        {
        }

        virtual void printStatistics()
        {
            std::cout << props_ << " copys in linear Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << numCalls_ << " same   with similar old length" << float(oldLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsC per call" << std::endl;
            std::cout << "ReducedToC " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedC " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }


        virtual void shrink(Clasp::LitVec& conflict, bool last);
    private:
        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };


    class FwdLinearIISCA : public ConflictAnalyzer
    {
    public:
        FwdLinearIISCA(GecodeSolver* g) : g_(g),  props_(0), numCalls_(0), sumLength_(0), oldLength_(0){}
        ~FwdLinearIISCA()
        {
        }

        virtual void printStatistics()
        {
            std::cout << props_ << " copys in fwdlin Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << numCalls_ << " same  with similar old length " << float(oldLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsC per call" << std::endl;
            std::cout << "ReducedToC " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedC " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }

        virtual void shrink(Clasp::LitVec& conflict, bool last);

    private:
        GecodeSolver* g_;
        Timer         t_;
        unsigned int  props_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };


    class RangeCA : public ConflictAnalyzer
    {
    public:
        RangeCA(GecodeSolver* g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0){}
        ~RangeCA()
        {
        }

        virtual void printStatistics()
        {
            std::cout << props_ << " copys in range Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << numCalls_ << " same  with similar old length " << float(oldLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsC per call" << std::endl;
            std::cout << "ReducedToC " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedC " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }

        virtual void shrink(Clasp::LitVec& conflict, bool last);
    private:
        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };


    class CCIISCA : public ConflictAnalyzer
    {
    public:
        CCIISCA(GecodeSolver* g);
        ~CCIISCA()
        { 
        }

        virtual void printStatistics()
        {
            std::cout << props_ << " copys in scc Conflict in " << t_.total() << std::endl;
            //std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            //std::cout << numCalls_ << " same  with similar old length " << float(oldLength_)/numCalls_ << std::endl;
            std::cout << numCalls_ << " ccalls with average length of  " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << numCalls_ << " same   with similar old length " << float(oldLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsC per call" << std::endl;
            std::cout << "ReducedToC " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedC " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }

        virtual void shrink(Clasp::LitVec& conflict, bool last);

    private:
        GecodeSolver* g_;
        Timer         t_;
        unsigned int  props_;
        typedef boost::dynamic_bitset<unsigned int> VarSet;
        std::map<Clasp::Var ,VarSet> varSets_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };


    class CCRangeCA : public ConflictAnalyzer
    {
    public:
        CCRangeCA(GecodeSolver* g);
        ~CCRangeCA()
        {
        }

        virtual void printStatistics()
        {
            std::cout << props_ << " copys in scc-range Conflict in " << t_.total() << std::endl;
            std::cout << numCalls_ << " ccalls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << numCalls_ << " same  with similar old length " << float(oldLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsC per call" << std::endl;
            std::cout << "ReducedToC " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedC " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }

        virtual void shrink(Clasp::LitVec& conflict, bool last);
    private:
        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        typedef boost::dynamic_bitset<unsigned int> VarSet;
        std::map<Clasp::Var,VarSet> varSets_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };

}
#endif
