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
#ifndef CLASP_MODEL_ENUMERATORS_H
#define CLASP_MODEL_ENUMERATORS_H

#ifdef _MSC_VER
#pragma once
#endif

#include <clasp/include/solve_algorithms.h>
#include <clasp/include/program_builder.h>
#include <clasp/include/clause.h>

namespace Clasp { 

class ModelEnumerator;

//! Interface for printing models
/*!
 * The base class is a "Null-Object".
 */
class ModelPrinter {
public:
	ModelPrinter();
	virtual ~ModelPrinter();
	virtual void printModel(const Solver&, const ModelEnumerator&)  = 0;
	virtual void printReport(const Solver&, const ModelEnumerator&) = 0;
private:
	ModelPrinter(const ModelPrinter&);
	ModelPrinter& operator=(const ModelPrinter&);
};

//! Common base class for model enumeration with minimization and projection
class ModelEnumerator : public Enumerator {
public:
	/*!
	 * \param p the printer to use for outputting results.
	 * \note printer must point to a heap allocated object and is owned by the new ModelEnumerator object!
	 */
	ModelEnumerator(ModelPrinter* p);
	~ModelEnumerator();
	
	/*!
	 * Initializes the enumerator.
	 * \note In the incremental setting, startSearch must be called once for each incremental step
	 * \note If a SAT-Preprocessor is used, startSearch must be called *before* this preprocessor runs,
	 *       ie before s.endAddConstraints() is called.
	 * \param s the solver in which this enumerator is used.
	 * \param index the (optional) atom symbol table of the logic program.
	 * \param project If true, projection is used on the atoms from index that have a non-empty name.
	 * \param c an (optional) minimize constraint. If != NULL, c is used for optimization.
	 */
	void       startSearch(Solver& s, AtomIndex* index, bool project, MinimizeConstraint* c);
	void       setNumModels(uint64 nm);
	bool       backtrackFromModel(Solver& s);
	//! prints a summary
	void       endSearch(Solver& s);
	AtomIndex* getIndex() const { return index_; }
	MinimizeConstraint* getMini() const { return mini_; }
protected:
	virtual void   doStartSearch(Solver&) {}
	virtual bool   doBacktrackFromModel(Solver& s, uint32 bl) = 0;
	virtual uint32 projectLevel(Solver& s) = 0;
	bool           projectionEnabled() const { return !project_.empty(); }
	uint32         numProjectionVars() const { return (uint32)project_.size(); }
	Var            projectVar(uint32 i)const { return project_[i]; }
	bool           backpropagate(Solver& s);
private:
	VarVec              project_;
	uint64              numModels_;
	AtomIndex*          index_;
	MinimizeConstraint* mini_;
	ModelPrinter*       printer_;
};

//! Backtrack based model enumeration
/*!
 * This class enumerates models by maintaining a special backtracking level 
 * and by suppressing certain backjumps. 
 * For normal model enumeration and minimization no extra nogoods are created.
 * For projection, the number of additionally needed nogoods is linear in the number of
 * projection atoms.
 */
class BacktrackEnumerator : public ModelEnumerator {
public:
	/*! \copydoc ModelEnumerator::ModelEnumerator(ModelPrinter*)
	 * \params projectOptions An octal digit specifying the options to use if projection is enabled. 
	 *         The 3-bits of the octal digit have the following meaning:
	 *         - bit 0: use heuristic when selecting a literal from a projection nogood
	 *         - bit 1: enable progress saving after the first solution was found
	 *         - bit 2: minimize backjumps when backtracking from a solution
	 *
	 */
	BacktrackEnumerator(ModelPrinter* p, uint32 projectOptions);
	bool allowRestarts() const { return false; }
private:
	typedef std::pair<Constraint*, uint32>  NogoodPair;
	typedef PodVector<NogoodPair>::type     Nogoods;
	typedef PodVector<uint8>::type          Flags;
	enum ProjectOptions {
		ENABLE_HEURISTIC_SELECT = 1,
		ENABLE_PROGRESS_SAVING  = 2,
		MINIMIZE_BACKJUMPING    = 4
	};
	void    doStartSearch(Solver&);
	bool    doBacktrackFromModel(Solver& s, uint32 bl);
	uint32  projectLevel(Solver& s);
	void    undoLevel(Solver& s);
	uint32  getBacktrackLevel(const Solver& s, uint32 bl) const;
	LitVec  projAssign_;
	Nogoods nogoods_;
	Flags   isProjectVar_;
	uint8   projectOpts_;
};

//! Recording based model enumeration
/*!
 * This class enumerates models by recording nogoods for found solutions.
 * For normal model enumeration and projection (with or without minimization) the
 * number of nogoods is bounded by the number of solutions.
 * For minimization (without projection) nogoods are only recorded if *all* optimal models are to be computed. 
 * In that case, the number of nogoods is bounded by the number of optimal models.
 *
 * If projection is not used, recorded solution nogoods only contain decision literals.
 * Otherwise, recorded solutions contain all projection literals.
 */
class RecordEnumerator : public ModelEnumerator {
public:
	/*! 
	 * \copydoc ModelEnumerator::ModelEnumerator(ModelPrinter*)
	 * \param restartOnModel if true, search is restartet after each model. Otherwise the
	 * solution nogood is used to determine the decision level on which search continues.
	 */
	RecordEnumerator(ModelPrinter* p, bool restartOnModel);
	~RecordEnumerator();
	bool allowRestarts() const { return true; }
private:
	typedef PodVector<Constraint*>::type ConstraintDB;
	bool    doBacktrackFromModel(Solver& s, uint32 bl);
	uint32  projectLevel(Solver& s);
	void    addSolution(Solver& s);
	uint32  assertionLevel(const Solver& s);
	bool    simplify(Solver& s, bool);
	ClauseCreator       solution_;
	ConstraintDB        nogoods_;
	bool                restart_;
};

}
#endif
