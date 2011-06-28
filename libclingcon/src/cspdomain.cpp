#include "clingcon/cspdomain.h"
#include "gringo/grounder.h"






namespace Clingcon
{
    CSPDomain::CSPDomain(const Loc &loc, CSPDomainLiteral* dom, const LitPtrVec& body) : SimpleStatement(loc),
                                                                                               body_(body),
                                                                                               dom_(dom)
    {
    }
    CSPDomain::CSPDomain(const Loc &loc, CSPDomainLiteral* dom) : SimpleStatement(loc),
                                                                        dom_(dom)
    {
    }


    CSPDomain::~CSPDomain()
    {
    }



    void CSPDomain::expandHead(Grounder* g, Lit *lit, Lit::ExpansionType type)
    {
        switch(type)
        {
        case Lit::RANGE:
        case Lit::RELATION:
            body_.push_back(lit);
            break;
        case Lit::POOL:
            LitPtrVec body;
            foreach(Lit &l, body_) body.push_back(l.clone());
            g->addInternal(new CSPDomain(loc(), static_cast<CSPDomainLiteral*>(lit)->clone(), body));
            break;
        }
    }


    void CSPDomain::visit(PrgVisitor *visitor)
    {
        visitor->visit(dom_.get(),false);
        foreach(Lit &lit, body_)
        { visitor->visit(&lit, true); }
    }

    void CSPDomain::normalize(Grounder *g)
    {
        //CSPDomainHeadExpander headExp(loc(), body_, g);

        dom_->normalize(g, boost::bind(&CSPDomain::expandHead, this, g, _1, _2 ));
        if(body_.size() > 0)
        {
            Lit::Expander bodyExp = boost::bind(&CSPDomain::append, this, _1);
            for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
                    body_[i].normalize(g, bodyExp);
        }
    }

    void CSPDomain::print(Storage *sto, std::ostream &out) const
    {
        dom_->print(sto, out);
        if(body_.size() > 0)
        {
                std::vector<const Lit*> body;
                foreach(const Lit &lit, body_) { body.push_back(&lit); }
                std::sort(body.begin(), body.end(), Lit::cmpPos);
                out << ":-";
                bool comma = false;
                foreach(const Lit *lit, body)
                {
                        if(comma) out << ",";
                        else comma = true;
                        lit->print(sto, out);
                }
        }
        out << ".";
    }

    void CSPDomain::append(Lit *lit)
    {
        body_.push_back(lit);
    }

    bool CSPDomain::grounded(Grounder *g)
    {
        dom_->grounded(g);
        Printer *printer = g->output()->printer<CSPDomainLiteral::Printer>();
        dom_->accept(printer);
        return true;
    }

}
