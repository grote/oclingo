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
#ifndef CLASP_GECODEREASON_H_INCLUDED
#define CLASP_GECODEREASON_H_INCLUDED

#include <clasp/literal.h>
#include <../app/clingcon/timer.h>
#include <boost/dynamic_bitset.hpp>
#include <map>

namespace Clingcon
{
    class GecodeSolver;

    class ReasonAnalyzer
    {
    public:
        virtual ~ReasonAnalyzer(){}
        // does change the conflict and maybe shrinks it
        virtual void generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end) = 0;
    };

    class SimpleRA : public ReasonAnalyzer
    {
    public:
        SimpleRA() : numCalls_(0), sumLength_(0), oldLength_(0){}
        ~SimpleRA()
        {
            std::cout << 0 << " propagataions in simple reasons in " << t_.total() << std::endl;
            std::cout << numCalls_ << " calls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << "ReducedToR " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedR " << (float(0)/float(oldLength_))*100 << " %" << std::endl;
        }

        virtual void generate(Clasp::LitVec& reason, const Clasp::Literal& , const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end)
        {
            oldLength_+=end-begin;
            t_.start();
            ++numCalls_;
            reason.insert(reason.end(), begin, end);
            sumLength_+=reason.size();
            t_.stop();
        }
    private:
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };

    //irreducible reason set
    //////////////// THIS IS NOT IRREDUCIBLE, if reason is a,b,c, then c can be the cause of ab, but ab is the minimal reason



    class FwdLinearIRSRA : public ReasonAnalyzer
    {
    public:
        FwdLinearIRSRA(GecodeSolver* g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0){}
        ~FwdLinearIRSRA()
        {
            std::cout << props_ << " propagataions in fwdlinear reasons in " << t_.total() << std::endl;
            std::cout << numCalls_ << " calls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsR per call" << std::endl;
            std::cout << "ReducedToR " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedR " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }
        virtual void generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end);

    private:
        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };


    class SCCIRSRA : public ReasonAnalyzer
    {
    public:
        SCCIRSRA(GecodeSolver* g);
        ~SCCIRSRA()
        {
            std::cout << props_ << " propagataions in scc reasons in " << t_.total() << std::endl;
            std::cout << numCalls_ << " calls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsR per call" << std::endl;
            std::cout << "ReducedToR " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedR " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }
        virtual void generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end);

    private:
        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        typedef boost::dynamic_bitset<unsigned int> VarSet;
        std::map<unsigned int,VarSet> varSets_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };


    // start incrementally adding a literal until a reason is reached
    class RangeIRSRA : public ReasonAnalyzer
    {
    public:
        RangeIRSRA(GecodeSolver* g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0){}
        ~RangeIRSRA()
        {
            std::cout << props_ << " propagataions in range reasons in " << t_.total() << std::endl;
            std::cout << numCalls_ << " calls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsR per call" << std::endl;
            std::cout << "ReducedToR " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedR " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }
        virtual void generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end);

    private:
        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };


    class LinearIRSRA : public ReasonAnalyzer
    {
    public:
        LinearIRSRA(GecodeSolver* g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0){}
        ~LinearIRSRA()
        {
            std::cout << props_ << " propagataions in linear reasons in " << t_.total() << std::endl;
            std::cout << numCalls_ << " calls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsR per call" << std::endl;
            std::cout << "ReducedToR " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedR " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }
        virtual void generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end);

    private:

        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };


    class LinearGroupedIRSRA : public ReasonAnalyzer
    {
    public:
        LinearGroupedIRSRA(GecodeSolver* g) : g_(g), props_(0), numCalls_(0), sumLength_(0), oldLength_(0){}
        ~LinearGroupedIRSRA()
        {
            std::cout << props_ << " propagataions in linear2 grouped reasons in " << t_.total() << std::endl;
            std::cout << numCalls_ << " calls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsR per call" << std::endl;
            std::cout << "ReducedToR " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedR " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }
        virtual void generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end);

    private:

        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };


    class SCCRangeRA : public ReasonAnalyzer
    {
    public:
        SCCRangeRA(GecodeSolver* g);
        ~SCCRangeRA()
        {
            std::cout << props_ << " propagataions in scc-range reasons in " << t_.total() << std::endl;
            std::cout << numCalls_ << " calls with average length of " << float(sumLength_)/numCalls_ << std::endl;
            std::cout << float(props_)/numCalls_ << " propsR per call" << std::endl;
            std::cout << "ReducedToR " << (float(sumLength_)/float(oldLength_))*100 << " %" << std::endl;
            std::cout << "AnalyzedR " << (float(props_)/float(oldLength_))*100 << " %" << std::endl;
        }
        virtual void generate(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end);

    private:
        GecodeSolver* g_;
        unsigned int  props_;
        Timer         t_;
        typedef boost::dynamic_bitset<unsigned int> VarSet;
        std::map<unsigned int,VarSet> varSets_;
        unsigned int numCalls_;
        unsigned int sumLength_;
        unsigned int oldLength_;
    };





}
#endif
