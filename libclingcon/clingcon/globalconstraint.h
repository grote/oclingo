#pragma once

#include <gringo/locateable.h>
#include <clingcon/clingcon.h>
#include <clingcon/constraintvarcond.h>
#include <clingcon/exception.h>


namespace Clingcon
{
    class GlobalConstraint;

    class GlobalConstraintHeadLit : public Lit, public Matchable, public Container
    {
    public:
        enum Type{
            Set,
            Index,
            Weight,
            UniqueEqual,
            At,
            AtSet
        };

        GlobalConstraintHeadLit(const Loc& loc, GlobalConstraint* gc, ConstraintVarCondPtrVec& vec, Type type) : Lit(loc), vec_(vec.release()), gc_(gc), type_(type)
        {
            for(ConstraintVarCondPtrVec::iterator i = vec_.begin(); i != vec_.end(); ++i)
                i->parent_ = this;
        }


        void enqueue(Grounder *g);
        virtual void add(ConstraintVarCond* add)
        {
            vec_.push_back(add);
        }

        virtual GlobalConstraintHeadLit *clone() const
        {
            ConstraintVarCondPtrVec vec(vec_);
            return new GlobalConstraintHeadLit(loc(), gc_, vec,type_);
        }

        virtual void normalize(Grounder *g, const Lit::Expander& expander);


        virtual bool fact() const
        {
            return false;
        }

        virtual void print(Storage *sto, std::ostream &out) const;
        virtual ::Index *index(Grounder *, Formula *, VarSet &)
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
            //GroundedConstraintVarLitVec vec2;
            //vec2.unique(boost::bind(&GroundedConstraintVarLit::equalTerm,_1,_2));
            //boost::ptr_vector<int> vec2;
            //vec2.unique();
            //#pragma message("Warning, i do sort the whole vector, not only my part !")
            size_t start = vec.size();
            for(ConstraintVarCondPtrVec::iterator i = vec_.begin(); i != vec_.end(); ++i)
                i->getVariables(vec);
            if (type_==Set)
            {
                if(vec.size()) // avoid boost bug
                {
                    vec.sort(vec.begin()+start, vec.end(), boost::bind(&GroundedConstraintVarLit::compareset, _1, _2, s) <0);
                    vec.unique(vec.begin()+start, vec.end(), boost::bind(&GroundedConstraintVarLit::equalTerm,_1,_2));
                }
            }
            else
            if (type_==AtSet)
            {
                vec.sort(vec.begin()+start, vec.end(),(boost::bind(&GroundedConstraintVarLit::compare, _1, _2, s) <0));
                vec.unique(vec.begin()+start, vec.end(),boost::bind(&GroundedConstraintVarLit::equal,_1,_2));
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    if (vec[i].vweight_.type!=Val::NUM)
                        throw CSPException("Only Integer levels allowed in optimize statement.");
                }
            }
            else if (type_==Index)
            {
                vec.sort(vec.begin()+start, vec.end(), (boost::bind(&GroundedConstraintVarLit::compareweight, _1, _2, s) <0));
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    if (i+1<vec.size() && vec[i].vweight_ == vec[i+1].vweight_)
                        if (vec[i].vt_ != vec[i+1].vt_)
                            throw CSPException("Two different elements with same index in GlobalConstraint.");
                }
                //sort(vec.begin(), vec.end(),  (boost::bind(&GroundedConstraintVarLit::compare, _1, _2, s) <0));
                //GroundedConstraintVarLitVec::iterator it = unique(vec.begin(), vec.end(), boost::bind(&GroundedConstraintVarLit::equal,_1,_2));
                vec.unique(vec.begin()+start, vec.end(),boost::bind(&GroundedConstraintVarLit::equal,_1,_2));
                //vec.erase(it,vec.end());

            }
            else if (type_==Weight)
            {
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    if (vec[i].vweight_.type!=Val::NUM)
                        throw CSPException("Only Integer weight allowed.");
                }
            }
            else if (type_==At)
            {
                for (size_t i = 0; i < vec.size(); ++i)
                {
                    if (vec[i].vweight_.type!=Val::NUM)
                        throw CSPException("Only Integer levels allowed in optimize statement.");
                }
            }
            else if (type_==UniqueEqual)
            {
                for (size_t i = 0; i < vec.size(); ++i)
                {
                     if (i+1<vec.size() && vec[i].vweight_ != vec[i+1].vweight_)
                          throw CSPException("Two different elements found, unique one expected.");
                }
            }
            //for(GroundedConstraintVarLitVec::iterator i = vec.begin(); i != vec.end(); ++i)
            //    std::cout << i->vt_.index << std::endl;
        }

    private:
        ConstraintVarCondPtrVec vec_;
        GlobalConstraint*       gc_;
        Type                    type_;
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
            virtual void beginHead(CSPLit::Type cmp) = 0;
            virtual void addHead(GroundedConstraintVarLitVec& vec) = 0;
            virtual void endHead() = 0;
            virtual void end() = 0;
            virtual ~Printer();
        };

        GlobalConstraint(const Loc& loc, GCType name, boost::ptr_vector<ConstraintVarCondPtrVec>& heads, LitPtrVec& body) : SimpleStatement(loc), body_(body.release()), type_(name)
        {
            for (size_t i=0; i < heads.size(); ++i)
            {
                GlobalConstraintHeadLit::Type t;
                switch (type_)
                {
                case DISTINCT:
                {
                    t=GlobalConstraintHeadLit::Set;
                    break;
                }
                case BINPACK:
                {
                    t=GlobalConstraintHeadLit::Index;
                    break;
                }
                case COUNT:
                {
                    t=GlobalConstraintHeadLit::Weight;
                    break;
                }
                case COUNT_UNIQUE:
                {
                    t=GlobalConstraintHeadLit::UniqueEqual;
                    break;
                }
                case COUNT_GLOBAL:
                {
                    if (i==0)
                        t=GlobalConstraintHeadLit::Set;
                    if (i==1)
                        t=GlobalConstraintHeadLit::Index;
                    break;
                }
                case MAXIMIZE_SET:
                case MINIMIZE_SET:
                {
                    t=GlobalConstraintHeadLit::AtSet;
                    break;
                }
                case MAXIMIZE:
                case MINIMIZE:
                {
                    t=GlobalConstraintHeadLit::At;
                    break;
                }
                default: assert(false);

                }

                heads_.push_back(new GlobalConstraintHeadLit(loc, this, heads[i],t));
            }



        }

        GlobalConstraint(const Loc& loc, GCType name, boost::ptr_vector<ConstraintVarCondPtrVec>& heads, CSPLit::Type cmp, LitPtrVec& body) : SimpleStatement(loc), body_(body.release()), type_(name), cmp_(cmp)
        {
            for (size_t i=0; i < heads.size(); ++i)
            {
                GlobalConstraintHeadLit::Type t;
                switch (type_)
                {
                case DISTINCT:
                {
                    t=GlobalConstraintHeadLit::Set;
                    break;
                }
                case BINPACK:
                {
                    t=GlobalConstraintHeadLit::Index;
                    break;
                }
                case COUNT:
                {
                    t=GlobalConstraintHeadLit::Weight;
                    break;
                }
                case COUNT_UNIQUE:
                {
                    t=GlobalConstraintHeadLit::UniqueEqual;
                    break;
                }
                case COUNT_GLOBAL:
                {
                    if (i==0)
                        t=GlobalConstraintHeadLit::Set;
                    if (i==1)
                        t=GlobalConstraintHeadLit::Index;
                    #pragma message("I think this switch has to be fixed, test it")
                    break;
                }
                case MAXIMIZE_SET:
                case MINIMIZE_SET:
                {
                    t=GlobalConstraintHeadLit::AtSet;
                    break;
                }
                case MAXIMIZE:
                case MINIMIZE:
                {
                    t=GlobalConstraintHeadLit::At;
                    break;
                }
                default: assert(false);

                }

                heads_.push_back(new GlobalConstraintHeadLit(loc, this, heads[i],t));
            }

        }


        virtual void visit(PrgVisitor *visitor);

        virtual void print(Storage *sto, std::ostream &out) const;
        virtual void normalize(Grounder *g);
        void         expandHead(Grounder *g, Lit *lit, Lit::ExpansionType type);
                void append(Lit *lit);
        virtual bool grounded(Grounder *g);
        virtual void accept(::Printer *v, Grounder* g);

        ~GlobalConstraint();

        LitPtrVec& body(){return body_;}
    private:
        boost::ptr_vector<GlobalConstraintHeadLit> heads_;
        LitPtrVec    body_;
        GCType       type_;
        CSPLit::Type cmp_;

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
