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
#include <clingcon/constraintfuncterm.h>

namespace Clingcon
{

	class WrapperFuncTerm : public WrapperTerm
	{
	public:
		WrapperFuncTerm(const Loc &loc, const uint32_t& name, const WrapperTermPtrVec &args) : WrapperTerm(loc), name_(name), args_(args)
		{
		}

		ConstraintFuncTerm* toConstraintTerm()
		{
                        TermPtrVec vec;
			for (WrapperTermPtrVec::iterator i = args_.begin(); i != args_.end(); ++i)
                                vec.push_back(i->toTerm());
			return new ConstraintFuncTerm(loc_,name_, vec);
		}

		FuncTerm* toTerm() const
		{
			TermPtrVec vec;
			for (WrapperTermPtrVec::const_iterator i = args_.begin(); i != args_.end(); ++i)
				vec.push_back(i->toTerm());
			return new FuncTerm(loc_,name_,vec);
		}

		WrapperFuncTerm *clone() const
		{
			return new WrapperFuncTerm(loc_,name_, args_);
		}
	private:
		uint32_t                name_;
		WrapperTermPtrVec       args_;
	};
}
