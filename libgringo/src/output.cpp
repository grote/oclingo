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

#include <gringo/output.h>
#include <gringo/storage.h>
#include <gringo/domain.h>

Output::Output()
	: s_(0)
	, hideAll_(false)
{
}

void Output::hideAll()
{
	if(!hideAll_) { doHideAll(); }
	hideAll_ = true;
}

void Output::show(uint32_t nameId, uint32_t arity, bool show)
{
	Domain *dom = s_->newDomain(nameId, arity);
	if (show)
	{
		if (!dom->show)
		{
			dom->show = true;
			doShow(nameId, arity, show);
		}
	}
	else
	{
		if (!dom->hide)
		{
			dom->hide = true;
			doShow(nameId, arity, show);
		}
	}
}

bool Output::shown(uint32_t domId)
{
	Domain *dom = s_->domain(domId);
	if(hideAll_) { return dom->show; }
	else         { return !dom->hide; }
}
