"""
Module oClingo Input

It can be used to retrieve online input via files or STDIN.
"""

import os, sys, re
import Parser


def getFromFile(file):
	"""
	Reads input stream from a file and returns a list online progression modules as strings.
	There will be no separate '#stop.' list element, so you are responsible to send '#stop.' yourself
	once the list is empty.
	"""
	if os.path.exists(file):
		f = open(file, 'r')
	
		input = f.read()
		p = Parser.Parser(input)
		p.parse_input()
		f.close()
		return p.get_online_input()


def getFromSTDIN():
	"""
	Reads one input step from STDIN, verifies and returns it
	"""
	print "Please enter new information, '#endstep.' on its own line to end:"
	
	while True:
		input = __readFromSTDIN()
		if re.match('#stop\.', input) == None:
			# no stop received, parse input
			p = Parser.Parser(input)
			if p.validate_input():
				break
			print "Please try again!"
		else:
			# received #stop. so stop
			break

	return input


def __readFromSTDIN():
	input = ''

	while True:
		# read lines until #endstep or #stop are received
		line = sys.stdin.readline()
		input += line
		if re.match('#endstep\.', line) != None or re.match('#stop\.', line) != None:
			break

	return input
