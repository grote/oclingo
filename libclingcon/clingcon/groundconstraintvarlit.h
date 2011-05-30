
#pragma once
#include <gringo/val.h>

namespace Clingcon
{

    enum GCType
    {
        DISTINCT,
        BINPACK
    };

    struct GroundedConstraintVarLit
    {
        Val vt_;
        Val vweight_;
        bool equalTerm(const GroundedConstraintVarLit &v) const
        {
            return vt_ == v.vt_;// && vweight_ == v.vweight_;
        }
        bool equal(const GroundedConstraintVarLit &v) const
        {
            return vt_ == v.vt_ && vweight_ == v.vweight_;
        }
        int compareset(const GroundedConstraintVarLit &v, Storage *s) const
        {
            return vt_.compare(v.vt_,s);// && vweight_ == v.vweight_;

        }
        int compare(const GroundedConstraintVarLit &v, Storage *s) const
        {
            return vweight_.compare(v.vweight_,s);// && vweight_ == v.vweight_;
        }
        /*int compare(const Val &v) const
        {
            return 1; //vt_.compare(v,s);// && vweight_ == v.vweight_;

        }*/
    };

    typedef std::vector<GroundedConstraintVarLit> GroundedConstraintVarLitVec;


}
