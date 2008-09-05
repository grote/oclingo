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

#ifndef CLASP_PLATFORM_H_INCLUDED
#define CLASP_PLATFORM_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1200
#include <basetsd.h>
typedef UINT8			uint8;
typedef INT32			int32;
typedef UINT32		uint32;
typedef UINT64		uint64;
typedef UINT_PTR	uintp;
#elif defined(__GNUC__) && __GNUC__ >= 3
#include <inttypes.h>
typedef uint8_t		uint8;
typedef int32_t		int32;
typedef uint32_t	uint32;
typedef uint64_t	uint64;
typedef uintptr_t uintp;
#else 
#error unknown compiler or platform. Please add typedefs manually.
#endif

#endif
