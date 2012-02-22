#!/usr/bin/env python
#########################################################################
#   Copyright 2010-2012 Torsten Grote
#
#	This program is free software: you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation, either version 3 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.
##########################################################################

import sys
import oclingo
from optparse import OptionParser

# Parse Command Line Options
usage = "usage: %prog [online.lp] [options]"
parser = OptionParser(usage=usage, version="%prog 0.2")
parser.add_option("-n", "--host", dest="host", help="Hostname of the oClingo server. Default: %default")
parser.add_option("-p", "--port", dest="port", help="Port the oClingo server is listening to. Default: %default")
parser.add_option("-t", "--time", dest="time", help="Time delay in seconds between sending input from [online.lp] to server. Default: %default")
parser.add_option("-w", "--wait", dest="wait", choices=["yes","no"], help="Wait for answer set before sending new input, yes or no. Default: %default")
parser.add_option("-d", "--debug", dest="debug", help="show debugging output", action="store_true")
parser.set_defaults(
	host = 'localhost',
	port = 25277,
	time = 0,
	wait = "yes",
	debug = False
)
(opt, args) = parser.parse_args()


def main():
	c = oclingo.Controller(opt.port, opt.host)
	c.connect()
	c.send("#step 1.\ne(1).\n#endstep.\n#stop.\n")
	result = c.recv()
	print oclingo.formatList(result)
	c.disconnect()
	return 0


if __name__ == '__main__':
	if sys.version < '2.6':
		print 'You need at least python 2.6'
		sys.exit(1)
	
	if opt.debug:
		sys.exit(main())
	else:
		try:
			sys.exit(main())
		except Exception, err:
			sys.stderr.write('ERROR: %s\n' % str(err))
			sys.exit(1)

