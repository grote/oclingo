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
#include <clingcon/constraintluaterm.h>

namespace Clingcon
{
	class WrapperLuaTerm : public WrapperTerm
	{
	public:
		WrapperLuaTerm(const Loc &loc, uint32_t name, const WrapperTermPtrVec &args) : WrapperTerm(loc), name_(name), args_(args)
		{
		}


		ConstraintLuaTerm* toConstraintTerm()
		{
			ConstraintTermPtrVec vec;
			for (WrapperTermPtrVec::iterator i = args_.begin(); i != args_.end(); ++i)
				vec.push_back(i->toConstraintTerm());
			return new ConstraintLuaTerm(loc_,name_,vec);
		}

		LuaTerm* toTerm() const
		{
			TermPtrVec vec;
			for (WrapperTermPtrVec::const_iterator i = args_.begin(); i != args_.end(); ++i)
				vec.push_back(i->toTerm());
			return new LuaTerm(loc_, name_, vec);
		}

		WrapperLuaTerm *clone() const
		{
			return new WrapperLuaTerm(loc_,name_, args_);
		}
	private:
		uint32_t                name_;
		WrapperTermPtrVec              args_;
	};
}
