#pragma once

#include <gringo/gringo.h>


#define CLINGCON_VERSION "2.0.0-beta"

namespace Clingcon
{
	class ConstraintTerm;
        class ConstraintVarCond;
        class ConstraintVarLit;
        class ConstraintMathTerm;
        class ConstraintConstTerm;
        class CSPLit;
	typedef boost::ptr_vector<ConstraintTerm> ConstraintTermPtrVec;
        typedef boost::ptr_vector<ConstraintVarCond> ConstraintVarCondPtrVec;

	ConstraintTerm* new_clone(const ConstraintTerm& a);
        ConstraintVarLit* new_clone(const ConstraintVarLit& a);
        ConstraintMathTerm* new_clone(const ConstraintMathTerm& a);
        ConstraintConstTerm* new_clone(const ConstraintConstTerm& a);
        CSPLit* new_clone(const CSPLit& a);

}



