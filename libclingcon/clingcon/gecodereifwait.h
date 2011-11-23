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
#pragma once
#include <clingcon/gecodesolver.h>

using namespace Gecode;
namespace Clingcon {

//template<typename T>
//    class ReifWait : public Propagator {
//    public:
//        typedef void (T::* MemFun)(int);
//    protected:
//      /// View to wait for becoming assigned
//      Int::BoolView x;
//      /// Continuation to execute

//      MemFun c;
//      //void (*c)(Space&);
//      /// Constructor for creation
//      ReifWait(Space& home, Int::BoolView x, MemFun c0, int index);
//      /// Constructor for cloning \a p
//      ReifWait(Space& home, bool shared, ReifWait& p);

//      static T* solver;
//      // index to the boolvar array, index > 0 means boolvar is true,
//      // index < 0 means boolvar is false
//      // if true, index-1 is index
//      // if false, index+1 is index
//      int index_;
//    public:
//      /// Perform copying during cloning
//      virtual Actor* copy(Space& home, bool share);
//      /// Const function (defined as low unary)
//      virtual PropCost cost(const Space& home, const ModEventDelta& med) const;
//      /// Perform propagation
//      virtual ExecStatus propagate(Space& home, const ModEventDelta& med);
//      /// Post propagator that waits until \a x becomes assigned and then executes \a c
//      static ExecStatus post(Space& home, GecodeSolver* csps, Int::BoolView x, MemFun c, int index);
//      /// Delete propagator and return its size
//      virtual size_t dispose(Space& home);
//    };

//    template <class T> T* ReifWait<T>::solver = 0;
//    //GecodeSolver* ReifWait::solver = 0;

//    template<typename T>
//    void reifwait(Home home, T* csps, BoolVar x, ReifWait<T>::MemFun c,
//                  int index, IntConLevel) {
//        if (home.failed()) return;
//        GECODE_ES_FAIL(ReifWait<T>::post(home,csps, x,c, index));
//    }


//    template<class T>
//    ReifWait<T>::ReifWait(Space& home, Int::BoolView x0, MemFun c0, int index)
//        : Propagator(home), x(x0), c(c0), index_(index) {
//        x.subscribe(home,*this,PC_GEN_ASSIGNED);
//    }

//    template<class T>
//    ReifWait<T>::ReifWait(Space& home, bool shared, ReifWait& p)
//        : Propagator(home,shared,p), c(p.c), index_(p.index_) {
//        x.update(home,shared,p.x);
//    }

//    template<class T>
//    Actor*
//    ReifWait<T>::copy(Space& home, bool share) {
//        return new (home) ReifWait(home,share,*this);
//    }
//    template<class T>
//    PropCost
//    ReifWait<T>::cost(const Space&, const ModEventDelta&) const {
//        return PropCost::unary(PropCost::LO);
//    }

//    template<class T>
//    ExecStatus
//    ReifWait<T>::propagate(Space& home, const ModEventDelta&) {
//        assert(x.assigned());
//        if (x.status() == Int::BoolVarImp::ONE)
//            (solver->*c)(index_+1);
//        else
//            (solver->*c)(-index_-1);
//        return home.failed() ? ES_FAILED : home.ES_SUBSUMED(*this);
//    }
//    template<class T>
//    ExecStatus
//    ReifWait<T>::post(Space& home, GecodeSolver* csps, Int::BoolView x, MemFun c, int index) {
//        solver = csps;
//        if (x.assigned()) {
//            if (x.status() == Int::BoolVarImp::ONE)
//                (solver->*c)(index+1);
//            else
//                (solver->*c)(-index-1);
//            return home.failed() ? ES_FAILED : ES_OK;
//        } else {
//            (void) new (home) ReifWait(home,x,c,index);
//            return ES_OK;
//        }
//    }

//    template<class T>
//    size_t
//    ReifWait<T>::dispose(Space& home) {
//        x.cancel(home,*this,PC_GEN_ASSIGNED);
//        (void) Propagator::dispose(home);
//        return sizeof(*this);
//    }


    class ReifWait : public Propagator {
    protected:
      /// View to wait for becoming assigned
      Int::BoolView x;
      /// Continuation to execute

      /// Constructor for creation
      ReifWait(Space& home, Int::BoolView x, Clasp::Var var);
      /// Constructor for cloning \a p
      ReifWait(Space& home, bool shared, ReifWait& p);

      static GecodeSolver* solver;
      // index to the boolvar array, index > 0 means boolvar is true,
      // index < 0 means boolvar is false
      // if true, index-1 is index
      // if false, index+1 is index
      Clasp::Var var_;
    public:
      /// Perform copying during cloning
      virtual Actor* copy(Space& home, bool share);
      /// Const function (defined as low unary)
      virtual PropCost cost(const Space& home, const ModEventDelta& med) const;
      /// Perform propagation
      virtual ExecStatus propagate(Space& home, const ModEventDelta& med);
      /// Post propagator that waits until \a x becomes assigned and then executes \a c
      static ExecStatus post(Space& home, GecodeSolver* csps, Int::BoolView x, Clasp::Var var);
      /// Delete propagator and return its size
      virtual size_t dispose(Space& home);
    };

    GecodeSolver* ReifWait::solver = 0;

    void reifwait(Home home, GecodeSolver* csps, BoolVar x, Clasp::Var var, IntConLevel) {
        if (home.failed()) return;
        GECODE_ES_FAIL(ReifWait::post(home,csps, x, var));
    }


    ReifWait::ReifWait(Space& home, Int::BoolView x0, Clasp::Var var)
        : Propagator(home), x(x0), var_(var) {
        x.subscribe(home,*this,PC_GEN_ASSIGNED);
    }

    ReifWait::ReifWait(Space& home, bool shared, ReifWait& p)
        : Propagator(home,shared,p), var_(p.var_) {
        x.update(home,shared,p.x);
    }

    Actor*
    ReifWait::copy(Space& home, bool share) {
        return new (home) ReifWait(home,share,*this);
    }
    PropCost
    ReifWait::cost(const Space&, const ModEventDelta&) const {
        return PropCost::unary(PropCost::LO);
    }

    ExecStatus
    ReifWait::propagate(Space& home, const ModEventDelta&) {
        assert(x.assigned());
        if (x.status() == Int::BoolVarImp::ONE)
            solver->newlyDerived(Clasp::Literal(var_, false));
        else
            solver->newlyDerived(Clasp::Literal(var_, true));
        return home.failed() ? ES_FAILED : home.ES_SUBSUMED(*this);
    }
    ExecStatus
    ReifWait::post(Space& home, GecodeSolver* csps, Int::BoolView x, Clasp::Var var) {
        solver = csps;
        if (x.assigned()) {
            if (x.status() == Int::BoolVarImp::ONE)
                solver->newlyDerived(Clasp::Literal(var, false));
            else
                solver->newlyDerived(Clasp::Literal(var, true));
            return home.failed() ? ES_FAILED : ES_OK;
        } else {
            (void) new (home) ReifWait(home,x,var);
            return ES_OK;
        }
    }

    size_t
    ReifWait::dispose(Space& home) {
        x.cancel(home,*this,PC_GEN_ASSIGNED);
        (void) Propagator::dispose(home);
        return sizeof(*this);
    }



}

