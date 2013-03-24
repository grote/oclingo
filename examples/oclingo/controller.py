#!/usr/bin/env python
#########################################################################
#   Copyright 2010-2013 Torsten Grote, Philipp Obermeier
#
# 	This program is free software: you can redistribute it and/or modify
# 	it under the terms of the GNU General Public License as published by
# 	the Free Software Foundation, either version 3 of the License, or
# 	(at your option) any later version.
#
# 	This program is distributed in the hope that it will be useful,
# 	but WITHOUT ANY WARRANTY; without even the implied warranty of
# 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# 	GNU General Public License for more details.
#
# 	You should have received a copy of the GNU General Public License
# 	along with this program.  If not, see <http://www.gnu.org/licenses/>.
##########################################################################

import sys
import os
import socket
import re
import time
import argparse
from threading import Thread
from subprocess import call, check_output, CalledProcessError
from argparse import Action
import subprocess

# Parse Command Line Options
usage = "usage: %(prog)s [online.lp] [options]"
parser = argparse.ArgumentParser(usage=usage)
parser.add_argument("files", nargs="*", help="Optional input stream file")
parser.add_argument('--version', action='version', version='0.2')
parser.add_argument("-n", "--host", dest="host", help="Hostname of the oClingo server. Default: %(default)s")
parser.add_argument("-p", "--port", dest="port", type=int, help="Port the oClingo server is listening to. Default: %(default)s")
parser.add_argument("-t", "--time", dest="time", type=int, help="Time delay in seconds between sending input from online.lp to server. Default: %(default)s")
parser.add_argument("-w", "--wait", dest="wait", choices=["yes", "no"], help="Wait for answer set before sending new input, yes or no. Default: %(default)s")
parser.add_argument("-d", "--debug", dest="debug", action="store_true", help="Show debugging output")
parser.add_argument("-q", "--query", dest="query", action="store_true", help="Activate query mode.")
parser.add_argument("--qq", dest="qquads", nargs="*", help="""For query mode: quads of the form '<query atom>,<base atom>,<arity>, <precondition>'
					can be given to establish an equivalence mapping in the encoding. E.g.,  triple 'q,base_q,3,pre' instructs the mapping of 'q/3' to 'base_q/3'
				 	where precondition 'pre' is applied to guarantee safety . Default: %(default)s""")
parser.add_argument("--qii", dest="qii", action="store_true", help="""For query mode: automatically increments the incremental step counter for each new query and
					implicitly adds the counter value as last argument to the query atom, e.g. 'q(1)' becomes 'q(1, <inc step>'. Default: %(default)s""")
parser.add_argument("-o", "--oclenc", dest="oclenc", nargs="*", help="Wrapper mode for oClingo server, takes oclingo encoding(s) as input. Default: %(default)s")
parser.add_argument("--op","--oclpar", dest="oclpar", nargs="*", help="For oClingo Wrapper mode: takes oclingo command line parameters as input. Default: %(default)s")
parser.add_argument("--qws", "--qwindow-size", dest="qWindowSize", type=int, help="""For query mode: specifies the default window size of queries,
					i.e., for how many input steps a query should be considered. Default: %(default)s""")
parser.set_defaults(
	files='',
	host='localhost',
	port=25277,
	time=0,
	wait="yes",
	qquads=[],
	oclenc=[],
	oclpar=[],
	qii=False,
	qWindowSize=1,
	# query=False,
	debug=False
)
args = parser.parse_args()
equivalEnc = "% Equivalence Additions\n"  # equivalence Encodings for query relations
currentIncStep = 0
# print args.files
# print len(args.files)
# print args.qapairs
# print len(args.qapairs)
#print args.oclenc
# print args.qquads

exit = False
have_answer_set = False
data_old = ''
online_input = [[]]

FACT = re.compile("^-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\.(\s*%.*)?$")  # Arbitrary fact clause
FACTT = re.compile("^-?[a-z_][a-zA-Z0-9_]*\((.*?),?(\d+)\)\.?$")  # Fact clause whose last component is a decimal number
INTEGRITY = re.compile("^\s*:-(\s*(not\s+)?-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\s*,?\s*)+\.(\s*%.*)?$")  # Integrity constraint clause.
RULE = re.compile("^\s*-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\s*:-(\s*(not\s+)?-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\s*,?\s*)+\.(\s*%.*)?$")  # Rule clause.
RELATION = re.compile("[a-z_][a-zA-Z0-9_]*")  # Valid relation name
QQUAD = re.compile("([a-z_][a-zA-Z0-9_]*),([a-z_][a-zA-Z0-9_]*),([0-9]*),([a-z_][a-zA-Z0-9_]*)")

IN_PARSER = {
	'step'			: re.compile("#step (\d+)( *: *\d+ *)?\."),
	'incstep'		: re.compile("#istep\."),  # +1 increment command for query mode
	'incstepd'		: re.compile("#istep (\d+)\."),  # +d increment command for query mode
	'endstep'		: re.compile("#endstep\."),
	'cumulative'	: re.compile("#cumulative\."),
	'volatile'		: re.compile("#volatile( : *\d+)?\."),
	'forget'		: re.compile("#forget (\d+)(\.\.\d+)?\."),
	'assert'		: re.compile("#assert : *.+\."),
	'retract'		: re.compile("#retract : *.+\."),
	'stop'			: re.compile("#stop\."),
}

# Parser for answer received from Oclingo
PARSER = [
					re.compile("^Step:\ (\d+)$"),
					re.compile("^(-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\ *)+$"),
					re.compile("^Input:$"),
					re.compile("^End of Step.$"),
					re.compile("^Warning: (?P<series>.+)$"),
					re.compile("^Error: (?P<series>.+)$"),
					re.compile("^UNSAT at step (\d+)$")
]


def addQatomEquival():
	""" Adds euqivalence mappings specified by '--qp'"""
	global equivalEnc
	for q in args.qquads:
		if QQUAD.match(q):
			extRel = QQUAD.match(q).group(1)
			baseRel = QQUAD.match(q).group(2)
			arity = QQUAD.match(q).group(3)
			precond = QQUAD.match(q).group(4)

			for i in range(int(arity)):
				if i == 0:
					relArgs = "X0"
				else:
					relArgs += ",X" + str(i)

			equivalEnc += ("#volatile t.\n"
							"#external " + extRel + "(" + relArgs + ",t) : " + precond + "(" + relArgs + ")" + ".\n"
							":- " + extRel + "(" + relArgs + ",t) , " + "not " + baseRel + "(" + relArgs + ").\n"
							":- not " + extRel + "(" + relArgs + ",t) , " + baseRel + "(" + relArgs + ").\n")
		else:
			raise RuntimeError("Invalid query quad format.")
		if args.debug:
			print "equivalEnc:\n" + equivalEnc

def main():
	# Add equivalence assignements for query and base atoms.
	if len(args.qquads):
		addQatomEquival()

	# if wrapper mode, launch own oclingo server
	if len(args.oclenc):
		threado = OclingoThread()
		threado.start()
		time.sleep(2)  # TODO: Proper sync signal

	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	connectToSocket(s)
	prepareInput()  # prepare input stream if given as file

	# if asynchronous execution, start thread to continuously receive input
	if args.wait == "no":
		thread = InputThread(s)
		thread.start()

	# loop to fetch and display answer sets received from oclingo
	while not exit:
		try:
			try:
				# receive answer sets
				answer_sets = getAnswerSets(s)
			except RuntimeWarning as e:
				print e.args.files[0]
				continue
			except SyntaxError as e:
				print e.args.files[0]

			# send answer sets to stdout
			processAnswerSets(answer_sets)

			if args.wait == "yes":  # if synchronous execution, fetch next input
				input = getInput()
				sendInput(s, input)
			else:
				# just in case, should be done before thread terminates
				if not thread.is_alive():
					sendInput(s, "#stop.\n")
					break

		except socket.error:
			print "Socket was closed."
			break
	closeSocket(s)
	return 0

def connectToSocket(s):
	try:
		s.connect((args.host, int(args.port)))
	except socket.error:
		raise EnvironmentError("Could not connect to %s:%s" % (args.host, args.port))

# Prepares input stream for processing if given as file
def prepareInput():
	"""Prepares input stream for processing if given as file."""
	if len(args.files) == 1:
		file = args.files[0]
		if os.path.exists(file):
			global online_input

			f = open(file, 'r')
			i = 0

			for line in f:
				# match new step
				if IN_PARSER['step'].match(line) != None:
					online_input[i].append(line)
				# match ground fact, integrity constraint or rule and insert into existing list
				elif FACT.match(line) != None or INTEGRITY.match(line) != None or RULE.match(line) != None:
					online_input[i].append(line)
				# match sections
				elif IN_PARSER['cumulative'].match(line) != None or IN_PARSER['volatile'].match(line) != None:
					online_input[i].append(line)
				# match forget
				elif IN_PARSER['forget'].match(line) != None:
					online_input[i].append(line)
				# match assert/retract
				elif IN_PARSER['assert'].match(line) != None or IN_PARSER['retract'].match(line) != None:
					online_input[i].append(line)
				# match comment
				elif re.match("^%.*\n$", line) != None or re.match("^\n$", line) != None:
					continue
				# match end of step and end list
				elif IN_PARSER['endstep'].match(line) != None:
					i += 1
					online_input.insert(i, [])
				# match end of online knowledge
				elif IN_PARSER['stop'].match(line) != None:
					if len(online_input[i]) == 0:
						online_input.pop(i)
					break
				# error: neither fact nor step
				else:
					i += 1
					raise RuntimeError("Invalid rule '%s' after step %d in file '%s'." % (line, i, file))
		else:
			raise RuntimeError("Could not find file '%s'." % file)
	elif len(args.files) > 1:
		raise RuntimeError("More than one online file was given")

def getAnswerSets(s):
	global exit, currentIncStep
	answer_sets = []

	while not exit:
		try:
			output = recv_until(s, '\0')
		except socket.error:
			raise EnvironmentError("Socket was closed.")

		for line in output.splitlines():
			matched = False
			for i in range(len(PARSER)):
				match = PARSER[i].match(line)
				if match != None:
					matched = True
					if i == 0:
						currentIncStep = int(match.group(1))
						if args.debug:
							print "Found answer set for step %s." % match.group(1)
					elif i == 1:
						answer_sets.append(line.split())
					elif i == 3:
						return answer_sets
					elif i == 4:
						raise RuntimeWarning(line)
					elif i == 5:
						raise RuntimeError(line)
					elif i == 6:
						print "Program was unsatisfiable and stopped at step %s!" % match.group(1)
						return answer_sets
			if not matched:
				raise SyntaxError("Unkown output received from server: %s" % line)
	return answer_sets

def recv_until(s, delimiter):
	global data_old, exit

	if args.debug:
		print "Receiving data from socket..."

	data_list = data_old.split(delimiter, 1)
	data = ''

	if len(data_list) > 1:
		data_old = data_list[1]
		return data_list[0]
	else:
		while not exit:
			data = ''
			try:
				data = s.recv(2048)
				if data == '':
					exit = True
					raise IOError("Socket was closed by server, probably because '#stop.' was sent, the program is not satisfiable anymore or --imax was reached.")
					return data_old
			except socket.error:
				if args.debug:
					print "Socket was closed. Returning last received data..."
				exit = True
				return data + data_old
			data = data_old + data
			data_list = data.split(delimiter, 1)
			if len(data_list) > 1:
				data = data_list[0]
				data_old = data_list[1]
				break;
			data_old = data

	return data

def processAnswerSets(answer_sets):
	global have_answer_set
	i = 1
	have_answer_set = True
	for answer_set in answer_sets:
		print "Answer: %d" % i
		printAnswerSet(answer_set)
		i += 1

def printAnswerSet(answer_set):
	"""prints the answer set"""
	plan = {}
	# stores all predicates in dictionary plan
	for predicate in answer_set:
		match = FACTT.match(predicate)
		if match != None:
			t = int(match.group(2))
			if t in plan:
				plan[t].append(predicate)
			else:
				plan[t] = [predicate]
		else:
			if 'B' in plan:
				plan['B'].append(predicate)
			else:
				plan['B'] = [predicate]

	# get predicate lenghts
	row_len = {}
	for time in plan:
		plan[time].sort()
		row = 0
		for predicate in plan[time]:
			if row in row_len:
				if len(predicate) > row_len[row]:
					row_len[row] = len(predicate)
			else:
				row_len[row] = len(predicate)
			row += 1

	# prints predicates in rows
	if 'B' in plan:
		printRow(plan, row_len, 'B')
	for time in sorted(plan.iterkeys()):
		if time != 'B':
			printRow(plan, row_len, time)

def printRow(plan, row_len, time):
	time_str = str(time).rjust(len(str(len(plan))))
	print "  %s. " % time_str,
	row = 0
	for predicate in plan[time]:
		 print predicate.ljust(row_len[row] + 1),
		 row += 1
	print ""

# Aux method called by InputThread, make sure the main thread is not accessing at the same time
def getInput():
	"""Aux method called by InputThread."""
	if len(args.files) == 1:
		time.sleep(float(args.time))
		if len(online_input) > 0:
			result = ''.join(online_input[0])
			online_input.pop(0)
			result += "#endstep.\n"
		else:
			result = "#stop.\n"

		print "Got input:"
		print result
	elif args.query:
		result = getInputFromSTDINQM()
	else:
		result = getInputFromSTDIN()
	return result

# Function to read directly from STDIN with support for
# adhoc query mode
def getInputFromSTDINQM():
	"""Function to read directly from STDIN with support for adhoc query mode"""
	input = ''
	print "Please enter your query (format: single ground clause)"
	# parse input
	while True:
		line = sys.stdin.readline()
		if 	FACT.match(line) != None or\
			INTEGRITY.match(line) != None or\
			RULE.match(line) != None:
				# In case implicit incremental par is requested by user
				if	FACT.match(line) != None and args.qii:
					global currentIncStep
					currentIncStep += 1
					substitute = "," + str(currentIncStep) + ")."
					line = re.sub(r'\)\s*.\s*', substitute, line)
				input = ("#step " + str(currentIncStep) + ".\n"
						"#volatile: " + str(args.qWindowSize) + ".\n" +
		        		line + "\n"
		        		"#endstep.\n")
				break
		elif IN_PARSER['incstep'].match(line) != None:  # +1 increment instruction
			input = ("#step " + str(currentIncStep + 1) + ".\n" 
					"#endstep.\n")
			break
		elif IN_PARSER['incstepd'].match(line) != None:  # +d increment instruction
			input = ("#step " + IN_PARSER['incstepd'].match(line).group(1) + ".\n"
					"#endstep.\n")
			break
		elif IN_PARSER['endstep'].match(line) != None or IN_PARSER['stop'].match(line) != None:
			input = line
			break
		else:
			print "  Warning: Ignoring unknown input."
	return input

# Function to read directly from STDIN with support for
# classical stream format,
def getInputFromSTDIN():
	"""Function to read directly from STDIN with support for classical stream format,"""
	input = ''
	print "Please enter new information, '#endstep.' on its own line to end:"

	# parse and concatenate input line by line
	while True:
		line = sys.stdin.readline()

		if IN_PARSER['step'].match(line) != None or\
		   IN_PARSER['forget'].match(line) != None or\
		   IN_PARSER['assert'].match(line) != None or\
		   IN_PARSER['retract'].match(line) != None or\
		   IN_PARSER['cumulative'].match(line) != None or\
		   IN_PARSER['volatile'].match(line) != None or\
		   FACT.match(line) != None or\
		   INTEGRITY.match(line) != None or\
		   RULE.match(line) != None:
			input += line
		elif IN_PARSER['endstep'].match(line) != None or IN_PARSER['stop'].match(line) != None:
			input += line
			break
		else:
			print "  Warning: Ignoring unknown input."
	return input

def sendInput(s, input):
	if args.debug:
		print "Sending following input to controller:"
		print input

	s.send('%s\0' % input)

	if re.match("#stop\.", input) != None:
		endProgram(s)

def endProgram(s):
	closeSocket(s)
	if args.debug:
		print "Exiting Program..."
	sys.exit(0)

def closeSocket(s):
	if args.debug:
		print "Closing socket..."
	try:
		s.shutdown(1)
		s.close()
	except socket.error:
		print "Socket was already closed."

class OclingoThread(Thread):
	global equivalEnc
	def __init__ (self):
		Thread.__init__(self)
	def run(self):
		# start oclingo
		#call (["oclingo"] + args.oclenc + args.oclpar)
		#print "Invoc: \n" + ("xargs echo " + "<" + args.oclenc[0] + " " + equivalEnc +  " | oclingo ")
		#call ([("xargs echo " + "<" + args.oclenc[0] + " \'" + equivalEnc +  "\'")], shell=True)
		call([("echo " + " \'" + equivalEnc +  "\' | cat "  + args.oclenc[0] + " - | oclingo ")] + args.oclpar, shell=True)
		#call ([("xargs echo " + "<" + args.oclenc[0] + " \'" + equivalEnc +  "\' | oclingo ")] + args.oclpar, shell=True)
		return
# 		proc = subprocess.Popen (["oclingo"] + args.oclenc + args.oclpar,
# 								stdout=subprocess.PIPE,
# 								stderr=subprocess.STDOUT)
#
# 		stdout_value, stderr_value = proc.communicate()
# 		print '\tcombined output:', repr(stdout_value)
# 		print '\tstderr value   :', repr(stderr_value)

class InputThread(Thread):
	def __init__ (self, s):
		Thread.__init__(self)
		self.s = s
	def run(self):
		global exit, have_answer_set
		while 1:
			input = getInput()
			if input == "#stop.\n":
				if not have_answer_set:
					print "Ignoring '#stop.' and waiting for first answer set..."
					continue
				exit = True
				endProgram(self.s)
				break
			else:
				sendInput(self.s, input)
		return

if __name__ == '__main__':
	if sys.version < '2.6':
		print 'You need at least python 2.6'
		sys.exit(1)

	if args.debug:
		sys.exit(main())
	else:
		try:
			sys.exit(main())
		except Exception, err:
			sys.stderr.write('ERROR: %s\n' % str(err))
			sys.exit(1)

