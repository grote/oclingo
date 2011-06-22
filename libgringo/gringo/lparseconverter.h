// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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

#pragma once

#include <gringo/gringo.h>
#include <gringo/output.h>

class LparseConverter : public Output
{
public:
	typedef std::vector<uint32_t> AtomVec;
	typedef std::vector<int32_t>  WeightVec;
	typedef std::vector<int32_t>  LitVec;
	struct ValCmp
	{
		ValCmp(const ValVec *v, uint32_t s);
		size_t operator()(uint32_t i) const;
		bool operator()(uint32_t a, uint32_t b) const;
		const ValVec *vals;
		uint32_t size;
	};
	typedef boost::unordered_map<uint32_t, uint32_t, ValCmp, ValCmp> SymbolMap;
protected:
	typedef std::vector<SymbolMap> SymbolTable;
	struct Minimize
	{
		AtomVec pos;
		AtomVec neg;
		WeightVec wPos;
		WeightVec wNeg;
	};
	struct AtomRef
	{
		AtomRef(uint32_t symbol, uint32_t offset)
			: symbol(symbol)
			, offset(offset) { }
		uint32_t symbol;
		uint32_t offset;
	};

	typedef std::vector<std::vector<AtomRef> >   NewSymbols;
	typedef std::vector<const Domain*>           DomainVec;
	typedef std::vector<bool>                    BoolVec;
	typedef boost::unordered_map<ValVec, LitVec> MiniMap;
	typedef std::map<Val, MiniMap, boost::function2<bool, const Val&, const Val &> >            PrioMap;
	typedef boost::unordered_map<std::pair<uint32_t,uint32_t>, boost::unordered_set<uint32_t> > DisplayMap;
	typedef boost::unordered_map<std::pair<uint32_t,uint32_t>, boost::unordered_set<uint32_t> > ExternalMap;
public:
	LparseConverter(bool shiftDisj);
	void addDomain(Domain *d);
	void prioLit(int32_t lit, const ValVec &set, bool maximize);
	void showAtom(PredLitRep *l);
	void hideAtom(PredLitRep *l);
	uint32_t symbol(PredLitRep *l);
	uint32_t falseSymbol() const { return false_; }
	virtual void initialize();
	virtual void endModule();
	void finalize();
	void externalAtom(PredLitRep *l);
	void printSymbolTable();
	void printExternalTable();
	bool shiftDisjunctions() const { return shiftDisjunctions_; }
	void addCompute(PredLitRep *l);\
	void printBasicRule(uint32_t head, uint32_t n, ...);
	void printBasicRule(uint32_t head, const LitVec &lits);

	virtual ~LparseConverter();
public:
	virtual void printBasicRule(uint32_t head, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printConstraintRule(uint32_t head, int32_t bound, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printWeightRule(uint32_t head, int32_t bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg) = 0;
	virtual void printMinimizeRule(const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg) = 0;
	virtual void printDisjunctiveRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printComputeRule(int models, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printSymbolTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name) = 0;
	virtual void printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name) = 0;
	virtual uint32_t symbol() = 0;
	virtual void doFinalize() = 0;
	virtual int getIncAtom() { return -1; }
protected:
	DisplayMap            atomsHidden_;
	DisplayMap            atomsShown_;
	ExternalMap           atomsExternal_;
	SymbolTable           symTab_;
	PrioMap               prioMap_;
	ValVec                vals_;
	uint32_t              false_;
	NewSymbols            newSymbols_;
	DomainVec             domains_;
	BoolVec               undefined_;
	std::vector<uint32_t> newSymbolsDone_;
	bool                  shiftDisjunctions_;
	std::vector<int32_t>  computePos_;
	std::vector<int32_t>  computeNeg_;
};
