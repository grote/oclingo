#pragma once
#include <gringo/formula.h>
#include <gringo/lit.h>
#include <clingcon/clingcon.h>
#include <gringo/gringo.h>
#include <gringo/printer.h>
//#include <clingcon/constraintterm.h>
#include <gringo/instantiator.h>
#include <gringo/aggrlit.h>
#include <gringo/output.h>
#include <clingcon/groundconstraintvarlit.h>
#include <clingcon/constraintterm.h>

namespace Clingcon
{
    class GlobalConstraintHeadLit;


    class ConstraintVarLit : public Lit, public Matchable
    {
    public:



        /*class Printer : public ::Printer
        {
        public:
                virtual void begin(GroundedConstraintVarLitVec& vvec) = 0;
                virtual ~Printer(){}
        };*/

        ConstraintVarLit(const Loc& loc, ConstraintTerm* t, Term* weight) : Lit(loc), t_(t), weight_(weight)
        {

        }

        clone_ptr<Term>* weight()
        {
            return &weight_;
        }

        clone_ptr<ConstraintTerm>* head()
        {
            return &t_;
        }

        //void enqueue(Grounder *g){};

        virtual ConstraintVarLit *clone() const
        {
            return new ConstraintVarLit(*this);
        }

        virtual void grounded(Grounder *grounder, ConstraintVarCond* parent);

        virtual void normalize(Grounder *, Expander *){}
        virtual bool fact() const {return false;}
        virtual void print(Storage *sto, std::ostream &out) const;
        virtual Index *index(Grounder *, Formula *, VarSet &)
        {
            return new MatchIndex(this);
        }

        virtual void visit(PrgVisitor *visitor);

        virtual void accept(::Printer *v);

        virtual bool match(Grounder *)
        {
            return true;
        }

        ~ConstraintVarLit();
    private:
        clone_ptr<ConstraintTerm> t_;
        clone_ptr<Term>           weight_;

    };

    inline ConstraintVarLit* new_clone(const ConstraintVarLit& a)
    {
            return a.clone();
    }


    class ConstraintVarCond : public Formula
    {
            friend class GlobalConstraintHeadLit;
    public:
            /*
            class Printer : public ::Printer
            {
            public:
                    typedef std::pair<uint32_t, uint32_t> State;
            public:
                virtual void begin(GroundedConstraintVarLit& head) = 0;
                virtual void step(bool last) = 0;
                virtual ~Printer(){};
            };
            */



    public:
            ConstraintVarCond(const Loc &loc, Term* weight,ConstraintTerm* first, LitPtrVec &lits);
            //AggrLit *aggr();
            //TermPtrVec *terms();
            GlobalConstraintHeadLit* lit()
            {
                return lit_;
            }

            clone_ptr<Term>* weight()
            {
                return head_.weight();
            }

            clone_ptr<ConstraintTerm>* head()
            {
                return head_.head();
            }

            LitPtrVec *lits();
            void add(Lit *lit);
            // head(bool head);
            //void initHead();

            void initInst(Grounder *g);
            //void finish();
            //void endGround(Grounder *g);
            void enqueue(Grounder *g); // muss man imho nichts machen ?
            void ground(Grounder *g); //imho dito
            bool grounded(Grounder *g);

            void normalize(Grounder *g, uint32_t number);
            void visit(PrgVisitor *visitor);
            void print(Storage *sto, std::ostream &out) const;
            void addDomain(Grounder *, bool ){}

            virtual void accept(::Printer *)
            {
                /*
                Printer *printer = v->output()->printer<Printer>();
                size_t last = 0;
                for (GroundedConstraintVarLitVec::iterator i=values_.end(); i != values_.end(); ++i)
                {
                    ++last;
                    printer->begin(*i);
                    if (last==values_.size())
                        printer->step(true);
                    else
                        printer->step(false);
                }
                */
                //head_.accept(v);

                //printer->begin(&head_);
            }

            void addGround(GroundedConstraintVarLit & gvl)
            {
                values_.push_back(gvl);
            }

            void getVariables(GroundedConstraintVarLitVec& vec)
            {
                vec.insert(vec.end(),values_.begin(), values_.end());
            }

            ~ConstraintVarCond();
    private:
            //SetLit                  set_;  // weights?
            ConstraintVarLit          head_;
            LitPtrVec                 lits_;
            clone_ptr<Instantiator>   inst_;
            VarVec                    headVars_;
            GlobalConstraintHeadLit  *lit_; // former known as aggr_
            GroundedConstraintVarLitVec  values_;
            //mutable tribool         complete_;
            //bool                    head_;
    };


    inline ConstraintVarCond* new_clone(const ConstraintVarCond& a)
    {
            return new ConstraintVarCond(a);
    }


}
