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

//#include <strstream>
#include <sstream>
#include <iomanip>
#include "timer.h"

CTimer::CTimer () {
  _sec = 0;
  _usec = 0;
}

void CTimer::Start () {
#ifdef WIN32
  _start = clock ();
#else
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  _start = ru.ru_utime;
#endif
}

void CTimer::Stop () {
#ifdef WIN32
  clock_t ticks = clock () - _start;
  long s = ticks / CLOCKS_PER_SEC;
  _sec += s;
  _usec += (ticks - s*CLOCKS_PER_SEC)*1000/CLOCKS_PER_SEC;
  if (_usec >= 1000) {
      _usec -= 1000;
      _sec++;
  }
#else
  struct rusage ru;
  getrusage(RUSAGE_SELF, &ru);
  struct timeval stop = ru.ru_utime;
  _sec+= stop.tv_sec - _start.tv_sec;
  if(stop.tv_usec >= _start.tv_usec)
  {
  	_usec+= stop.tv_usec - _start.tv_usec;
  }
  else
  {
  	_usec+= 1000000 - _start.tv_usec + stop.tv_usec;
	_sec--;
  }
  if(_usec >= 1000000)
  {
    _usec-= 1000000;
    _sec++;
  }
#endif
}

void CTimer::Reset () {
  _sec = 0;
  _usec = 0;
}

std::string CTimer::Print () const {

  std::istringstream in(std::ios::in | std::ios::out);
  std::ostream out(in.rdbuf ());

#ifdef WIN32
  out << _sec << '.' << std::setw(3) << std::setfill('0') << _usec; 
#else
  out << _sec << '.' << std::setw(3) << std::setfill('0') << _usec / 1000; 
#endif
  
  return (in.str());
}

CTimer::operator double() const
{
#ifdef WIN32
  return _sec + _usec / 1000.0;
#else
  return _sec + _usec / 1000000.0;
#endif
}

