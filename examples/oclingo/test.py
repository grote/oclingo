#!/usr/bin/env python
#########################################################################
#   Copyright 2011 Torsten Grote
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

TESTS = [
	['blocksworld', 'blocksworld.lp', 'instance.lp', 'online.lp', 'online.out'],
	['elevator', 'elevator.lp', 'building.lp',  'online.lp',  'online.out'],
	['elevator', 'elevator.lp', 'building2.lp', 'online2.lp', 'online2.out'],
	['little_robot', 'robot.lp', 'world.lp', 'online.lp',  'online.out'],
	['little_robot', 'robot.lp', 'world.lp', 'online2.lp', 'online2.out'],
	['little_robot', 'robot.lp', 'world.lp', 'online3.lp', 'online3.out'],
	['sensor', 'sensor.lp', 'room.lp',  'online.lp',  'online.out'],
	['sensor', 'sensor.lp', 'room3.lp', 'online3.lp', 'online3.out'],
#	['technical1', 'technical.lp',  'online.lp',  'online.out'],
#	['technical1', 'technical.lp', 'online2.lp', 'online2.out'],
	['technical1', 'technical.lp', 'online3.lp', 'online3.out'],
	['technical1', 'technical.lp', 'online4.lp', 'online4.out'],
	['technical1', 'technical.lp', 'online5.lp', 'online5.out'],
	['technical1', 'technical.lp', 'online6.lp', 'online6.out'],
	['technical2', 'volatile1.lp', 'vonline1.lp', 'vonline1.out'],
	['tictactoe', 'tictactoe.lp', 'online.lp',   'online.out'],
	['tictactoe', 'tictactoe.lp', 'online2.lp', 'online2.out'],
	['tictactoe', 'tictactoe.lp', 'online3.lp', 'online3.out'],
	['wumpus', 'wumpus.lp', 'world.lp',  'online.lp',  'online.out'],
#	['wumpus', 'wumpus.lp', 'world2.lp', 'online2.lp', 'online2.out'],
		]

OCLINGO = ''
CONTROLLER = ''

import sys
import os
import subprocess
import tempfile

def main():
	tests = []
	for test in TESTS:
		tests.append(TestCase(test))

	findPrograms()
	print

	for test in tests:
		runOClingo(test)
		output = runController(test)
		
		result = compareOutput(test, output)
	
		if result:
			print "Test " + test.name + "/" + test.online + " completed sucessfully."
		else:
			print "Error: Test " + test.online + " failed!"
			return 1

	print
	print "*****************************************"
	print "* ALL TEST RUNS SUCCESSFULLY COMPLETED! *"
	print "*****************************************"

	return 0

class TestCase:
	def __init__(self, case):
		self.name = case[0]
		self.encoding = os.path.join(self.name, case[1])
		if len(case) > 4:
			self.instance = os.path.join(self.name, case[2])
			self.online = os.path.join(self.name, case[3])
			self.output = os.path.join(self.name, case[4])
		else:
			self.instance = ''
			self.online = os.path.join(self.name, case[2])
			self.output = os.path.join(self.name, case[3])
		self.check()

	def check(self):
		if not os.path.isfile(self.encoding):
			raise IOError('File not found: ' + self.encoding)
		if self.instance != '' and not os.path.isfile(self.instance):
			raise IOError('File not found: ' + self.instance)
		if not os.path.isfile(self.online):
			raise IOError('File not found: ' + self.online)
		if not os.path.isfile(self.output):
			raise IOError('File not found: ' + self.output)


def findPrograms():
	global OCLINGO
	if not os.path.exists(OCLINGO):
		OCLINGO = '../../qtcreator-build/bin/oclingo'
		if not os.path.exists(OCLINGO):
			OCLINGO = '../../build/release/bin/oclingo'
			if not os.path.exists(OCLINGO):
				OCLINGO = '../../build/debug/bin/oclingo'
	try:
		subprocess.call([OCLINGO, '--version'], stdout=open('/dev/null', 'w'), stderr=open('/dev/null', 'w'))
	except OSError, err:
		print "Error: Could not find", OCLINGO, err
		sys.exit(1)
	print "Found oClingo at " + OCLINGO

	global CONTROLLER
	if not os.path.exists(CONTROLLER):
		CONTROLLER = './controller.py'
		if not os.path.exists(CONTROLLER):
			CONTROLLER = 'controller.py'
	try:
		subprocess.call([CONTROLLER, '--version'], stdout=open('/dev/null', 'w'), stderr=open('/dev/null', 'w'))
	except OSError, err:
		print "Error: Could not find", CONTROLLER, err
		sys.exit(1)
	print "Found controller at " + CONTROLLER


def runOClingo(test):
	global OCLINGO
	try:
		subprocess.Popen([OCLINGO, test.instance, test.encoding, '--imax=99', '0'], stdout=subprocess.PIPE, stderr=subprocess.PIPE).pid
	except OSError, err:
		print err
		sys.exit(1)


def runController(test):
	global CONTROLLER
	try:
		# dirty hack to wait until socket is open
		for n in range(1, 100):
			tmp = subprocess.call("netstat -tln | grep ':25277 .* LISTEN'", stdout=open('/dev/null', 'w'), stderr=open('/dev/null', 'w'), shell=True)
			if tmp == 0:
				break;
		
		(result, error) = subprocess.Popen([CONTROLLER, '-t 0', test.online], stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate()

		return result
	except OSError, err:
		print err
		sys.exit(1)


def compareOutput(test, output):
	try:
		(result, error) = subprocess.Popen(['diff', '-bB', test.output, '-'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE).communicate(output)
		if len(result) > 0:
			print result
			return False
	except OSError, err:
		print err
		sys.exit(1)

	return True


if __name__ == '__main__':
	if sys.version < '2.6':
		print 'You need at least python 2.6'
		sys.exit(1)

	try:
		sys.exit(main())
	except Exception, err:
		sys.stderr.write('ERROR: %s\n' % str(err))
		sys.exit(1)

