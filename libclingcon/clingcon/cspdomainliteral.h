

#pragma once


#include "gringo/lit.h"
#include "gringo/index.h"
#include "gringo/rangeterm.h"
#include "gringo/prgvisitor.h"
#include "gringo/printer.h"
#include "gringo/output.h"




namespace Clingcon
{

    class CSPDomainLiteral : public Lit, public Matchable
    {
    public:

        class Printer : public ::Printer
        {
        public:
                virtual void begin(Val lower, Val upper) = 0;
                //virtual void begin(Val t, Val lower, Val upper) = 0;
                virtual ~Printer(){}
        };

        CSPDomainLiteral(const Loc &loc, Term* t, Term* lower, Term* upper) : Lit(loc), Matchable(), t_(t), lower_(lower), upper_(upper)
        {
        }

        virtual void normalize(Grounder *g, const Expander& expander)
        {
            if (t_) t_->normalize(this,Term::PtrRef(t_),g,expander,false);
            lower_->normalize(this,Term::PtrRef(lower_),g,expander,false);
            upper_->normalize(this,Term::PtrRef(upper_),g,expander,false);

        }

        virtual bool fact() const
        {
            return false;
        }

        virtual void print(Storage *sto, std::ostream &out) const
        {
            out << "$domain(";
            if(t_)
            {
              t_->print(sto,out);
              out << ",";
            }
            lower_->print(sto,out);
            out << "..";
            upper_->print(sto,out);
            out << ")";
        }

        virtual void grounded(Grounder *grounder)
        {
            if (t_)
                vt_ = t_->val(grounder);
            vlower_ = lower_->val(grounder);
            vupper_ = upper_->val(grounder);
        }

        virtual Index *index(Grounder *, Formula *, VarSet &)
        {
           return new MatchIndex(this);
        }

        virtual void visit(PrgVisitor *visitor)
        {
             if (t_) visitor->visit(t_.get(),false);
             visitor->visit(lower_.get(),false);
             visitor->visit(upper_.get(),false);
        }

        virtual void accept(::Printer *v)
        {
            Printer *printer = v->output()->printer<Printer>();
            //if (t_)
            //    printer->begin(vt_,vlower_, vupper_);
            //else
                printer->begin(vlower_, vupper_);
        }

        virtual bool match(Grounder *)
        {
            return true;
        }

        CSPDomainLiteral *clone() const
        {
                return new CSPDomainLiteral(*this);
        }

        virtual ~CSPDomainLiteral();

    private:
        clone_ptr<Term> t_;
        clone_ptr<Term> lower_;
        clone_ptr<Term> upper_;

        Val vt_;
        Val vlower_;
        Val vupper_;

    };
}
