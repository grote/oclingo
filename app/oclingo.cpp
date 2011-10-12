// Copyright (c) 2010, Torsten Grote <tgrote@uni-potsdam.de>
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

#include "oclingo/oclingo_app.h"

template <>
void FromGringo<OCLINGO>::otherOutput()
{
	out.reset(new oClaspOutput(grounder.get(), solver, app.gringo.disjShift, app.oclingo.online.port, app.oclingo.online.import));
}



int main(int argc, char **argv)
{
	return ClingoApp<OCLINGO>::instance().run(argc, argv);
}
