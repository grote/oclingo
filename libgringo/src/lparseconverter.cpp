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
GRINGO_EXPORT_PRINTER(JunctionPrinter)
GRINGO_EXPORT_PRINTER(ShowPrinter)
GRINGO_EXPORT_PRINTER(ExternalPrinter)
GRINGO_EXPORT_PRINTER(IncPrinter)

}

LparseConverter::ValCmp::ValCmp(const ValVec *v, uint32_t s) :
	vals(v),
	size(s)
{
}

size_t LparseConverter::ValCmp::operator()(uint32_t i) const
{
	return boost::hash_range(vals->begin() + i, vals->begin() + i + size);
}

bool LparseConverter::ValCmp::operator()(uint32_t a, uint32_t b) const
{
	return std::equal(vals->begin() + a, vals->begin() + a + size, vals->begin() + b);
}

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

void LparseConverter::addDomain(Domain *d)
{
	symTab_.push_back(SymbolMap(0, ValCmp(&vals_, d->arity()), ValCmp(&vals_, d->arity())));
	domains_.push_back(d);

}

uint32_t LparseConverter::symbol(PredLitRep *l)
{
	uint32_t domId = l->dom()->domId();
	uint32_t size  = vals_.size();
	std::copy(l->vals().begin(), l->vals().end(), std::back_insert_iterator<ValVec>(vals_));
	std::pair<SymbolMap::iterator, bool> res = symTab_[domId].insert(SymbolMap::value_type(size, 0));
	if(res.second)
	{
		uint32_t sym = symbol();
		res.first->second = sym;
		if(newSymbols_.size() <= domId) { newSymbols_.resize(domId + 1); }
		newSymbols_[domId].push_back(AtomRef(sym, size));
	}
	else
	{
		// spend a new symbol if the symbol itself did not occur in a head yet
		uint32_t sym = res.first->second;
		if(sym < undefined_.size() && undefined_[sym] && !l->dom()->external())
		{
			sym = symbol();
			res.first->second = sym;
			if(newSymbols_.size() <= domId) { newSymbols_.resize(domId + 1); }
			newSymbols_[domId].push_back(AtomRef(sym, res.first->first));
		}
		vals_.resize(size);
	}
	return res.first->second;
}

void LparseConverter::showAtom(PredLitRep *l)
{
	atomsShown_[DisplayMap::key_type(l->dom()->nameId(), l->dom()->arity())].insert(symbol(l));
}

void LparseConverter::hideAtom(PredLitRep *l)
{
	atomsHidden_[DisplayMap::key_type(l->dom()->nameId(), l->dom()->arity())].insert(symbol(l));
}

void LparseConverter::prioLit(int32_t lit, const ValVec &set, bool maximize)
{
	prioMap_[set[1]][set].push_back(maximize ? -lit : lit);
}

void LparseConverter::printSymbolTable()
{
	for(DomainMap::const_iterator i = s_->domains().begin(); i != s_->domains().end(); ++i)
	{
		uint32_t nameId = i->second->nameId();
		uint32_t arity  = i->second->arity();
		uint32_t domId  = i->second->domId();
		DisplayMap::iterator itShown = atomsShown_.find(DisplayMap::key_type(nameId, arity));
		bool globShow = shown(nameId, arity);
		if(globShow || itShown != atomsShown_.end())
		{
			DisplayMap::iterator hidden = atomsHidden_.find(DisplayMap::key_type(nameId, arity));
			const std::string &name = s_->string(nameId);
			if(newSymbols_.size() <= domId) newSymbols_.resize(domId + 1);
			foreach(AtomRef &j, newSymbols_[domId])
			{
				if((globShow && (hidden == atomsHidden_.end() || hidden->second.find(j.symbol) == hidden->second.end())) ||
				   (!globShow && itShown != atomsShown_.end() && itShown->second.find(j.symbol) != itShown->second.end()))
				{
					printSymbolTableEntry(j, arity, name);
				}
			}
		}
	}
}

void LparseConverter::printExternalTable()
{
	if(external_.size() > 0 || atomsExternal_.size() > 0)
	{
		for(DomainMap::const_iterator i = s_->domains().begin(); i != s_->domains().end(); ++i)
		{
			uint32_t nameId = i->second->nameId();
			uint32_t arity  = i->second->arity();
			uint32_t domId  = i->second->domId();
			ExternalMap::iterator ext = atomsExternal_.find(Signature(nameId, arity));
			bool globExt = external_.find(Signature(nameId, arity)) != external_.end();
			if(globExt || ext != atomsExternal_.end())
			{
				if(newSymbols_.size() <= domId) newSymbols_.resize(domId + 1);
				const std::string &name = s_->string(nameId);
				foreach(AtomRef &j, newSymbols_[domId])
				{
					if(globExt || ext->second.find(j.symbol) != ext->second.end())
						printExternalTableEntry(j, arity, name);
				}
			}
		}
	}
}

void LparseConverter::externalAtom(PredLitRep *l)
{
	atomsExternal_[ExternalMap::key_type(l->dom()->nameId(), l->dom()->arity())].insert(symbol(l));
}

void LparseConverter::addCompute(PredLitRep *l)
{
	if(l->sign()) computeNeg_.push_back(symbol(l));
	else computePos_.push_back(symbol(l));
}

void LparseConverter::endModule()
{
	newSymbolsDone_.resize(newSymbols_.size());
	for(uint32_t domId = 0; domId < newSymbols_.size(); domId++)
	{
		const Domain *dom = domains_[domId];
		for(std::vector<AtomRef>::iterator it = newSymbols_[domId].begin() + newSymbolsDone_[domId]; it != newSymbols_[domId].end(); it++)
		{
			// mark symbols that do not appear in any head
			if(!dom->find(vals_.begin() + it->offset).valid())
			{
				if(undefined_.size() <= it->symbol) { undefined_.resize(it->symbol + 1, false); }
				undefined_[it->symbol] = true;
			}
		}
	}
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
	doFinalize();
	newSymbols_.clear();
	newSymbolsDone_.clear();
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

LparseConverter::~LparseConverter()
{
}

