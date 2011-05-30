#pragma once

#include <gringo/locateable.h>
#include <clingcon/clingcon.h>
#include <clingcon/constraintvarcond.h>
#include <clingcon/exception.h>


namespace Clingcon
{
    class GlobalConstraint;

    class GlobalConstraintHeadLit : public Lit, public Matchable
    {
    public:

        GlobalConstraintHeadLit(const Loc& loc, GlobalConstraint* gc, ConstraintVarCondPtrVec& vec) : Lit(loc), vec_(vec.release()), gc_(gc), set_(false)
        {
            for(ConstraintVarCondPtrVec::iterator i = vec_.begin(); i != vec_.end(); ++i)
                i->lit_ = this;
        }

        void makeSet()
        {
            set_ = true;
        }

        void enqueue(Grounder *g);
        virtual void add(ConstraintVarCond* add)
        {
            vec_.push_back(add);
        }

        virtual GlobalConstraintHeadLit *clone() const
        {
            ConstraintVarCondPtrVec vec(vec_);
            return new GlobalConstraintHeadLit(loc(), gc_, vec);
        }

        virtual void normalize(Grounder *g, Expander *expander);

        virtual bool fact() const
        {
            return false;
        }

        virtual void print(Storage *sto, std::ostream &out) const;
        virtual Index *index(Grounder *, Formula *, VarSet &)
        {
            return new MatchIndex(this);

        }

        virtual void visit(PrgVisitor *visitor);
        virtual void accept(::Printer *)
        {
            /*
            Printer *printer = v->output()->printer<Printer>();
            printer->begin();
            //bool last = false;
            for(size_t i = 0; i < vec_.size(); ++i)
            {
                vec_[i].accept(v);
            }

            printer->end();*/

        }
        ~GlobalConstraintHeadLit();

        bool match(Grounder *g)
        {
            foreach(ConstraintVarCond &lit, vec_) { lit.ground(g); }
            return true;
        }

        void getVariables(GroundedConstraintVarLitVec& vec, Storage* s)
        {
            for(ConstraintVarCondPtrVec::iterator i = vec_.begin(); i != vec_.end(); ++i)
                i->getVariables(vec);
            if (set_)
            {

                //sort(vec.begin(), vec.end(),  (boost::bind(&GroundedConstraintVarLit::compare, _1, _2) <0));
                sort(vec.begin(), vec.end(),  (boost::bind(&GroundedConstraintVarLit::compareset, _1, _2, s) <0));
                GroundedConstraintVarLitVec::iterator it = unique(vec.begin(), vec.end(), boost::bind(&GroundedConstraintVarLit::equalTerm,_1,_2));
                vec.resize(it-vec.begin());
            }
            else
            {
                sort(vec.begin(), vec.end(),  (boost::bind(&GroundedConstraintVarLit::compare, _1, _2, s) <0));
                GroundedConstraintVarLitVec::iterator it = unique(vec.begin(), vec.end(), boost::bind(&GroundedConstraintVarLit::equal,_1,_2));
                vec.resize(it-vec.begin());
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    if (i+1<vec.size() && vec[i].vweight_ == vec[i+1].vweight_)
                        throw CSPException("Two different elements with same index in GlobalConstraint.");
                }
            }
            //for(GroundedConstraintVarLitVec::iterator i = vec.begin(); i != vec.end(); ++i)
            //    std::cout << i->vt_.index << std::endl;

        }

    private:
        ConstraintVarCondPtrVec vec_;
        GlobalConstraint*       gc_;
        bool                    set_;
    };

    inline GlobalConstraintHeadLit* new_clone(const GlobalConstraintHeadLit& a)
    {
            return a.clone();
    }

    class GlobalConstraint : public SimpleStatement
    {
    public:


        class Printer : public ::Printer
        {
        public:
            virtual void type(GCType type) = 0;
            virtual void beginHead() = 0;
            virtual void addHead(GroundedConstraintVarLitVec& vec) = 0;
            virtual void endHead() = 0;
            virtual void end() = 0;
            virtual ~Printer();
        };

        GlobalConstraint(const Loc& loc, GCType name, boost::ptr_vector<ConstraintVarCondPtrVec>& heads, LitPtrVec& body) : SimpleStatement(loc), body_(body.release()), type_(name)
        {
            for(boost::ptr_vector<ConstraintVarCondPtrVec>::iterator i = heads.begin(); i != heads.end(); ++i)
            {
                heads_.push_back(new GlobalConstraintHeadLit(loc, this, *i));
            }

            if (type_==DISTINCT)
            {
                heads_[0].makeSet();
            }
        }

        virtual void visit(PrgVisitor *visitor);

        virtual void print(Storage *sto, std::ostream &out) const;
        virtual void normalize(Grounder *g);
        virtual void append(Lit *lit);
        virtual bool grounded(Grounder *g);
        virtual void accept(::Printer *v, Grounder* g);

        ~GlobalConstraint();

        LitPtrVec& body(){return body_;}
    private:
        boost::ptr_vector<GlobalConstraintHeadLit> heads_;
        LitPtrVec body_;
        GCType      type_;

    };

   /* class GlobalDistinct : public GlobalConstraint
    {
    public:
        GlobalDistinct(const Loc& loc, boost::ptr_vector<ConstraintVarCondPtrVec> &heads, LitPtrVec &body) : GlobalConstraint(loc, heads, body)
        {}

        ~GlobalDistinct();

    private:

    };
    */
}
