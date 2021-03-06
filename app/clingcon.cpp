// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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

#include "clingcon/clingcon_app.h"
#include <gringo/output.h>
#include <gringo/litdep.h>

int main(int argc, char **argv)
{
	return ClingconApp<CLINGCON>::instance().run(argc, argv);
}


namespace Clingcon
{
    GRINGO_EXPORT_PRINTER(LParseCSPLitPrinter)
    GRINGO_EXPORT_PRINTER(PlainCSPLitPrinter)
    GRINGO_EXPORT_PRINTER(LParseCSPDomainPrinter)
    GRINGO_EXPORT_PRINTER(PlainCSPDomainPrinter)

    //GRINGO_EXPORT_PRINTER(PlainConstraintVarLitPrinter)
    //GRINGO_EXPORT_PRINTER(PlainConstraintVarCondPrinter)
    //GRINGO_EXPORT_PRINTER(PlainConstraintHeadLitPrinter)
    GRINGO_EXPORT_PRINTER(PlainGlobalConstraintPrinter)
    GRINGO_EXPORT_PRINTER(LParseGlobalConstraintPrinter)
}
