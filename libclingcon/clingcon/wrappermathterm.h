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
#include <gringo/mathterm.h>
#include <clingcon/wrapperterm.h>
#include <clingcon/constraintmathterm.h>

namespace Clingcon
{
	class WrapperMathTerm : public WrapperTerm
	{
	public:
		WrapperMathTerm(const Loc &loc, const MathTerm::Func &f, WrapperTerm *a, WrapperTerm *b = 0) : WrapperTerm(loc), f_(f), a_(a), b_(b)
		{
		}

		ConstraintMathTerm* toConstraintTerm()
		{
                        return new ConstraintMathTerm(loc_,f_,a_->toConstraintTerm(),(b_.get() ? b_->toConstraintTerm() : 0));
		}

		MathTerm* toTerm() const
		{
                        return new MathTerm(loc_,f_,a_->toTerm(),(b_.get() ? b_->toTerm() : 0));
		}

		WrapperMathTerm *clone() const
		{
                        return new WrapperMathTerm(loc_,f_,a_->clone(),(b_.get() ? b_->clone() : 0));
		}
	private:
		MathTerm::Func         f_;
		clone_ptr<WrapperTerm> a_;
		clone_ptr<WrapperTerm> b_;
	};
}
