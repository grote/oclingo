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
#include <gringo/sumaggrlit.h>
#include <gringo/minmaxaggrlit.h>
#include <gringo/lparseconverter.h>
#include "output/lparserule_impl.h"

namespace lparseconverter_impl
{

template <uint32_t Type>
class MinMaxAggrLitPrinter : public AggrLit::Printer<MinMaxAggrLit, Type>
{
public:
	MinMaxAggrLitPrinter(LparseConverter *output);
	void begin(State state, bool head, bool sign, bool complete);
	void _begin(State state, bool head, bool sign, bool complete);
	void lower(const Val &l, bool leq);
	void upper(const Val &u, bool leq);
	void end();
	LparseConverter *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
protected:
	LparseConverter   *output_;
	State              state_;
	Val                lower_;
	Val                upper_;
	bool               lleq_;
	bool               uleq_;
	bool               head_;
	bool               sign_;
	bool               complete_;
};

}
