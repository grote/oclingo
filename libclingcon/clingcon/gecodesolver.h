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
#ifndef CLASP_GECODE_H_INCLUDED
#define CLASP_GECODE_H_INCLUDED

#include <clingcon/cspconstraint.h>
#include <clingcon/cspsolver.h>
#include <clasp/util/misc_types.h>
#include <clasp/constraint.h>
#include <clasp/clause.h>
#include <vector>
#include <map>
#include <set>
#include <gecode/search.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include <clingcon/interval.h>


using namespace Gecode;
namespace Clingcon {

    class ConflictAnalyzer;
    class ReasonAnalyzer;
    class Linear2IRSRA;
    class Linear2GroupedIRSRA;
    class FirstUIPRA;
    class Linear2IISCA;
    class Linear2GroupedIISCA;


    class GecodeSolver : public CSPSolver
    {
    public:
        class SearchSpace;

    public:
        friend class SearchSpace;
        //debug
        friend class Linear2IRSRA;
        friend class Linear2GroupedIRSRA;
        friend class FirstUIPRA;
        friend class Linear2IISCA;
        friend class Linear2GroupedIISCA;


        static std::vector<int> optValues;
        static bool             optAll;

        enum Mode
        {
            SIMPLE,
            LINEAR,
            LINEAR_FWD,
            LINEAR_GROUPED,
            SCC,
            LOG,
            RANGE,
            SCCRANGE

        };

        GecodeSolver(bool lazyLearn, bool useCDG, bool weakAS, int numAS,
                     const std::string& ICLString, const std::string& BranchVar, const std::string& BranchVal, std::vector<int> optValueVec, bool optAllPar,
                     bool initialLookahead, const std::string& reduceReason, const std::string& reduceConflict);
        std::string num2name( unsigned int);

        virtual ~GecodeSolver();
        /*
    * adds a constraint with the uid of the atom representing the constraint
    *
    **/
        //virtual void setDomain(int lower, int upper);
        //virtual void addConstraint(Constraint c, int uid);
        //virtual void addDomain(const std::string& var, int lower, int upper);
        virtual void propagateLiteral(const Clasp::Literal& l, int date);
        virtual bool initialize();
        unsigned int currentDL() const;
        unsigned int assLength(unsigned int index) const;
        typedef Interval<int>    IntInterval;
        typedef IntervalSet<int> Domain;
        typedef std::map<unsigned int, Domain> DomainMap;

        // is called when b_[index] is derived via propagation (also by setting it manually?)
        void newlyDerived(int index);
        void addVarToIndex(unsigned int var, unsigned int index);
    private:



        Domain getSet(unsigned int var, CSPLit::Type t, int x) const;
        DomainMap guessDomainsImpl(const Constraint* c);
        void guessDomains(const Constraint* c, bool val);

        DomainMap domains_;   // a domain for each variable
    public:
        // return pointer to space on level 0
        SearchSpace* getRootSpace() const;
        // set if we shall record propagations of gecode
        void setRecording(bool r);
        //virtual bool propagate(const Clasp::LitVec& lv, bool& foundNewLits);
        virtual bool propagate();
        virtual bool propagateMinimize();
        virtual void reset();
        virtual void undo(unsigned int level);
        /*
    * pre: complete assignment
    * return true if a valid solution for the asp vars exists
    * return false elsewise, generates a conflict
    * post: the next answer can be printed
    */
        virtual bool hasAnswer();
        virtual bool nextAnswer();

        /*
    *pre: hasAnswer has been called and returned true
    * prints the answer set
    */
        virtual void printAnswer();


        //end public

    protected:

        //void generateReason(Clasp::LitVec& lits, unsigned int upToAssPos);
        //void generateConflict(Clasp::LitVec& lits, unsigned int upToAssPos);
        bool propagateNewLiteralsToClasp();
        //adds a conflict to the clausecreator
        void createReason(Clasp::LitVec& reason, const Clasp::Literal& l, const Clasp::LitVec::const_iterator& begin, const Clasp::LitVec::const_iterator& end);
        /*
                         * sets a conflict wrt to the current assignment
    */
        void setConflict(Clasp::LitVec conflict);
        /*
    * pre: cdg has been build
    * param in: variable to start with
    * param out: all dependencies of in
    * sideeffect: visitedConstraints_ polluted
    */
        //void findAllDependencies(unsigned int l, std::vector<unsigned int>& result);

        // vector of indices to the boolean variables
        // that have been derived sine last propagation
        Clasp::LitVec derivedLits_;

        std::map<unsigned int, unsigned int> varToIndex_; // translates from clasp Clasp::Literal::Var to boolean csp variable index for reified constraints
        std::map<unsigned int, unsigned int> indexToVar_; // vice versa
        unsigned int varToIndex(unsigned int var);
        unsigned int indexToVar(unsigned int index);

        std::vector<SearchSpace*> spaces_; // spaces_[dl_[Y]] is the fully propagated space of decicion level dl[Y]
        SearchSpace* currentSpace_; // the current space
        std::vector<unsigned int> dl_; // dl_[Y] = X. spaces_[Y] has decision level X
        std::vector<unsigned int> assLength_; // length of the assignment of space X

        typedef std::map<unsigned int, unsigned int> LitToAssPosition;
        //typedef std::map<unsigned int, unsigned int> AssPosToSize;

        LitToAssPosition litToAssPosition_;
        //AssPosToSize assPosToSize_;



        //typedef std::map<unsigned int, std::set<unsigned int> > IntToSet;
        Clasp::LitVec assignment_; //current/old assignemnt
        //			Clasp::LitVec assBeforeProp_; // assignment of (currentDecisionLevel-1) + one currently propagated c-atom
        SearchSpace* enumerator_; // the space for enumerating constraint answer sets

        //everything for enumerating answers
        Search::Options searchOptions_;
        DFS<GecodeSolver::SearchSpace>* dfsSearchEngine_; // depth first search
        BAB<GecodeSolver::SearchSpace>* babSearchEngine_; // branch and bound search



        bool lazyLearn_; // true for lazy(dummy) learning
        bool useCDG_;    // true when using CDG for reason minimisation
        bool weakAS_;    // true if only weak Answer sets shall be calculated
        int  numAS_;     // number of answer sets of weakAS_ is false
        int  asCounter_; // current number of answer sets calculated of the same propositional answer set
        bool updateOpt_; // found model since last backtracking?
        ConflictAnalyzer* conflictAnalyzer_;
        ReasonAnalyzer*   reasonAnalyzer_;
        bool              recording_;  // internal state if i should record propagations of gecode
        bool              initialLookahead_;
        Mode              reduceReason_;
        Mode              reduceConflict_;

        class CSPDummy : public Clasp::Constraint
        {
        public:
            CSPDummy(GecodeSolver* gecode) : gecode_(gecode){}
            Clasp::ConstraintType reason(const Clasp::Literal& l, Clasp::LitVec& reason);

            Constraint::PropResult propagate(const Clasp::Literal&, uint32&, Clasp::Solver&)
            {
                return Constraint::PropResult(true, true);
            }
            Clasp::ConstraintType type() const {return Clasp::Constraint_t::learnt_other;}
        private:
            GecodeSolver* gecode_;
        };

        friend class CSPDummy;
        CSPDummy dummyReason_;
    private:
        //typedef std::set<unsigned int> ConstraintSet ;
        //struct Node
        //{
        //	ConstraintSet node_;
        //};
        //std::vector<bool> visitedConstraints_; // helper variable for CDG search

        //std::map<unsigned int, Node> dependencyGraph_; // graph of constraint dependencies <constraint uid, deps>
        Clasp::LitVec propQueue_;

    public:
        class SearchSpace : public Space
        {
        public:
            friend class GecodeSolver;
            friend class Linear2IRSRA;
            friend class FirstUIPRA;
            friend class Linear2IISCA;
            enum Value
            {
                BFREE,
                BTRUE,
                BFALSE
            };

            //TODO, find a better way for litToVar, static, or smart
            SearchSpace(GecodeSolver* csps, unsigned int numVar, GecodeSolver::ConstraintMap& constraints,
                        LParseGlobalConstraintPrinter::GCvec& gcvec);
            SearchSpace(bool share, SearchSpace& sp);
            virtual ~SearchSpace(){}
            virtual SearchSpace* copy(bool share);
            virtual void constrain(const Space& b);
            void propagate(const Clasp::LitVec::const_iterator& lvstart, const Clasp::LitVec::const_iterator& lvend);
            void propagate(const Clasp::Literal& i);
            //void propagate(const Clasp::LitVec& lv, Clasp::Solver* s);
            //	bool propagate(const Clasp::Literal lv, Clasp::Solver* s);
            void print(const std::vector<std::string>& variables) const;
            //Clasp::LitVec getAssignment(const Clasp::LitVec& as);
            Value getValueOfConstraint(const Clasp::Var& i);
            bool updateOptValues(); // update opt values with static data from GecodeSolver

            // delete litToVar and all shared memory between the spaces
            void cleanAll();
        private:
            void generateConstraint(const Constraint* c, unsigned int boolvar);
            void generateConstraint(const Constraint* c, bool val);
            Gecode::LinRel generateLinearRelation(const Constraint* c);
            Gecode::BoolExpr generateBooleanExpression(const Constraint* c);
            //void generateLinearConstraint(CSPSolver* csps, const GroundConstraint* c, IntArgs& args, IntVarArgs& array, unsigned int num);
            void generateGlobalConstraint(LParseGlobalConstraintPrinter::GC& gc);
            Gecode::LinExpr generateLinearExpr(const GroundConstraint* c);
            Gecode::LinExpr generateSum(std::vector<std::pair<GroundConstraint*,bool> >& vec);
            Gecode::LinExpr generateSum(std::vector<std::pair<GroundConstraint*,bool> >& vec, size_t i);
            //IntVar generateVariable(Constraint c);

            IntVarArray x_;
            BoolVarArray b_;

            //i do it static to now to blow up the space
            static IntVarArgs iva_; // for collecting temporary variables
            static GecodeSolver* csps_;
        public:
            IntVarArray opts_; // optimization variables
            //std::vector<IntVar> tempVars_;
        };
    };
}
#endif
