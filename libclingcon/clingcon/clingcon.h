#pragma once

#include <gringo/gringo.h>

namespace Clingcon
{
	class ConstraintTerm;
	class ConstraintVarTerm;
	class WrapperTerm;
        class ConstraintVarCond;
        class ConstraintVarLit;
        class ConstraintRangeTerm;
        class ConstraintPoolTerm;
        class ConstraintMathTerm;
        class ConstraintLuaTerm;
        class ConstraintFuncTerm;
        class ConstraintConstTerm;
        class CSPLit;
	typedef boost::ptr_vector<ConstraintTerm> ConstraintTermPtrVec;
	typedef boost::ptr_vector<WrapperTerm> WrapperTermPtrVec;
        typedef boost::ptr_vector<ConstraintVarCond> ConstraintVarCondPtrVec;

	ConstraintTerm* new_clone(const ConstraintTerm& a);
	ConstraintVarTerm* new_clone(const ConstraintVarTerm& a);
        ConstraintVarLit* new_clone(const ConstraintVarLit& a);
        ConstraintRangeTerm* new_clone(const ConstraintRangeTerm& a);
        ConstraintPoolTerm* new_clone(const ConstraintPoolTerm& a);
        ConstraintMathTerm* new_clone(const ConstraintMathTerm& a);
        ConstraintLuaTerm* new_clone(const ConstraintLuaTerm& a);
        ConstraintFuncTerm* new_clone(const ConstraintFuncTerm& a);
        ConstraintConstTerm* new_clone(const ConstraintConstTerm& a);
        CSPLit* new_clone(const CSPLit& a);

}



