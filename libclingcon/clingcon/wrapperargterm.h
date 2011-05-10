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

namespace Clingcon
{
	class WrapperArgTerm : public WrapperTerm
	{
	public:
		WrapperArgTerm(const Loc &loc, WrapperTerm *term, const WrapperTermPtrVec& terms) : WrapperTerm(loc), term_(term), terms_(terms)
		{
		}


		ConstraintTerm* toConstraintTerm()
		{
			assert("This should not occur"!=0);
			return 0;
		}

		ArgTerm* toTerm() const
		{
			TermPtrVec vec;
			for (WrapperTermPtrVec::const_iterator i = terms_.begin(); i != terms_.end(); ++i)
				vec.push_back(i->toTerm());
			return new ArgTerm(loc_, term_->toTerm(),vec);
		}

		WrapperArgTerm *clone() const
		{
			return new WrapperArgTerm(loc_,term_->clone(),terms_);
		}
	private:
		clone_ptr<WrapperTerm> term_;
	        WrapperTermPtrVec      terms_;
	};
}
