// Copyright (c) 2008, Roland Kaminski
//
// This file is part of GrinGo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

#include "smodelsconverter.h"
#include "grounder.h"

using namespace NS_GRINGO;
using namespace NS_OUTPUT;

SmodelsConverter::SmodelsConverter(std::ostream *out) : Output(out), negBoundsWarning_(true)
{
}

void SmodelsConverter::initialize(GlobalStorage *g, SignatureVector *pred)
{
	Output::initialize(g, pred);
	// should get 1 here
	false_            = newUid();
}

int SmodelsConverter::getFalse() const
{
	return false_;
}

void SmodelsConverter::handleHead(Object *o)
{
	// no negations in the haed
	assert(!o->neg_);
	if(dynamic_cast<Atom*>(o))
	{
		Atom *head = static_cast<Atom*>(o);
		addAtom(head);
		assert(head->getUid() > 0);
		printBasicRule(head->getUid(), pos_, neg_);
	}
	else if(dynamic_cast<Aggregate*>(o))
	{
		Aggregate *head = static_cast<Aggregate*>(o);
		printHead(head);
		for(ObjectVector::iterator it = head->lits_.begin(); it != head->lits_.end(); it++)
		{
			assert(dynamic_cast<Atom*>(*it));
			addAtom(static_cast<Atom*>(*it));
			if((*it)->getUid() > 0)
				head_.push_back((*it)->getUid());
		}
		if(head_.size() > 0)
			printChoiceRule(head_, pos_, neg_);
	}
	else if(dynamic_cast<Disjunction*>(o))
	{
		Disjunction *head = static_cast<Disjunction*>(o);
		for(ObjectVector::iterator it = head->lits_.begin(); it != head->lits_.end(); it++)
		{
			assert(dynamic_cast<Atom*>(*it));
			addAtom(static_cast<Atom*>(*it));
			assert((*it)->getUid() > 0);
			head_.push_back((*it)->getUid());
		}
		assert(head_.size() > 0);
		printDisjunctiveRule(head_, pos_, neg_);
	}
	else
	{
		assert(false);
	}
}

void SmodelsConverter::handleBody(ObjectVector &body)
{
	for(ObjectVector::iterator it = body.begin(); it != body.end(); it++)
	{
		if(dynamic_cast<Atom*>(*it))
		{
			Atom *a = static_cast<Atom*>(*it);
			addAtom(a);
			int uid = a->getUid();
			if(uid > 0)
				pos_.push_back(uid);
			else
				neg_.push_back(-uid);
		}
		else if(dynamic_cast<Aggregate*>(*it))
		{
			printBody(static_cast<Aggregate*>(*it));
		}
		else if(dynamic_cast<DeltaObject*>(*it))
		{
			pos_.push_back(getIncUid());
		}
		else
		{
			assert(false);
		}
	}
}

void SmodelsConverter::printHead(Aggregate *a)
{
	negA_.clear();
	posA_.clear();
	wNegA_.clear();
	wPosA_.clear();
	int l, u;
	switch(a->type_)
	{
		case Aggregate::COUNT:
			handleCount(a, l, u);
			break;
		case Aggregate::SUM:
			handleSum(false, a, l, u);
			break;
		case Aggregate::MAX:
			handleMax(a, l, u);
			break;
		case Aggregate::MIN:
			handleMin(a, l, u);
			break;
		case Aggregate::AVG:
			handleAvg(false, a, l, u);
			break;
		case Aggregate::PARITY:
			u = 0;
			handleParity(true, a, l);
			break;
		default:
			assert(false);
			break;
	}
	if(l > 0)
	{
		neg_.push_back(l);
		printBasicRule(false_, pos_, neg_);
		neg_.pop_back();
	}
	if(u > 0)
	{
		pos_.push_back(u);
		printBasicRule(false_, pos_, neg_);
		pos_.pop_back();
	}
}

void SmodelsConverter::handleAggregate(ObjectVector &lits)
{
	for(ObjectVector::iterator it = lits.begin(); it != lits.end(); it++)
	{
		assert(dynamic_cast<Atom*>(*it));
		Atom *a = static_cast<Atom*>(*it);
		addAtom(a);
		int uid = a->getUid();
		if(uid > 0)
			posA_.push_back(uid);
		else
			negA_.push_back(-uid);
	}
}

void SmodelsConverter::handleAggregate(ObjectVector &lits, IntVector &weights)
{
	IntVector::iterator wIt = weights.begin();
	for(ObjectVector::iterator it = lits.begin(); it != lits.end(); it++, wIt++)
	{
		assert(dynamic_cast<Atom*>(*it));
		Atom *a = static_cast<Atom*>(*it);
		addAtom(a);
		int uid = a->getUid();
		if(uid > 0)
		{
			posA_.push_back(uid);
			wPosA_.push_back(*wIt);
		}
		else
		{
			negA_.push_back(-uid);
			wNegA_.push_back(*wIt);
		}
	}
}

void SmodelsConverter::handleParity(bool body, Aggregate *a, int &l)
{
	assert(a->bounds_ == Aggregate::LU);
	if(body && negBoundsWarning_)
	{
		negBoundsWarning_ = false;
		std::cerr << "Warning: parity aggregate in a rule body do not use recursively!" << std::endl;
	}
	handleAggregate(a->lits_);
	int head;
	head = newUid();
	IntVector pos, neg(1);
	if(a->lower_ == 0)
		printBasicRule(head, pos, pos);
	pos.resize(1);
	for(IntVector::iterator it = posA_.begin(); it != posA_.end(); it++)
	{
		pos[0] = *it;
		neg[0] = head;
		head = newUid();
		printBasicRule(head, pos, neg);
		printBasicRule(head, neg, pos);
	}
	neg.resize(0);
	pos.resize(2);
	for(IntVector::iterator it = negA_.begin(); it != negA_.end(); it++)
	{
		pos[0] = *it;
		pos[1] = head;
		head = newUid();
		printBasicRule(head, pos, neg);
		printBasicRule(head, neg, pos);
	}
	l = head;
}

void SmodelsConverter::handleCount(Aggregate *a, int &l, int &u)
{
	if(!(a->bounds_ & Aggregate::LU))
	{
		l = u = 0;
		return;
	}
	handleAggregate(a->lits_);
	if((a->bounds_ & Aggregate::L))
	{
		l = newUid();
		printConstraintRule(l, a->lower_, posA_, negA_);
	}
	else
		l = 0;
	if((a->bounds_ & Aggregate::U))
	{
		u = newUid();
		printConstraintRule(u, a->upper_ + 1, posA_, negA_);
	}
	else
		u = 0;	
}

// TODO: to much copy and paste in the two functions below
void SmodelsConverter::handleSum(bool body, Aggregate *a, int &l, int &u)
{
	if(!(a->bounds_ & Aggregate::LU))
	{
		l = u = 0;
		return;
	}
	IntVector::iterator wIt = a->weights_.begin();
	for(ObjectVector::iterator it = a->lits_.begin(); it != a->lits_.end(); it++, wIt++)
	{
		assert(dynamic_cast<Atom*>(*it));
		Atom *atm = static_cast<Atom*>(*it);
		addAtom(atm);
		int uid = atm->getUid();
		// transform away negative bounds
		if(*wIt < 0)
		{
			if(body && negBoundsWarning_)
			{
				negBoundsWarning_ = false;
				std::cerr << "Warning: sum with negative bounds in the body of a rule" << std::endl;
			}
			if(uid < 0)
				posA_.push_back(-uid), wPosA_.push_back(-*wIt);
			else
				negA_.push_back(uid), wNegA_.push_back(-*wIt);
			if((a->bounds_ & Aggregate::L))
				a->lower_-= *wIt;
			if((a->bounds_ & Aggregate::U))
				a->upper_-= *wIt;
		}
		else if(uid > 0)
		{
			if(uid > 0)
				posA_.push_back(uid), wPosA_.push_back(*wIt);
			else
				negA_.push_back(-uid), wNegA_.push_back(*wIt);
		}
	}	
	if((a->bounds_ & Aggregate::L))
	{
		l = newUid();
		printWeightRule(l, a->lower_, posA_, negA_, wPosA_, wNegA_);
	}
	else
		l = 0;
	if((a->bounds_ & Aggregate::U))
	{
		u = newUid();
		printWeightRule(u, a->upper_ + 1, posA_, negA_, wPosA_, wNegA_);
	}
	else
		u = 0;
}

void SmodelsConverter::handleAvg(bool body, Aggregate *a, int &l, int &u)
{
	if(!(a->bounds_ & Aggregate::LU))
	{
		l = u = 0;
		return;
	}
	IntVector::iterator wIt = a->weights_.begin();
	for(ObjectVector::iterator it = a->lits_.begin(); it != a->lits_.end(); it++, wIt++)
	{
		assert(dynamic_cast<Atom*>(*it));
		Atom *atm = static_cast<Atom*>(*it);
		addAtom(atm);
	}
	// handle the lower bound
	if((a->bounds_ & Aggregate::L))
	{
		int bound = 0;
		IntVector::iterator wIt = a->weights_.begin();
		for(ObjectVector::iterator it = a->lits_.begin(); it != a->lits_.end(); it++, wIt++)
		{
			int uid    = static_cast<Atom*>(*it)->getUid();
			int weight = *wIt - a->lower_;
			// transform away negative bounds
			if(weight < 0)
			{
				if(body && negBoundsWarning_)
				{
					negBoundsWarning_ = false;
					std::cerr << "Warning: average aggregates may not be used recursively" << std::endl;
				}
				if(uid < 0)
					posA_.push_back(-uid), wPosA_.push_back(-weight);
				else
					negA_.push_back(uid), wNegA_.push_back(-weight);
				bound-= weight;
			}
			else if(weight > 0)
			{
				if(uid > 0)
					posA_.push_back(uid), wPosA_.push_back(weight);
				else
					negA_.push_back(-uid), wNegA_.push_back(weight);
			}
		}	
		l = newUid();
		printWeightRule(l, bound, posA_, negA_, wPosA_, wNegA_);
	}
	else
		l = 0;
	// handle the upper bound
	if((a->bounds_ & Aggregate::U))
	{
		negA_.clear();
		posA_.clear();
		wNegA_.clear();
		wPosA_.clear();
		int bound = 0;
		IntVector::iterator wIt = a->weights_.begin();
		for(ObjectVector::iterator it = a->lits_.begin(); it != a->lits_.end(); it++, wIt++)
		{
			int uid    = static_cast<Atom*>(*it)->getUid();
			int weight = *wIt - a->upper_;
			// transform away negative bounds
			if(weight < 0)
			{
				if(body && negBoundsWarning_)
				{
					negBoundsWarning_ = false;
					std::cerr << "Warning: average aggregates may not be used recursively" << std::endl;
				}
				if(uid < 0)
					posA_.push_back(-uid), wPosA_.push_back(-weight);
				else
					negA_.push_back(uid), wNegA_.push_back(-weight);
				bound-= weight;
			}
			else if(weight > 0)
			{
				if(uid > 0)
					posA_.push_back(uid), wPosA_.push_back(weight);
				else
					negA_.push_back(-uid), wNegA_.push_back(weight);
			}
		}
		u = newUid();
		printWeightRule(u, bound + 1, posA_, negA_, wPosA_, wNegA_);
	}
	else
		u = 0;
}

void SmodelsConverter::handleMax(Aggregate *a, int &l, int &u)
{
	IntVector empty, lit(1);
	if(!(a->bounds_ & Aggregate::LU))
	{
		l = u = 0;
		return;
	}
	handleAggregate(a->lits_, a->weights_);
	l = (a->bounds_ & Aggregate::L) ? newUid() : 0;
	u = (a->bounds_ & Aggregate::U) ? newUid() : 0;
	for(IntVector::iterator it = negA_.begin(), itW = wNegA_.begin(); it != negA_.end(); it++, itW++)
	{
		lit[0] = *it;
		if((a->bounds_ & Aggregate::U) && (*itW > a->upper_))
			printBasicRule(u, empty, lit);
		if((a->bounds_ & Aggregate::L) && (*itW >= a->lower_))
			printBasicRule(l, empty, lit);
	}
	for(IntVector::iterator it = posA_.begin(), itW = wPosA_.begin(); it != posA_.end(); it++, itW++)
	{
		lit[0] = *it;
		// there may be no literal bigger than the upper bound
		if((a->bounds_ & Aggregate::U) && (*itW > a->upper_))
			printBasicRule(u, lit, empty);
		// if there is a literal bigger than the lower bound the aggregate may be satisfied
		if((a->bounds_ & Aggregate::L) && (*itW >= a->lower_))
			printBasicRule(l, lit, empty);
	}
}

void SmodelsConverter::handleMin(Aggregate *a, int &l, int &u)
{
	// l is the monotone part and u the inverted antimonotone part
	// it has nothing to do with lower and upper bound anymore :)
	IntVector empty, lit(1);
	if(!(a->bounds_ & Aggregate::LU))
	{
		l = u = 0;
		return;
	}
	handleAggregate(a->lits_, a->weights_);
	l = (a->bounds_ & Aggregate::U) ? newUid() : 0;
	u = (a->bounds_ & Aggregate::L) ? newUid() : 0;
	for(IntVector::iterator it = negA_.begin(), itW = wNegA_.begin(); it != negA_.end(); it++, itW++)
	{
		lit[0] = *it;
		if((a->bounds_ & Aggregate::U) && (*itW <= a->upper_))
			printBasicRule(l, empty, lit);
		if((a->bounds_ & Aggregate::L) && (*itW < a->lower_))
			printBasicRule(u, empty, lit);
	}
	for(IntVector::iterator it = posA_.begin(), itW = wPosA_.begin(); it != posA_.end(); it++, itW++)
	{
		lit[0] = *it;
		// there may be no literal bigger than the upper bound the aggregate may be satisfied
		if((a->bounds_ & Aggregate::U) && (*itW <= a->upper_))
			printBasicRule(l, lit, empty);
		// there may be no literal smaller than the lower bound
		if((a->bounds_ & Aggregate::L) && (*itW < a->lower_))
			printBasicRule(u, lit, empty);
	}
}

void SmodelsConverter::printBody(Aggregate *a)
{
	negA_.clear();
	posA_.clear();
	wNegA_.clear();
	wPosA_.clear();
	int l, u;
	switch(a->type_)
	{
		case Aggregate::COUNT:
			handleCount(a, l, u);
			break;
		case Aggregate::SUM:
			handleSum(true, a, l, u);
			break;
		case Aggregate::MAX:
			handleMax(a, l, u);
			break;
		case Aggregate::MIN:
			handleMin(a, l, u);
			break;
		case Aggregate::AVG:
			handleAvg(true, a, l, u);
			break;
		case Aggregate::PARITY:
			u = 0;
			handleParity(true, a, l);
			break;
		default:
			assert(false);
			break;
	}
	if(a->neg_)
	{
		int h = newUid();
		IntVector pos;
		IntVector neg;
		if(l > 0)
			pos.push_back(l);
		if(u > 0)
			neg.push_back(u);
		printBasicRule(h, pos, neg);
		neg_.push_back(h);
	}
	else
	{
		if(l > 0)
			pos_.push_back(l);
		if(u > 0)
			neg_.push_back(u);
	}
}

void SmodelsConverter::print(Fact *r)
{
	handleHead(r->head_);
}

void SmodelsConverter::print(Rule *r)
{
	assert(dynamic_cast<Conjunction*>(r->body_));
	handleBody(static_cast<Conjunction*>(r->body_)->lits_);
	handleHead(r->head_);
}

void SmodelsConverter::print(Integrity *r)
{
	handleBody(static_cast<Conjunction*>(r->body_)->lits_);
	printBasicRule(false_, pos_, neg_);
}

void SmodelsConverter::print(Optimize *r)
{
	int inv = r->type_ == Optimize::MINIMIZE ? 1 : -1;
	IntVector::iterator wIt = r->weights_.begin();
	for(ObjectVector::iterator it = r->lits_.begin(); it != r->lits_.end(); it++, wIt++)
	{
		assert(dynamic_cast<Atom*>(*it));
		Atom *a = static_cast<Atom*>(*it);
		addAtom(a);
		int uid = inv * a->getUid();
		if(uid > 0)
		{
			pos_.push_back(uid);
			wPos_.push_back(*wIt);
		}
		else
		{
			neg_.push_back(-uid);
			wNeg_.push_back(*wIt);
		}
	}
	printMinimizeRule(pos_, neg_, wPos_, wNeg_);
}

void SmodelsConverter::print(Compute *r)
{
	for(ObjectVector::iterator it = r->lits_.begin(); it != r->lits_.end(); it++)
	{
		assert(dynamic_cast<Atom*>(*it));
		Atom *a = static_cast<Atom*>(*it);
		addAtom(a);
		int uid = a->getUid();
		if(uid > 0)
			pos_.push_back(uid);
		else
			neg_.push_back(-uid);
	}
	printComputeRule(r->models_, pos_, neg_);
}

void SmodelsConverter::print(Object *r)
{
	pos_.clear();
	neg_.clear();
	head_.clear();
	wPos_.clear();
	wNeg_.clear();
	if(dynamic_cast<Fact*>(r))
	{
		print(static_cast<Fact*>(r));
	}
	else if(dynamic_cast<Integrity*>(r))
	{
		print(static_cast<Integrity*>(r));
	}
	else if(dynamic_cast<Rule*>(r))
	{
		print(static_cast<Rule*>(r));
	}
	else if(dynamic_cast<Optimize*>(r))
	{
		print(static_cast<Optimize*>(r));
	}
	else if(dynamic_cast<Compute*>(r))
	{
		print(static_cast<Compute*>(r));
	}
	else
	{
		assert(false);
	}
}

SmodelsConverter::~SmodelsConverter()
{
}

