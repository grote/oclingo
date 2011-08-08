
#include <gringo/grounder.h>
#include <clingcon/constraintvarcond.h>
#include <clingcon/globalconstraint.h>
#include <clingcon/constraintterm.h>
//#include <clingcon/cspprinter.h>
#include <gringo/litdep.h>
#include <clingcon/globalconstraint.h>

namespace Clingcon
{
    void ConstraintVarLit::print(Storage *sto, std::ostream &out) const
    {
        t_->print(sto,out);
        if (weight_)
        {
            out << "[";
            weight_->print(sto,out);
            out << "]";
        }
        else
        {
            out << "$==";
            equal_->print(sto,out);
        }

    }

    void ConstraintVarLit::accept(::Printer *)
    {
        //Printer *printer = v->output()->printer<Printer>();
        //printer->begin(values_);
    }

    void ConstraintVarLit::grounded(Grounder *grounder, ConstraintVarCond* parent)
    {
        GroundedConstraintVarLit* v = new GroundedConstraintVarLit();
        v->vt_.reset(t_->toGroundConstraint(grounder));
        if (weight_)
            v->vweight_ = weight_->val(grounder);
        else
            v->vweight_ = equal_->val(grounder);
        parent->addGround(v);
    }

    void ConstraintVarLit::normalize(Grounder * g, const Expander& e)
    {
        t_->normalize(this,ConstraintTerm::PtrRef(t_), g, e, false);
        if (weight_.get())
            weight_->normalize(this,Term::PtrRef(weight_), g, e, false);
        if (equal_.get())
            equal_->normalize(this,ConstraintTerm::PtrRef(equal_), g, e, false);
    }

    void ConstraintVarLit::visit(PrgVisitor *visitor)
    {
        t_->visitVarTerm(visitor);
        //visitor->visit(t_.get(),false);
        if (weight_)
            visitor->visit(weight_.get(),false);
        else
            equal_->visit(visitor,false);
            //visitor->visit(equal_.get(),false);

    }


    ConstraintVarLit::~ConstraintVarLit(){}

    Container* ConstraintVarCond::parent()
    {
        return parent_;
    }

    void ConstraintVarCond::expandHead(Lit *lit, Lit::ExpansionType type)
    {
            switch(type)
            {
                    case Lit::RANGE:
                    case Lit::RELATION:
                            add(lit);
                            break;
                    case Lit::POOL:
                            LitPtrVec lits;
                            //lits.push_back(lit);
                            lits.insert(lits.end(), this->lits()->begin(), this->lits()->end());
                           // if (*weight())
                            {
                                //clone_ptr<Term> weight(*this->weight());
                                //clone_ptr<ConstraintTerm> head(*this->head());
                                //clone_ptr<ConstraintTerm> head(lit);
                                //this->lit()->add(new ConstraintVarCond(loc(), weight.release(), head.release(), lits));
                                this->parent()->add(new ConstraintVarCond(loc(), dynamic_cast<ConstraintVarLit*>(lit), lits));
                            }
                            //else
                            //{
                                //clone_ptr<ConstraintTerm> equal(*this->equal());
                                //clone_ptr<ConstraintTerm> head(*this->head());
                                //clone_ptr<ConstraintTerm> head(lit);
                                //this->lit()->add(new ConstraintVarCond(loc(), equal.release(), head.release(), lits));
                                //this->lit()->add(new ConstraintVarCond(loc(), equal.release(), head.release(), lits));
                           // }
                            break;
            }
    }


    ConstraintVarCond::ConstraintVarCond(const Loc &loc, Term* weight, ConstraintTerm* first, LitPtrVec &lits) : Formula(loc), head_(new ConstraintVarLit(loc, first, weight)), lits_(lits.release())
    {
    }

    ConstraintVarCond::ConstraintVarCond(const Loc &loc, ConstraintTerm* equal, ConstraintTerm* first, LitPtrVec &lits) : Formula(loc), head_(new ConstraintVarLit(loc, first, equal)), lits_(lits.release())
    {
    }

    ConstraintVarCond::ConstraintVarCond(const Loc &loc, ConstraintVarLit* head, LitPtrVec &lits) : Formula(loc), head_(head), lits_(lits.release())
    {
    }


    void ConstraintVarCond::initInst(Grounder *g)
    {
        if(!inst_.get())
        {
                inst_.reset(new Instantiator(vars(), boost::bind(&ConstraintVarCond::grounded, this, _1)));
                simpleInitInst(g, *inst_);
        }
        if(inst_->init(g)) { enqueue(g); }//nur im Statement enqueue aufrufen
        //inst_->init(g);
    }


    void ConstraintVarCond::enqueue(Grounder *g)
    {
            //parent_->enqueue(g);
    }


    void ConstraintVarCond::ground(Grounder *g)
    {
            inst_->ground(g);
    }



    bool ConstraintVarCond::grounded(Grounder *g)
    {
        head_->grounded(g, this);
        foreach(Lit &lit, lits_)
        {
                lit.grounded(g);
        }
        //foreach(Term& term, set_.terms())
        //{
        //    term.
        //}

        //Printer *printer = g->output()->printer<Printer>();
        //printer->begin(Printer::State(aggr_->aggrUid(), aggr_->domain().lastId()), set);
        //bool head = head_;
        //lit_->accept(printer);
        //foreach(Lit &lit, lits_)
        //{
        //    lit.accept(printer);
                //if(!lit.fact()) { lit.accept(printer); }
                //else if(head)   { printer->trueLit(); }
                //if(head) {
                 //       printer->endHead();
                 //       head = false;
               // }
        //}
        //printer->end();
        return true;

        /*
            bool fact = true;
            foreach(Lit &lit, lits_)
            {
                    lit.grounded(g);
                    if(!lit.fact()) { fact = false; }
            }
            ValVec set;
            foreach(Term &term, *set_.terms()) { set.push_back(term.val(g)); }
            try { aggr_->domain().last()->accumulate(g, &lits_[0], headVars_, *aggr_, set, fact); }
            catch(const Val *val)
            {
                    std::ostringstream oss;
                    oss << "cannot convert ";
                    val->print(g, oss);
                    oss << " to integer";
                    std::string str(oss.str());
                    oss.str("");
                    print(g, oss);
                    throw TypeException(str, StrLoc(g, loc()), oss.str());
            }
            Printer *printer = g->output()->printer<Printer>();
            printer->begin(Printer::State(aggr_->aggrUid(), aggr_->domain().lastId()), set);
            bool head = head_;
            foreach(Lit &lit, lits_)
            {
                    if(!lit.fact()) { lit.accept(printer); }
                    else if(head)   { printer->trueLit(); }
                    if(head) {
                            printer->endHead();
                            head = false;
                    }
            }
            printer->end();
*/
     //      return true;
    }


    void ConstraintVarCond::normalize(Grounder *g)
    {
        head_->normalize(g, boost::bind(&ConstraintVarCond::expandHead, this, _1, _2));

        Lit::Expander bodyExp(boost::bind(&ConstraintVarCond::add, this, _1));
        for(LitPtrVec::size_type i = 0; i < lits_.size(); i++)
        {
            // an cond anhängen
                lits_[i].normalize(g, bodyExp);
        }
        /*
            assert(!lits_.empty());
            bool head = lits_[0].head() || style_ != STYLE_DLV;
            if(head)
            {
                    AggrCondHeadExpander headExp(*this);
                    lits_[0].normalize(g, &headExp);
            }
            CondSetExpander setExp(*this);
            set_.normalize(g, &setExp);
            AggrCondBodyExpander bodyExp(*this);
            for(LitPtrVec::size_type i = head; i < lits_.size(); i++)
            {
                    lits_[i].normalize(g, &bodyExp);
            }
            if(style_ != STYLE_DLV)
            {
                    AnonymousRemover::remove(g, *this);
                    LparseAggrCondConverter::convert(g, *this, number);
            }
            */

    }

    void ConstraintVarCond::visit(PrgVisitor *visitor)
    {
        visitor->visit(head_.get(),false);
        //head_.visit(visitor);
        for(LitPtrVec::size_type i = 0; i < lits_.size(); i++)
        {
                //lits_[i].visit(visitor);
            visitor->visit(&(lits_[i]),false);
        }

    }


    void ConstraintVarCond::print(Storage *sto, std::ostream &out) const
    {
        head_->print(sto,out);
        for(LitPtrVec::size_type i = 0; i < lits_.size(); i++)
        {
            out << ":";
            lits_[i].print(sto,out);
        }
    }


    void ConstraintVarCond::add(Lit *lit)
    {
        lits_.push_back(lit);
    }

    LitPtrVec *ConstraintVarCond::lits()
    {
        return &lits_;
    }

    ConstraintVarCond::~ConstraintVarCond(){}

}
