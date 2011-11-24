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

#include <gringo/lparseoutput.h>
#include <gringo/predlitrep.h>
#include <gringo/domain.h>
#include <gringo/storage.h>
#include <gringo/rule.h>
#include <gringo/sumaggrlit.h>

#include <stdarg.h>

namespace lparseconverter_impl
{

GRINGO_EXPORT_PRINTER(RulePrinter)
GRINGO_EXPORT_PRINTER(OptimizePrinter)
GRINGO_EXPORT_PRINTER(ComputePrinter)
GRINGO_EXPORT_PRINTER(AggrCondPrinter)
GRINGO_EXPORT_PRINTER(SumAggrLitPrinter)
GRINGO_EXPORT_PRINTER(MinAggrLitPrinter)
GRINGO_EXPORT_PRINTER(MaxAggrLitPrinter)
/*
GRINGO_EXPORT_PRINTER(AvgPrinter)
GRINGO_EXPORT_PRINTER(ParityPrinter)
*/
GRINGO_EXPORT_PRINTER(JunctionLitPrinter)
GRINGO_EXPORT_PRINTER(DisplayPrinter)
GRINGO_EXPORT_PRINTER(ExternalPrinter)
GRINGO_EXPORT_PRINTER(IncPrinter)

}

//////////////////////////// LparseConverter::Symbol ////////////////////////////

void LparseConverter::Symbol::print(const Storage *s, std::ostream &out) const
{
	out << s->string(s->domain(repr.first)->nameId());
	if (!repr.second.empty())
	{
		out << "(";
		bool comma = false;
		foreach (Val const &val, repr.second)
		{
			if (comma) { out << ","; }
			else       { comma = true; }
			val.print(s, out);
		}
		out << ")";
	}

}

//////////////////////////// LparseConverter ////////////////////////////

LparseConverter::LparseConverter(bool shiftDisj)
	: prioMap_(boost::bind(static_cast<int (Val::*)(const Val&, Storage *) const>(&Val::compare), _1, _2, boost::ref(s_)) < 0)
	, shiftDisjunctions_(shiftDisj)
{
	initPrinters<LparseConverter>();
}

void LparseConverter::initialize()
{
	false_ = symbol();
}

LparseConverter::Symbol const &LparseConverter::symbol(uint32_t symbol)
{
	return *symbolMap_.get<BySymbol>().find(symbol);
}

LparseConverter::Symbol const &LparseConverter::symbol(PredLitRep *l)
{
	Symbol::Repr repr(l->dom()->domId(), ValVec());
	repr.second.assign(l->vals().begin(), l->vals().end());
	SymbolMap::iterator it = symbolMap_.find(repr);
	if (it == symbolMap_.end())
	{
		it = symbolMap_.insert(Symbol(repr, symbol())).first;
		newSymbols_.push_back(&*it);
	}
	return *it;
}

void LparseConverter::display(const Val &head, LitVec body, bool show)
{
	std::stringstream ss;
	head.print(storage(), ss);
	if(show) { atomsShown_[ss.str()].push_back(body); }
	else     { atomsHidden_[ss.str()].push_back(body); }
}

void LparseConverter::prioLit(int32_t lit, const ValVec &set, bool maximize)
{
	prioMap_[set[1]][set].push_back(maximize ? -lit : lit);
}

void LparseConverter::prepareSymbolTable()
{
	foreach (Symbol const* symbol, newSymbols_)
	{
		Domain *dom = storage()->domain(symbol->repr.first);
		if (dom->show || (!hideAll_ && !dom->hide))
		{
			std::stringstream ss;
			symbol->print(storage(), ss);
			LitVec lits;
			lits.push_back(symbol->symbol);
			atomsShown_[ss.str()].push_back(lits);
		}
	}

	foreach (DisplayMap::reference ref, atomsShown_)
	{
		uint32_t sym;
		DisplayMap::iterator it = atomsHidden_.find(ref.first);
		if (it != atomsHidden_.end())
		{
			int32_t show;
			int32_t hide;
			if (ref.second.size() != 1 || ref.second.back().size() != 1)
			{
				show = symbol();
				foreach (LitVec &lits, ref.second) { printBasicRule(show, lits); }
			}
			else { show = ref.second.back().back(); }
			if (it->second.size() != 1 || it->second.back().size() != 1)
			{
				hide = symbol();
				foreach (LitVec &lits, it->second) { printBasicRule(hide, lits); }
			}
			else { hide = it->second.back().back(); }
			if (hide != show)
			{
				sym = symbol();
				printBasicRule(sym, 2, show, -hide);

			}
			else { continue; }
		}
		else if(ref.second.size() != 1 || ref.second.back().size() != 1 || ref.second.back().back() < 0)
		{
			sym = symbol();
			foreach (LitVec &lits, ref.second) { printBasicRule(sym, lits); }
		}
		else { sym = ref.second.back().back(); }
		shownSymbols_.push_back(ShownSymbols::value_type(sym, ref.first));
	}
	atomsShown_.clear();
	atomsHidden_.clear();
}

void LparseConverter::printSymbolTable()
{
	foreach (ShownSymbols::value_type &shown, shownSymbols_)
	{
		printSymbolTableEntry(shown.first, shown.second);
	}
}

void LparseConverter::printExternalTable()
{
	foreach (Symbol const *sym, newSymbols_)
	{
		if (sym->external) { printExternalTableEntry(*sym); }
		else if (storage()->domain(sym->repr.first)->external())
		{
			sym->external = true;
			printExternalTableEntry(*sym);
		}
	}
}

void LparseConverter::addCompute(PredLitRep *l)
{
	if(l->sign()) { computeNeg_.push_back(symbol(l)); }
	else          { computePos_.push_back(symbol(l)); }
}

void LparseConverter::finalize()
{
	Output::finalize();
	foreach(PrioMap::value_type &min, prioMap_)
	{
		AtomVec   pos,  neg;
		WeightVec wPos, wNeg;
		foreach(MiniMap::value_type &lits, min.second)
		{
			assert(!lits.second.empty());
			assert(lits.first[0].type == Val::NUM);
			int32_t sym;
			if(lits.second.size() == 1) { sym = lits.second.back(); }
			else
			{
				sym = symbol();
				printBasicRule(sym, lits.second);
			}
			int32_t weight = lits.first[0].num;
			if(weight < 0)
			{
				weight *= -1;
				sym    *= -1;
			}
			if(sym < 0)
			{
				neg.push_back(-sym);
				wNeg.push_back(weight);
			}
			else
			{
				pos.push_back(sym);
				wPos.push_back(weight);
			}
		}
		printMinimizeRule(pos, neg, wPos, wNeg);
	}
	prepareSymbolTable();
	doFinalize();
	shownSymbols_.clear();
	newSymbols_.clear();
}

void LparseConverter::printBasicRule(uint32_t head, uint32_t n, ...)
{
	AtomVec pos, neg;
	va_list vl;
	va_start(vl, n);
	for (uint32_t i = 0; i < n; i++)
	{
		int32_t v = va_arg(vl, int32_t);
		if(v > 0)      { pos.push_back(v); }
		else if(v < 0) { neg.push_back(-v); }
	}
	va_end(vl);
	printBasicRule(head, pos, neg);
}

void LparseConverter::printBasicRule(uint32_t head, const LitVec &lits)
{
	AtomVec pos, neg;
	foreach(int32_t lit, lits)
	{
		if(lit > 0) { pos.push_back(lit); }
		else        { neg.push_back(-lit); }
	}
	printBasicRule(head, pos, neg);
}

void LparseConverter::transformDisjunctiveRule(uint32_t n, ...)
{
	va_list vl;
	va_start(vl, n);
	LitVec head, body;
	for (uint32_t i = 0; i < n; i++) { head.push_back(va_arg(vl, int32_t)); }
	n = va_arg(vl, uint32_t);
	for (uint32_t i = 0; i < n; i++) { body.push_back(va_arg(vl, int32_t)); }
	va_end(vl);
	transformDisjunctiveRule(head, body);
}

void LparseConverter::transformDisjunctiveRule(LitVec const &head, LitVec const &body)
{
	AtomVec pos, neg, atoms;
	foreach (int32_t lit, head)
	{
		if (lit > 0) { atoms.push_back(lit); }
		else
		{
			uint32_t sym = symbol();
			printBasicRule(sym, 1, lit);
			neg.push_back(sym);
		}
	}
	foreach (int32_t lit, body)
	{
		if (lit > 0) { pos.push_back(lit); }
		else         { neg.push_back(lit); }
	}
	boost::range::sort(atoms);
	atoms.resize(boost::range::unique(atoms).size());
	if (!shiftDisjunctions_) { printDisjunctiveRule(atoms, pos, neg); }
	else
	{
		foreach (uint32_t a, atoms)
		{
			foreach (uint32_t b, atoms)
			{
				if (a != b) { neg.push_back(b); }
			}
			printBasicRule(a, pos, neg);
			foreach (uint32_t b, atoms)
			{
				if (a != b) { neg.pop_back(); }
			}
		}
	}
}

LparseConverter::~LparseConverter()
{
}
