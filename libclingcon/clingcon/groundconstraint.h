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
                GroundConstraint()
                {}


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
                }
                GroundConstraint(Storage* s, Operator op, GroundConstraint* a, GroundConstraint* b) : op_(op), a_(a), b_(b)
		{}

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
                    }
                }

                void getAllVariables(std::vector<unsigned int>& vec, CSPSolver* csps) const;

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
                                ret += "+"; break;
                            case TIMES:
                                ret += "+"; break;
                            case DIVIDE:
                                ret += "+"; break;
                        }
                        ret += b_->getString()+")";
                        return ret;
                    }

                }

                ~GroundConstraint()
                {
                }
        public:
	Operator op_;
        std::auto_ptr<GroundConstraint> a_;
        std::auto_ptr<GroundConstraint> b_;
        //Val val_;
        int value_;
        std::string name_;
	};
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
