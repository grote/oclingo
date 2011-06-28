// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <gringo/gringo.h>
#include <clingcon/wrapperterm.h>
#include <clingcon/constraintsumterm.h>

namespace Clingcon
{
        class WrapperSumTerm : public WrapperTerm
	{
	public:
                WrapperSumTerm(const Loc &loc, Clingcon::ConstraintVarCondPtrVec* cond) : WrapperTerm(loc), cond_(cond)
		{
		}

                ConstraintSumTerm* toConstraintTerm()
		{
                        return new ConstraintSumTerm(loc_,cond_.release());
		}

                Term* toTerm() const
		{
                    assert(false);
                    return 0;
                        //return new MathTerm(loc_,f_,a_->toTerm(),b_->toTerm());
		}

                WrapperSumTerm *clone() const
		{
                    return new WrapperSumTerm(loc_,new Clingcon::ConstraintVarCondPtrVec(*cond_.get()));
		}
	private:
                clone_ptr<Clingcon::ConstraintVarCondPtrVec> cond_;

	};
}
