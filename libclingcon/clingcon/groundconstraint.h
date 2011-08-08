//
// Copyright (c) 2006-2007, Benjamin Kaufmann
//
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/
//
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#ifndef CLASP_GROUNDCONSTRAINT_H_INCLUDED
#define CLASP_GROUNDCONSTRAINT_H_INCLUDED

//#include <clasp/include/util/misc_types.h>
#include <gringo/gringo.h>


namespace Clingcon {

//	class GroundConstraint
//	{
//		public:
//	};

//	class GroundConstant : public GroundConstraint
//	{
//	public:
//		GroundConstant(const Val& v) : val_(v)
//		{}
//	protected:
//	Val val_;
//	};

        class CSPSolver;

        class GroundConstraint
	{
	public:
		enum Operator
		{
                        VARIABLE,
                        INTEGER,
			ABS,
			PLUS,
			MINUS,
			TIMES,
			DIVIDE
		};


	public:	
                GroundConstraint() : a_(0), b_(0)
                {}

                ~GroundConstraint()
                {
                    delete a_;
                    delete b_;
                }

                GroundConstraint(const GroundConstraint& copy) : op_(copy.op_),
                                                                 a_(copy.a_ ? new GroundConstraint(*copy.a_) : 0),
                                                                 b_(copy.b_ ? new GroundConstraint(*copy.b_) : 0),
                                                                 value_(copy.value_),
                                                                 name_(copy.name_)
                {

                }

                GroundConstraint *clone() const
                {
                   return new GroundConstraint(*this);
                }

                GroundConstraint(Storage* s, const Val& a)
                {
                    if (a.type==Val::NUM)
                    {
                        op_ = INTEGER;
                        value_ = a.number();
                    }
                    else
                    {
                        op_ = VARIABLE;
                        std::stringstream ss;
                        a.print(s,ss);
                        name_ = ss.str();
                    }
                    a_ = 0;
                    b_ = 0;
                }
                GroundConstraint(Storage* , Operator op, GroundConstraint* a, GroundConstraint* b) : op_(op), a_(a), b_(b)
		{}

                void print(Storage* s, std::ostream& out)
                {
                    out << getString();
                }

                void simplify()
                {
                    if (a_)
                        a_->simplify();
                    if (b_)
                        b_->simplify();

                    if (op_==ABS && a_->isInteger())
                    {
                        op_=INTEGER;
                        value_ = a_->getInteger();
                        delete a_;
                        a_ = 0;
                        return;
                    }

#pragma message "avoid dividing by zero"
                    if (a_ && a_->isInteger() && b_ && b_->isInteger())
                    {
                        switch (op_)
                        {
                        case PLUS:
                            value_ = a_->getInteger() + b_->getInteger(); break;
                        case MINUS:
                            value_ = a_->getInteger() - b_->getInteger(); break;
                        case TIMES:
                            value_ = a_->getInteger() * b_->getInteger(); break;
                        case DIVIDE:
                            value_ = a_->getInteger() / b_->getInteger(); break;
                        }
                        op_=INTEGER;
                        delete a_;
                        delete b_;
                        a_ = 0;
                        b_ = 0;
                    }

                }

                int compare(const GroundConstraint &cmp, Storage *s) const
                {
                    if (isOperator() && cmp.isOperator())
                    {
                        if (op_ != cmp.op_)
                            return -1;
                        int res = a_->compare(*(cmp.a_),s);
                        return (res != 0 ? b_->compare(*(cmp.b_),s) : res);
                    }
                    if (isInteger() && cmp.isInteger())
                    {
                        return value_ - cmp.value_;
                    }
                    if (isVariable() && cmp.isVariable())
                    {
                        return name_.compare(cmp.name_);
                    }
                    return -1;
                }

                bool operator==(const GroundConstraint& cmp) const
                {
                    if (isOperator() && cmp.isOperator())
                    {
                        if (op_ != cmp.op_)
                            return false;
                        return *a_ == *(cmp.a_) && *b_ == *(cmp.b_);
                    }
                    if (isInteger() && cmp.isInteger())
                    {
                        return value_ == cmp.value_;
                    }
                    if (isVariable() && cmp.isVariable())
                    {
                        return name_ == cmp.name_;
                    }
                    return false;
                }

                bool operator!=(const GroundConstraint& cmp) const
                {
                    return !(*this==cmp);
                }

                Operator getOperator() const
                {
                    return op_;
                }

                bool isOperator() const
                {
                    switch (op_)
                    {
                    case ABS:
                    case PLUS:
                    case MINUS:
                    case TIMES:
                    case DIVIDE:
                        return true;
                    case VARIABLE:
                    case INTEGER:
                        return false;
                    }
                    return false;
                }

                bool isInteger() const
                {
                    return op_==INTEGER;
                }

                bool isVariable() const
                {
                    return op_==VARIABLE;
                }

                unsigned int getLinearSize() const
                {
                    switch (op_)
                    {
                    case VARIABLE:
                    case INTEGER:
                    case ABS:   return 1;
                    case PLUS:
                    case MINUS: return a_->getLinearSize() + b_->getLinearSize();
                    case TIMES:
                    case DIVIDE:
                    {
                        if (a_->op_==INTEGER)
                        {
                           return b_->getLinearSize();
                        }
                        else
                        if (b_->op_==INTEGER)
                        {
                           return a_->getLinearSize();
                        }
                        else
                            return 1;

                    }
                    default: assert(false);
                    }
                    assert(false);
                    return 0;
                }

                void getAllVariables(std::vector<unsigned int>& vec, CSPSolver* csps) const;
                void registerAllVariables(CSPSolver* csps) const;

                int getInteger() const
                {
                    return value_;
                }


                std::string getString() const
                {
                    if (op_==VARIABLE)
                        return name_;
                    if (op_==INTEGER)
                    {
                        std::stringstream ss;
                        ss << value_;
                        //val_.print(s,ss);
                        return ss.str();
                    }
                    else
                    if (op_==ABS)
                    {
                        return "#abs("+a_->getString()+")";

                    }
                    else
                    {
                        std::string ret;
                        ret = "("+a_->getString();
                        switch (op_)
                        {
                            case PLUS:
                                ret += "+"; break;
                            case MINUS:
                                ret += "-"; break;
                            case TIMES:
                                ret += "*"; break;
                            case DIVIDE:
                                ret += "/"; break;
                            case VARIABLE:
                            case INTEGER:
                            case ABS:
                            default: assert(false);
                        }
                        ret += b_->getString()+")";
                        return ret;
                    }

                }

        public:
	Operator op_;
        GroundConstraint* a_;
        GroundConstraint* b_;
        //Val val_;
        int value_;
        std::string name_;
	};

        struct IndexedGroundConstraint
        {
        public:
            IndexedGroundConstraint(Storage* s, GroundConstraint* a, const Val& b) : a_(a), b_(s,b)
            {

            }

            clone_ptr<GroundConstraint> a_;
            GroundConstraint b_;

        };

        inline GroundConstraint* new_clone(const GroundConstraint& a)
        {
           return a.clone();
        }

//        class IndexedGroundConstraint : public GroundConstraint
//        {
//        public:
//            IndexedGroundConstraint(Storage* s, const Val& a, const Val& weight) : GroundConstraint(s,a)
//            {
//                std::stringstream ss;
//                weight.print(s,ss);
//                weightname_ = ss.str();
//            }

//           // IndexedGroundConstraint(Storage* s, Operator op, GroundConstraint* a, GroundConstraint* b, const) : GroundConstraint(s, op, a, b)

//            std::string getWeight(){return weightname_;}

//        private:

//            std::string weightname_;

//        };

        typedef boost::ptr_vector<GroundConstraint> GroundConstraintVec;
        typedef boost::ptr_vector<IndexedGroundConstraint> IndexedGroundConstraintVec;
/*
		public:
			enum Type
			{
				VARIABLE,
				INTEGER,
				OPERATOR,
				RELATION,
				UNDEF

			};

			enum Relation
			{
				EQ,
				NE,
				LE,
				LT,
				GE,
				GT
			};

			CSPConstraint();
			CSPConstraint(const CSPConstraint& cc);
			const CSPConstraint operator=(const CSPConstraint& cc);
			~CSPConstraint();
			void setVariable(unsigned int var);
			void setInteger(int i);
			void setOperator(Operator op, CSPConstraint* a, CSPConstraint* b);
			void setRelation(Relation op, CSPConstraint* a, CSPConstraint* b);

			unsigned int getLinearSize() const;


			Type getType() const;
			unsigned int getVar() const;
			int getInteger() const;
			Operator getOperator(CSPConstraint*& a, CSPConstraint*& b) const;
			Relation getRelation(CSPConstraint*& a, CSPConstraint*& b) const;

			std::vector<unsigned int> getAllVariables();

		private:
			void clear();
			Type type_;
			union
			{
				unsigned int var_;
				int integer_;
				Operator op_;
				Relation rel_;
			};
			CSPConstraint* a_;
			CSPConstraint* b_;
			unsigned int lin_; // lenght of the linear component, 1 if not linear (constant, variable, mult)

	};
	*/
}
#endif
