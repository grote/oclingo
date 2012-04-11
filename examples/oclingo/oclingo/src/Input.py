"""
Module oClingo Input
TODO
"""

import os, sys, re
		
# TODO consider using a real parser
PARSER = {
	'step'          : re.compile("#step (\d+)( *: *\d+ *)?\."),
	'endstep'       : re.compile("#endstep\."),
	'cumulative'    : re.compile("#cumulative\."),
	'volatile'      : re.compile("#volatile( : *\d+)?\."),
	'forget'        : re.compile("#forget (\d+)(\.\.\d+)?\."),
	'assert'        : re.compile("#assert : *.+\."),
	'retract'       : re.compile("#retract : *.+\."),
	'stop'          : re.compile("#stop\."),
	'fact'			: re.compile("^-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\.(\s*%.*)?$"),
	'integrity'		: re.compile("^\s*:-(\s*(not\s+)?-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\s*,?\s*)+\.(\s*%.*)?$"),
	'rule'			: re.compile("^\s*-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\s*:-(\s*(not\s+)?-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\s*,?\s*)+\.(\s*%.*)?$")
}

def getFromFile(file):
	"""
	Reads input stream from a file and returns list of predicate lists
	"""
	if os.path.exists(file):
		online_input = [[]]

		f = open(file, 'r')
		i = 0
		
		for line in f:
			# match various statements
			if PARSER['step'].match(line) != None or\
			   PARSER['fact'].match(line) != None or\
			   PARSER['integrity'].match(line) != None or\
			   PARSER['rule'].match(line) != None or\
			   PARSER['cumulative'].match(line) != None or\
			   PARSER['volatile'].match(line) != None or\
			   PARSER['forget'].match(line) != None or\
			   PARSER['assert'].match(line) != None or\
			   PARSER['retract'].match(line) != None:
				online_input[i].append(line)
			# match comment or empty line
			elif re.match("^%.*\n$", line) != None or re.match("^\n$", line) != None:
				continue
			# match end of step and end list
			elif PARSER['endstep'].match(line) != None:
				i += 1
				online_input.insert(i, [])
			# match end of online knowledge
			elif PARSER['stop'].match(line) != None:
				if len(online_input[i]) == 0:
					online_input.pop(i)
				break
			# error: neither fact nor step
			else:
				i += 1
				raise RuntimeError("Invalid rule '%s' after step %d in file '%s'." % (line, i, file))
		return online_input
	else:
		raise RuntimeError("Could not find file '%s'." % file)


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

