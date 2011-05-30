#include <clingcon/cspglobalprinter.h>
#include <gringo/litdep.h>

//GRINGO_REGISTER_PRINTER(Clingcon::PlainConstraintVarLitPrinter, Clingcon::ConstraintVarLit::Printer, PlainOutput)
//GRINGO_REGISTER_PRINTER(Clingcon::PlainConstraintVarCondPrinter, Clingcon::ConstraintVarCond::Printer, PlainOutput)
//GRINGO_REGISTER_PRINTER(Clingcon::PlainConstraintHeadLitPrinter, Clingcon::GlobalConstraintHeadLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(Clingcon::PlainGlobalConstraintPrinter, Clingcon::GlobalConstraint::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(Clingcon::LParseGlobalConstraintPrinter, Clingcon::GlobalConstraint::Printer, LparseConverter)
