"""
Module oClingo Input
TODO
"""

import os, sys, re
import Parser


def getFromFile(file):
	"""
	Reads input stream from a file and returns list of predicate lists
	"""
	if os.path.exists(file):
		f = open(file, 'r')
	
		input = f.read()
		p = Parser.Parser(input)
		p.parse_input()
		return p.get_online_input()


def getFromSTDIN():
	"""
	Reads one input step from STDIN, verifies and returns it
	"""
	print "Please enter new information, '#endstep.' on its own line to end:"
	input = ''

	while True:
		line = sys.stdin.readline()

		if PARSER['step'].match(line) != None or\
		   PARSER['fact'].match(line) != None or\
		   PARSER['integrity'].match(line) != None or\
		   PARSER['rule'].match(line) != None or\
		   PARSER['cumulative'].match(line) != None or\
		   PARSER['volatile'].match(line) != None or\
		   PARSER['forget'].match(line) != None or\
		   PARSER['assert'].match(line) != None or\
		   PARSER['retract'].match(line) != None:
			input += line
		elif PARSER['endstep'].match(line) != None or PARSER['stop'].match(line) != None:
			input += line
			break
		else:
			print "  Warning: Ignoring unknown input."

	return input

