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

#ifndef CLASP_TIMER_H_INCLUDED
#define CLASP_TIMER_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif
#include <clasp/util/platform.h> // uint64

class Timer {
public:
	Timer() : running_(false), start_(0), split_(0), total_(0) {}
	void   start()   { running_ = true;  start_ = clockStamp(); }
	void   stop()    { split(clockStamp()); running_ = false; }
	void   reset()   { *this  = Timer(); }
	//! same as stop(), start();
	void   lap()     { uint64 t; split(t = clockStamp()); start_ = t; }
	//! elapsed time (in seconds) for last start-stop cycle
	double elapsed() const { 
		return split_ / double(ticksPerSec());
	}
	//! total elapsed time for all start-stop cycles 
	double total()   const {
		return total_ / double(ticksPerSec());
	}
	static uint64 clockStamp();
	static uint64 ticksPerSec();
private:
	void split(uint64 t) { if(running_) { total_ += (split_ = t-start_); } }
	bool   running_;
	uint64 start_;
	uint64 split_;
	uint64 total_;
};

#endif
