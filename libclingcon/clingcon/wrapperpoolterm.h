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
#include <clingcon/constraintpoolterm.h>

namespace Clingcon
{
	class WrapperPoolTerm : public WrapperTerm
	{
	public:
		WrapperPoolTerm(const Loc &loc, WrapperTerm *a, WrapperTerm *b) : WrapperTerm(loc), a_(a), b_(b)
		{
		}

		//WrapperPoolTerm(const Loc &loc, clone_ptr<WrapperTerm>& a, clone_ptr<WrapperTerm>& b) : WrapperTerm(loc), a_(a), b_(b)
		//{
		//}


		ConstraintPoolTerm* toConstraintTerm()
		{
			return new ConstraintPoolTerm(loc_,a_->toConstraintTerm(),b_->toConstraintTerm());
		}

		PoolTerm* toTerm() const
		{
			return new PoolTerm(loc_, a_->toTerm(),b_->toTerm());
		}

		WrapperPoolTerm *clone() const
		{
			return new WrapperPoolTerm(loc_,a_->clone(),b_->clone());
		}
	private:
		clone_ptr<WrapperTerm> a_;
		clone_ptr<WrapperTerm> b_;
	};
}
