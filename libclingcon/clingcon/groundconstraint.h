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
#pragma once

//#include <clasp/include/util/misc_types.h>
#include <gringo/gringo.h>
#include <clingcon/exception.h>


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
                        DIVIDE,
                        MIN,
                        MAX
		};


	public:	
                GroundConstraint() //: a_(0), b_(0)
                {}

                ~GroundConstraint()
                {
                    //delete a_;
                    //delete b_;
                }

                GroundConstraint(const GroundConstraint& copy) : op_(copy.op_),
                                                                 //a_(copy.a_ ? new GroundConstraint(*copy.a_) : 0),
                                                                 //b_(copy.b_ ? new GroundConstraint(*copy.b_) : 0),
                                                                 a_(copy.a_.clone()), // check if necessary
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
                    //a_ = 0;
                    //b_ = 0;
                }
                GroundConstraint(Storage* , Operator op, GroundConstraint* a, GroundConstraint* b) : op_(op)//, a_(a), b_(b)
                {
                    a_.push_back(a); if (b) a_.push_back(b);
                }

                GroundConstraint(Storage* , Operator op, boost::ptr_vector<GroundConstraint>& vec) : op_(op), a_(vec)
                {}

                void print(Storage* s, std::ostream& out)
                {
                    out << getString();
                }

                void simplify()
                {
                    for (boost::ptr_vector<GroundConstraint>::iterator i = a_.begin(); i != a_.end(); ++i)
                        i->simplify();
                    /*if (a_)
                        a_->simplify();
                    if (b_)
                        b_->simplify();
                        */

                    if (op_==ABS && a_[0].isInteger())
                    {
                        op_=INTEGER;
                        value_ = a_[0].getInteger();
                        //delete a_;
                        a_.pop_back();
                        //a_ = 0;
                        return;
                    }

                    bool allInts=a_.size();
                    for (boost::ptr_vector<GroundConstraint>::iterator i = a_.begin(); i != a_.end(); ++i)
                        if (!i->isInteger())
                        {
                            allInts=false;
                            break;
                        }


                    //if (a_.size()>1 && a_[0].isInteger() && b_[1].isInteger())
                    if (allInts)
                    {
                        switch (op_)
                        {
                        case PLUS:
                            value_ = a_[0].getInteger() + a_[1].getInteger(); break;
                        case MINUS:
                            value_ = a_[0].getInteger() - a_[1].getInteger(); break;
                        case TIMES:
                            value_ = a_[0].getInteger() * a_[1].getInteger(); break;
                        case DIVIDE:
                        {
                            if (a_[1].getInteger() == 0)
                                throw CSPException("Error: division by zero detected in CSP term");
                            value_ = a_[0].getInteger() / a_[1].getInteger(); break;
                        }
                        case MIN:
                        {
                            int min = std::numeric_limits<int>::max();
                            for (boost::ptr_vector<GroundConstraint>::iterator i = a_.begin(); i != a_.end(); ++i)
                            {
                                if (i->getInteger() < min)
                                    min = i->getInteger();
                            }
                            value_ = min;
                            break;
                        }
                        case MAX:
                        {
                            int max = std::numeric_limits<int>::min();
                            for (boost::ptr_vector<GroundConstraint>::iterator i = a_.begin(); i != a_.end(); ++i)
                            {
                                if (i->getInteger() > max)
                                    max = i->getInteger();
                            }
                            value_ = max;
                            break;
                        }
                        }
                        op_=INTEGER;
                        a_.release();
                        //delete a_;
                        //delete b_;
                        //a_ = 0;
                        //b_ = 0;
                    }

                }

                int compare(const GroundConstraint &cmp, Storage *s) const
                {
                    if (isOperator() && cmp.isOperator())
                    {
                        if (op_ != cmp.op_)
                            return -1;

                        if (a_.size() != cmp.a_.size())
                            return -1;
                        for (unsigned int i = 0; i != a_.size(); ++i)
                        {

                            int res = a_[i].compare((cmp.a_[i]),s);
                            if (res==0) return 0;
#pragma message "This is strange, please test this"
                        }
                        // this is the way it was before but it does not make sense
                        //int res = a_->compare(*(cmp.a_),s);
                        //return (res != 0 ? b_->compare(*(cmp.b_),s) : res);
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
                        bool ret = false;
                        for (unsigned int i = 0; i != a_.size(); ++i)
                        {
                            if (!(a_[i] == cmp.a_[i]))
                                return false;
                        }
                        return true;
                        //return *a_ == *(cmp.a_) && *b_ == *(cmp.b_);
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
                    case MIN:
                    case MAX:
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
                    case MINUS: return a_[0].getLinearSize() + a_[1].getLinearSize();
                    case TIMES:
                    case DIVIDE:
                    {
                        if (a_[0].op_==INTEGER)
                        {
                           return a_[1].getLinearSize();
                        }
                        else
                        if (a_[1].op_==INTEGER)
                        {
                           return a_[0].getLinearSize();
                        }
                        else
                            return 1;

                    }
                    case MIN:
                    case MAX:
                        return 1; // test this
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
                        return "$abs("+a_[0].getString()+")";

                    }
                    else
                    if (op_==MIN || op_==MAX)
                    {
                        std::string ret;
                        if (op_==MIN)
                            ret+="$min{";
                        if (op_==MAX)
                            ret+="$max{";
                        for (unsigned int i = 0; i != a_.size(); ++i)
                        {
                            ret+=a_[i].getString();
                            if (i<a_.size()-1)
                                ret+=",";
                        }
                        ret+="}";
                        return ret;
                    }
                    else
                    {
                        std::string ret;
                        ret = "("+a_[0].getString();
                        switch (op_)
                        {
                            case PLUS:
                                ret += "$+"; break;
                            case MINUS:
                                ret += "$-"; break;
                            case TIMES:
                                ret += "$*"; break;
                            case DIVIDE:
                                ret += "$/"; break;
                            case VARIABLE:
                            case INTEGER:
                            case ABS:
                            default: assert(false);
                        }
                        ret += a_[1].getString()+")";
                        return ret;
                    }

                }

        public:
	Operator op_;
        boost::ptr_vector<GroundConstraint> a_;
        //GroundConstraint* b_;
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

        typedef boost::ptr_vector<GroundConstraint> GroundConstraintVec;
        typedef boost::ptr_vector<IndexedGroundConstraint> IndexedGroundConstraintVec;

}

