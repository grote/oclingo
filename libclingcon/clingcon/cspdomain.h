

#pragma once

#include "gringo/formula.h"
#include "clingcon/cspdomainliteral.h"


namespace Clingcon
{
    class CSPDomain : public SimpleStatement
    {
    public:
        CSPDomain(const Loc &loc, CSPDomainLiteral* lit, const LitPtrVec& body);
        CSPDomain(const Loc &loc, CSPDomainLiteral* lit);

        virtual void visit(PrgVisitor *visitor);

        virtual void normalize(Grounder *g);
        virtual void print(Storage *sto, std::ostream &out) const;
        virtual void append(Lit *lit);
        virtual bool grounded(Grounder *g);
        virtual ~CSPDomain();
    private:
        LitPtrVec                    body_;
        clone_ptr<CSPDomainLiteral>  dom_;
    };

    class CSPDomainException : public std::exception
    {
    public:
        CSPDomainException(const std::string &message)  : message_(message)
        {
        }

        const char *what() const throw()
        {
            return message_.c_str();
        }

        ~CSPDomainException() throw() { }
    private:
            const std::string   message_;
    };

}

