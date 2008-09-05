// Copyright 1998 by Patrik Simons
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston,
// MA 02111-1307, USA.
//
// Patrik.Simons@hut.fi
#ifndef TIMER_H
#define TIMER_H

/***************************************************************************
 *                                                                         *
 * Includes                                                                *
 *                                                                         *
 ***************************************************************************/
#include <time.h>
#include <string>
#ifndef WIN32
#	include <sys/resource.h>
#endif

/***************************************************************************
 *                                                                         *
 * class CTimer                                                            *
 *                                                                         *
 ***************************************************************************/
/*! \class CTimer timer.h
 *  \brief Timer class for duration of answer set computation.
 *  \author Patrik Simons
 *  \date 1998
 */
class CTimer {

public:
  //! Constructor.
  CTimer ();
  
  //! Destructor.
  ~CTimer () {};

  //! Starts the timer.
  void Start ();

  //! Stops the timer.
  void Stop ();

  //! Resets the timer to zero.
  void Reset ();

  operator double() const;

  //! Returns the timer value.
  std::string Print () const;

  //! Seconds.
  unsigned long _sec;

  //! Milliseconds.
  unsigned long _usec;

#ifdef WIN32
  //! Actual cpu time
  clock_t _start;
#else
  //! Actual cpu time
  struct timeval _start;
#endif

};

#endif

