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

#include <clasp/include/constraint.h>
#include <stdexcept>

namespace Clasp {
Constraint::Constraint() {}
Constraint::~Constraint() {}

LearntConstraint::~LearntConstraint() {}

bool Constraint::simplify(Solver&, bool) {
	return false;
}

void Constraint::destroy() {
	delete this;
}

void Constraint::undoLevel(Solver&) {}

LearntConstraint::LearntConstraint() {}
uint32 LearntConstraint::activity() const {
	return 0;
}
void LearntConstraint::decreaseActivity() { }

bool Antecedent::checkPlatformAssumptions() {
	int32* i = new int32(22);
	uint64 p = (uint64)(uintp)i;
	bool convOk = ((int32*)(uintp)p) == i;
	bool alignmentOk = (p & 3) == 0;
	delete i;
	if ( !alignmentOk ) {
		throw std::runtime_error("Unsupported Pointer-Alignment!");
	}
	if ( !convOk ) {
		throw std::runtime_error("Unsupported Platform: Can't convert between Pointer and Integer!");
	}
	Literal max = posLit(varMax-1);
	Antecedent a(max);
	if (a.type() != Antecedent::binary_constraint || a.firstLiteral() != max) {
		throw std::runtime_error("Unsupported Platform: Cast between 64- and 32-bit integer does not work as expected!");
	}
	Antecedent b(max, ~max);
	if (b.type() != Antecedent::ternary_constraint || b.firstLiteral() != max || b.secondLiteral() != ~max) {
		throw std::runtime_error("Unsupported Platform: Cast between 64- and 32-bit integer does not work as expected!");
	}
	return true;
}

}
