
#pragma once
#include <gringo/val.h>
#include <clingcon/groundconstraint.h>

namespace Clingcon
{

    enum GCType
    {
        DISTINCT,
        BINPACK,
        COUNT,
        COUNT_UNIQUE,
        COUNT_GLOBAL,
        MINIMIZE_SET,
        MINIMIZE,
        MAXIMIZE_SET,
        MAXIMIZE
    };

    struct GroundedConstraintVarLit
    {
        GroundedConstraintVarLit() : vt_(0), vweight_(Val::number(0))
        {}
       // GroundedConstraintVarLit(GroundConstraint* vt, const Val& vweight) : vt_(vt), vweight_(vweight)
       // {}


        clone_ptr<GroundConstraint> vt_;
        Val vweight_;

       //GroundedConstraintVarLit *clone() const
        //{
        //    return new GroundedConstraintVarLit(new GroundConstraint(*vt_),vweight_);
        //}

        bool equalTerm(const GroundedConstraintVarLit &v) const
        {
            return *vt_ == *v.vt_;// && vweight_ == v.vweight_;
        }
        bool equal(const GroundedConstraintVarLit &v) const
        {
            return *vt_ == *v.vt_ && vweight_ == v.vweight_;
        }
        int compareset(const GroundedConstraintVarLit &v, Storage *s) const
        {
            return vt_->compare(*v.vt_,s);// && vweight_ == v.vweight_;
        }
        int compareweight(const GroundedConstraintVarLit &v, Storage *s) const
        {
            return vweight_.compare(v.vweight_,s);// && vweight_ == v.vweight_;
        }
        int compare(const GroundedConstraintVarLit &v, Storage *s) const
        {
            int res = vweight_.compare(v.vweight_,s);
            return (res==0) ? vt_->compare(*v.vt_,s) : res;
        }
        /*int compare(const Val &v) const
        {
            return 1; //vt_.compare(v,s);// && vweight_ == v.vweight_;

        }*/
    };

   // inline GroundedConstraintVarLit* new_clone(const GroundedConstraintVarLit& a)
   // {
   //                 return a.clone();
   // }

    typedef boost::ptr_vector<GroundedConstraintVarLit> GroundedConstraintVarLitVec;


}
