
#pragma once

#include <clingcon/clingcon.h>
#include <clingcon/csplit.h>
#include <gringo/plainoutput.h>
#include <gringo/lparseconverter.h>
#include <src/output/lparserule_impl.h>
#include <clingcon/cspoutputinterface.h>
#include <gringo/lparseconverter.h>
#include <clingcon/cspconstraint.h>
#include <clingcon/cspdomainliteral.h>
#include <clingcon/cspdomain.h>
#include <clingcon/constraintvarcond.h>

#include <iostream>



namespace Clingcon
{
        class PlainCSPLitPrinter : public CSPLit::Printer
	{
	public:
                PlainCSPLitPrinter(PlainOutput *output) : output_(output) { }
                void begin(CSPLit::Type t, const GroundConstraint* a, const GroundConstraint* b)
		{
                    output_->print();
                    printGroundConstraint(a);
                    switch (t)
                    {
                    case CSPLit::ASSIGN:
                    case CSPLit::EQUAL:
                        output_->out() << "$==";
                        break;
                    case CSPLit::GREATER:
                        output_->out() << "$>";
                        break;
                    case CSPLit::LOWER:
                        output_->out() << "$<";
                        break;
                    case CSPLit::GEQUAL:
                        output_->out() << "$>=";
                        break;
                    case CSPLit::LEQUAL:
                        output_->out() << "$<=";
                        break;
                    case CSPLit::INEQUAL:
                        output_->out() << "$!=";
                        break;
                    }
                    printGroundConstraint(b);

		}

                void printGroundConstraint(const GroundConstraint* a)
                {

                    if (a->isVariable())
                    {
                        output_->out() << a->getString();
                        return;
                    }
                    if (a->isInteger())
                    {
                        output_->out() << a->getInteger();
                        return;
                    }

                    if (a->op_ == GroundConstraint::ABS)
                    {
                        output_->out() << "#abs(";
                        printGroundConstraint(a->a_);
                        output_->out() << ")";
                        return;
                    }

                    output_->out() << "(";
                    printGroundConstraint(a->a_);
                    switch (a->op_)
                    {
                        case GroundConstraint::DIVIDE: output_->out() << "/"; break;
                        case GroundConstraint::PLUS: output_->out() << "+"; break;
                        case GroundConstraint::MINUS: output_->out() << "-"; break;
                        case GroundConstraint::TIMES: output_->out() << "*"; break;
                        case GroundConstraint::VARIABLE:
                        case GroundConstraint::INTEGER:
                        case GroundConstraint::ABS:
                        default: assert(false);
                    }
                    printGroundConstraint(a->b_);
                    output_->out() << ")";


                }

		PlainOutput *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
	};

        class LParseCSPLitPrinter : public CSPLit::Printer
        {
        public:


                LParseCSPLitPrinter(LparseConverter *output) : output_(output) { }
                ~LParseCSPLitPrinter()
                {
                    for (ConstraintVec::iterator i = constraints_.begin(); i != constraints_.end(); ++i)
                        delete i->second;
                }

                void begin(CSPLit::Type t, const GroundConstraint* a, const GroundConstraint* b)
                {
                    Constraint* c = new Constraint(t,a,b);

                    lparseconverter_impl::RulePrinter *printer = static_cast<lparseconverter_impl::RulePrinter*>(output_->printer<Rule::Printer>());

                    std::string name = a->getString();
                    switch (t)
                    {
                    case CSPLit::ASSIGN:
                    case CSPLit::EQUAL:
                        name += "$==";
                        break;
                    case CSPLit::GREATER:
                        name += "$>";
                        break;
                    case CSPLit::LOWER:
                        name += "$<";
                        break;
                    case CSPLit::GEQUAL:
                        name += "$>=";
                        break;
                    case CSPLit::LEQUAL:
                        name += "$<=";
                        break;
                    case CSPLit::INEQUAL:
                        name += "$!=";
                        break;
                    }
                    name += b->getString();

                    boost::unordered_map<std::string,unsigned int>::iterator i = map_.find(name);
                    if (i!=map_.end())
                    {
                        printer->addBody(i->second, false);
                        constraints_.push_back(std::make_pair(i->second,c));
                    }
                    else
                    {
                        uint32_t atom = static_cast<CSPOutputInterface*>(output_)->symbol(name,true);
                        map_[name]=atom;
                        printer->addBody(atom, false);
                       constraints_.push_back(std::make_pair(atom,c));
                    }


                }

                 ConstraintVec* getConstraints()
                 {
                     return &constraints_;
                 }

                LparseConverter *output() const { return output_; }
        private:
                LparseConverter *output_;
                boost::unordered_map<std::string,unsigned int> map_;
                ConstraintVec constraints_;

        };

        class PlainCSPDomainPrinter : public CSPDomainLiteral::Printer
        {
        public:
                PlainCSPDomainPrinter(PlainOutput *output) : output_(output) { }
                void begin(Val lower, Val upper)
                {
                    //output_->print();//not sure about this
                    output_->beginRule();
                    output_->out() << "$domain(";
                    lower.print(output_->storage(),output_->out());
                    output_->out() << "..";
                    upper.print(output_->storage(),output_->out());
                    output_->out() << ").\n";
                    output_->endStatement();
                }

                void begin(Val t, Val lower, Val upper)
                {

                    //output_->print();//not sure about this
                    output_->beginRule();
                    output_->out() << "$domain(";
                    t.print(output_->storage(),output_->out());
                    output_->out() << ",";
                    lower.print(output_->storage(),output_->out());
                    output_->out() << "..";
                    upper.print(output_->storage(),output_->out());
                    output_->out() << ").\n";
                    output_->endStatement();
                }

                PlainOutput *output() const { return output_; }
                std::ostream &out() const { return output_->out(); }
        private:
                PlainOutput *output_;
        };



        class LParseCSPDomainPrinter : public CSPDomainLiteral::Printer
        {
        public:

            struct Range
            {
            public:
                Range() : lower(0), upper(0)
                {}
                Range(int l, int u) : lower(l), upper(u)
                {}

                int lower;
                int upper;
            };

            typedef std::vector<Range> Domain;
            typedef boost::unordered_map<std::string,Domain> Domains;




            LParseCSPDomainPrinter(LparseConverter *output) : output_(output)
            {}
            ~LParseCSPDomainPrinter()
            {
                // for (ConstraintVec::iterator i = constraints_.begin(); i != constraints_.end(); ++i)
                //    delete i->second;
            }

            //could use UNDEF to decide on t
            void begin(Val lower, Val upper)
            {
                if (lower.type!=Val::NUM)
                    throw CSPDomainException("Lower bound must be an integer");
                if (upper.type!=Val::NUM)
                    throw CSPDomainException("Upper bound must be an integer");

                default_.push_back(Range(lower.number(), upper.number()));
            }

            void begin(Val t, Val lower, Val upper)
            {

                if (lower.type!=Val::NUM)
                    throw CSPDomainException("Lower bound must be an integer");
                if (upper.type!=Val::NUM)
                    throw CSPDomainException("Upper bound must be an integer");
                if (t.type==Val::NUM)
                    throw CSPDomainException("Variable name may not be an integer");

                std::stringstream ss;
                t.print(output_->storage(), ss);
                std::string name = ss.str();

                //Domains::iterator i = map_.find(name);
                //if (i != map_.end())
                //    std::cerr << "Warning:   Two domains are given for one csp variable, taking the latter one.";
                map_[name].push_back(Range(lower.number(), upper.number()));
            }

            Domains* getDomains()
            {
                return &map_;
            }

            const Domain& getDefaultDomain()
            {
                return default_;
            }

            LparseConverter *output() const { return output_; }
        private:
            LparseConverter *output_;
            Domains map_;
            Domain default_;
            //ConstraintVec constraints_;

        };



}

