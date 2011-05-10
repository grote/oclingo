#pragma once

#include <gringo/gringo.h>

namespace Clingcon
{
	class ConstraintTerm;
	class ConstraintVarTerm;
	class WrapperTerm;
	typedef boost::ptr_vector<ConstraintTerm> ConstraintTermPtrVec;
	typedef boost::ptr_vector<WrapperTerm> WrapperTermPtrVec;

	ConstraintTerm* new_clone(const ConstraintTerm& a);
	ConstraintVarTerm* new_clone(const ConstraintVarTerm& a);


}



