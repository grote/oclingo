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

	struct Symbol
	{
		typedef std::pair<uint32_t, ValVec> Repr;

		Symbol(Repr &repr, uint32_t symbol)
			: symbol(symbol)
			, external(false)
			, undefined(false)
		{
			std::swap(this->repr, repr);
		}

		operator uint32_t() const { return symbol; }
		void print(Storage const *s, std::ostream &out) const;

		Repr     repr;
		uint32_t symbol;
		bool     mutable external;
		bool     mutable undefined;
	};

	struct BySymbol { };
	typedef boost::multi_index::multi_index_container
	<
		Symbol, boost::multi_index::indexed_by
		<
			boost::multi_index::hashed_unique<boost::multi_index::member<Symbol, Symbol::Repr const, &Symbol::repr> >,
			boost::multi_index::hashed_unique<boost::multi_index::tag<BySymbol>, boost::multi_index::member<Symbol, uint32_t const, &Symbol::symbol> >
		>
	> SymbolMap;

	typedef std::vector<uint32_t> AtomVec;
	typedef std::vector<int32_t>  WeightVec;
	typedef std::vector<int32_t>  LitVec;
protected:
	struct Minimize
	{
		AtomVec pos;
		AtomVec neg;
		WeightVec wPos;
		WeightVec wNeg;
	};
	typedef std::vector<Symbol const *>                                              NewSymbols;
	typedef boost::unordered_map<ValVec, LitVec>                                     MiniMap;
	typedef std::map<Val, MiniMap, boost::function2<bool, const Val&, const Val &> > PrioMap;
	typedef boost::unordered_map<std::string, std::vector<LitVec> >                  DisplayMap;
	typedef std::vector<std::pair<uint32_t, std::string> >                           ShownSymbols;
public:
	LparseConverter(bool shiftDisj);
	void prioLit(int32_t lit, const ValVec &set, bool maximize);
	Symbol const &symbol(uint32_t symbol);
	Symbol const &symbol(PredLitRep *l);
	uint32_t falseSymbol() const { return false_; }
	virtual void initialize();
	virtual void endModule();
	void finalize();
	void printSymbolTable();
	void printExternalTable();
	void transformDisjunctiveRule(uint32_t n, ...);
	void transformDisjunctiveRule(LitVec const &head, LitVec const &body);
	void addCompute(PredLitRep *l);\
	void printBasicRule(uint32_t head, uint32_t n, ...);
	void printBasicRule(uint32_t head, const LitVec &lits);
	void display(const Val &head, LitVec body, bool show);
	void prepareSymbolTable();
	void prepareExternalTable();
	virtual ~LparseConverter();

public:
	virtual void printBasicRule(uint32_t head, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printConstraintRule(uint32_t head, int32_t bound, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printWeightRule(uint32_t head, int32_t bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg) = 0;
	virtual void printMinimizeRule(const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg) = 0;
	virtual void printDisjunctiveRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printComputeRule(int models, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printSymbolTableEntry(uint32_t symbol, std::string const &name) = 0;
	virtual void printExternalTableEntry(Symbol const &symbol) = 0;
	virtual uint32_t symbol() = 0;
	virtual void doFinalize() = 0;
	virtual uint32_t getVolAtom(int vol_window) { (void) vol_window; return 0; }
protected:
	DisplayMap            atomsHidden_;
	DisplayMap            atomsShown_;
	PrioMap               prioMap_;
	uint32_t              false_;
	bool                  shiftDisjunctions_;
	std::vector<int32_t>  computePos_;
	std::vector<int32_t>  computeNeg_;

	SymbolMap             symbolMap_;
	ShownSymbols          shownSymbols_;
	NewSymbols            newSymbols_;
	uint32_t              newSymbolsDone_;
};
