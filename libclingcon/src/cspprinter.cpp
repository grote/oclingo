#include <clingcon/cspprinter.h>
#include <gringo/litdep.h>


GRINGO_REGISTER_PRINTER(Clingcon::PlainCSPLitPrinter, Clingcon::CSPLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(Clingcon::LParseCSPLitPrinter, Clingcon::CSPLit::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(Clingcon::PlainCSPDomainPrinter, Clingcon::CSPDomainLiteral::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(Clingcon::LParseCSPDomainPrinter, Clingcon::CSPDomainLiteral::Printer, LparseConverter)

