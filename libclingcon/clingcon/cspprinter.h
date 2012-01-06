
#pragma once

#include <clingcon/clingcon.h>
#include <clingcon/csplit.h>
#include <gringo/plainoutput.h>
#include <gringo/lparseconverter.h>
#include <src/output/lparserule_impl.h>
#include <clingcon/cspoutputinterface.h>
#include <gringo/lparseconverter.h>
#include <clingcon/cspdomainliteral.h>
#include <clingcon/cspdomain.h>
#include <clingcon/interval.h>
#include <clingcon/cspconstraint.h>

#include <iostream>



namespace Clingcon
{
        class PlainCSPLitPrinter : public CSPLit::Printer
	{
	public:
                PlainCSPLitPrinter(PlainOutput *output) : output_(output) { }

                void start()
                {
                    if (stack_.size()==0)
                        output_->print();
                }
                void normal(CSPLit::Type t, const GroundConstraint* a, const GroundConstraint* b)
		{
                    std::stringstream ss;

                    printGroundConstraint(ss,a);
                    switch (t)
                    {
                    case CSPLit::ASSIGN:
                    case CSPLit::EQUAL:
                        ss << "$==";
                        break;
                    case CSPLit::GREATER:
                        ss << "$>";
                        break;
                    case CSPLit::LOWER:
                        ss << "$<";
                        break;
                    case CSPLit::GEQUAL:
                        ss << "$>=";
                        break;
                    case CSPLit::LEQUAL:
                        ss << "$<=";
                        break;
                    case CSPLit::INEQUAL:
                        ss << "$!=";
                        break;
                    default: assert(("A simple constraint should always have a comparision operator", false));
                    }
                    printGroundConstraint(ss,b);

                    stack_.push_back(ss.str());

		}

                virtual void open()
                {
                   // ss_ << "(";
                }

                virtual void conn(CSPLit::Type t)
                {
                    switch(t)
                    {
                    case CSPLit::AND: stack_.push_back(" $and "); break;
                    case CSPLit::OR:  stack_.push_back(" $or ");  break;
                    case CSPLit::XOR: stack_.push_back(" $xor "); break;
                    case CSPLit::EQ:  stack_.push_back(" $eq ");  break;
                    default: assert(("A connective should be used as a connective", false));
                    }

                }

                virtual void close()
                {
                   std::stringstream ss;
                   ss << "(";
                   ss << stack_[stack_.size()-2];
                   ss << stack_[stack_.size()-3];
                   ss << stack_[stack_.size()-1];
                   ss << ")";
                   stack_.pop_back();
                   stack_.pop_back();
                   stack_.back() = ss.str();
                }

                void end()
                {
                    if (stack_.size()==1)
                    {
                        output_->out() << stack_.back();
                        stack_.pop_back();
                    }
                }

                void printGroundConstraint(std::stringstream& ss, const GroundConstraint* a)
                {

                    if (a->isVariable())
                    {
                        ss << a->getString();
                        return;
                    }
                    if (a->isInteger())
                    {
                        ss << a->getInteger();
                        return;
                    }

                    if (a->op_ == GroundConstraint::ABS)
                    {
                        ss << "$abs(";
                        printGroundConstraint(ss, &(a->a_[0]));
                        ss << ")";
                        return;
                    }

                    if (a->op_ == GroundConstraint::MIN || a->op_ == GroundConstraint::MAX)
                    {
                        if (a->op_ == GroundConstraint::MIN)
                            ss << "$min{";
                        if (a->op_ == GroundConstraint::MAX)
                            ss << "$max{";
                        for (size_t i = 0; i < a->a_.size(); ++i)
                        {
                            printGroundConstraint(ss, &(a->a_[i] ));
                            if (i+1 < a->a_.size())
                                ss << ",";
                        }
                        ss << "}";
                        return;
                    }

                    ss << "(";
                    printGroundConstraint(ss, &(a->a_[0]));
                    switch (a->op_)
                    {
                        case GroundConstraint::DIVIDE: ss << "$/"; break;
                        case GroundConstraint::PLUS:   ss << "$+"; break;
                        case GroundConstraint::MINUS:  ss << "$-"; break;
                        case GroundConstraint::TIMES:  ss << "$*"; break;
                        case GroundConstraint::VARIABLE:
                        case GroundConstraint::INTEGER:
                        case GroundConstraint::ABS:
                        default: assert(false);
                    }
                    printGroundConstraint(ss, &(a->a_[1]));
                    ss << ")";
                }

		PlainOutput *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
                std::vector<std::string> stack_;
	};

        class LParseCSPLitPrinter : public CSPLit::Printer
        {
        public:


            LParseCSPLitPrinter(LparseConverter *output) : output_(output) { }
            ~LParseCSPLitPrinter()
            {
                //for (ConstraintVec::iterator i = constraints_.begin(); i != constraints_.end(); ++i)
                //    delete i->second;
            }

            void start()
            {
            }

            void normal(CSPLit::Type t, const GroundConstraint* a, const GroundConstraint* b)
            {
                Constraint* c = new Constraint(t,a,b);
                stack_.push_back(c);

                std::stringstream ss;
                ss << a->getString();
                switch (t)
                {
                case CSPLit::ASSIGN:
                case CSPLit::EQUAL:
                    ss << "$==";
                    break;
                case CSPLit::GREATER:
                    ss << "$>";
                    break;
                case CSPLit::LOWER:
                    ss << "$<";
                    break;
                case CSPLit::GEQUAL:
                    ss << "$>=";
                    break;
                case CSPLit::LEQUAL:
                    ss << "$<=";
                    break;
                case CSPLit::INEQUAL:
                    ss << "$!=";
                    break;
                default: assert(("A simple constraint should always have a comparision operator", false));
                }
                ss << b->getString();

                name_.push_back(ss.str());

            }

            virtual void open(){}

            virtual void conn(CSPLit::Type t)
            {
                switch(t)
                {
                case CSPLit::AND: name_.push_back(" $and "); break;
                case CSPLit::OR:  name_.push_back(" $or ");  break;
                case CSPLit::XOR: name_.push_back(" $xor "); break;
                case CSPLit::EQ:  name_.push_back(" $eq ");  break;
                default: assert(("A connective should be used as a connective", false));
                }
                stack_.push_back(new Constraint(t));

            }

            virtual void close()
            {
                std::stringstream ss;
                ss << "(";
                ss << name_[name_.size()-2];
                ss << name_[name_.size()-3];
                ss << name_[name_.size()-1];
                ss << ")";
                name_.pop_back();
                name_.pop_back();
                name_.back() = ss.str();

                stack_[stack_.size()-3]->add(stack_[stack_.size()-2]);
                stack_[stack_.size()-3]->add(stack_[stack_.size()-1]);
                stack_.pop_back();
                stack_.pop_back();
            }


            void end()
            {
                if (stack_.size()==1)
                //if (counter_==0)
                {
                    lparseconverter_impl::RulePrinter *printer = static_cast<lparseconverter_impl::RulePrinter*>(output_->printer<Rule::Printer>());

#pragma i could use a constraint for mapping instead of string
                    boost::unordered_map<std::string,unsigned int>::iterator i = map_.find(name_[0]);
                    if (i!=map_.end())
                    {
                        printer->addBody(i->second, false);
                        constraints_.push_back(std::make_pair(i->second,stack_[0]));
                    }
                    else
                    {
                        uint32_t atom = static_cast<CSPOutputInterface*>(output_)->symbol(name_[0],true);
                        map_[name_[0]]=atom;
                        printer->addBody(atom, false);
                        constraints_.push_back(std::make_pair(atom,stack_[0]));
                    }
                    name_.pop_back();
                    stack_.pop_back();
                }
            }

            ConstraintVec* getConstraints()
            {
                return &constraints_;
            }

            LparseConverter *output() const { return output_; }
        private:
            LparseConverter *output_;
            //unsigned int counter_;
            boost::unordered_map<std::string,unsigned int> map_;
            ConstraintVec constraints_;
            std::vector<Constraint*> stack_;
            std::vector<std::string> name_;

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

                /*void begin(Val t, Val lower, Val upper)
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
                }*/

                PlainOutput *output() const { return output_; }
                std::ostream &out() const { return output_->out(); }
        private:
                PlainOutput *output_;
        };



        class LParseCSPDomainPrinter : public CSPDomainLiteral::Printer
        {
        public:

            typedef Interval<int> Domain;


            LParseCSPDomainPrinter(LparseConverter *output) : output_(output), default_(std::numeric_limits<int>::min()+2, std::numeric_limits<int>::max()-1)
            {
            }
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

                default_ = Domain(lower.number(), upper.number());
            }

            /*void begin(Val t, Val lower, Val upper)
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
                map_[name].push_back(Domain(lower.number(), upper.number()));
            }*/


            const Domain& getDefaultDomain()
            {
                return default_;
            }

            LparseConverter *output() const { return output_; }
        private:
            LparseConverter *output_;
            Domain default_;
            //ConstraintVec constraints_;

        };



}

