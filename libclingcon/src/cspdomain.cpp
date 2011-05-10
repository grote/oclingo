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

    namespace
    {
            class CSPDomainHeadExpander : public Expander
            {
            public:
                    CSPDomainHeadExpander(const Loc &loc, LitPtrVec &body, Grounder *g);
                    void expand(Lit *lit, Type type);
            private:
                    const Loc &loc_;
                    LitPtrVec &body_;
                    Grounder  *g_;
            };

            class CSPDomainBodyExpander : public Expander
            {
            public:
                    CSPDomainBodyExpander(LitPtrVec &body);
                    void expand(Lit *lit, Type type);
            private:
                    LitPtrVec &body_;
            };

            CSPDomainHeadExpander::CSPDomainHeadExpander(const Loc &loc, LitPtrVec &body, Grounder *g)
                    : loc_(loc)
                    , body_(body)
                    , g_(g)
            {
            }

            void CSPDomainHeadExpander::expand(Lit *lit, Expander::Type type)
            {
                    switch(type)
                    {
                            case RANGE:
                            case RELATION:
                                    body_.push_back(lit);
                                    break;
                            case POOL:
                                    LitPtrVec body;
                                    foreach(Lit &l, body_) body.push_back(l.clone());
                                    g_->addInternal(new CSPDomain(loc_, static_cast<CSPDomainLiteral*>(lit)->clone(), body));
                                    break;
                    }
            }

            CSPDomainBodyExpander::CSPDomainBodyExpander(LitPtrVec &body)
                    : body_(body)
            {
            }

            void CSPDomainBodyExpander::expand(Lit *lit, Expander::Type type)
            {
                    (void)type;
                    body_.push_back(lit);
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
        CSPDomainHeadExpander headExp(loc(), body_, g);
        dom_->normalize(g, &headExp);
        if(body_.size() > 0)
        {
            CSPDomainBodyExpander bodyExp(body_);
            for(LitPtrVec::size_type i = 0; i < body_.size(); i++)
                    body_[i].normalize(g, &bodyExp);
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
