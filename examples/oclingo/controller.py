#!/usr/bin/python
#########################################################################
#   Copyright 2010 Torsten Grote
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
import os
import socket
import re
import time
from optparse import OptionParser
from threading import Thread

# Parse Command Line Options
usage = "usage: %prog [online.lp] [options]"
parser = OptionParser(usage=usage, version="%prog 0.1")
parser.add_option("-n", "--host", dest="host", help="Hostname of the online iClingo server. Default: %default")
parser.add_option("-p", "--port", dest="port", help="Port the online iClingo server is listening to. Default: %default")
parser.add_option("-t", "--time", dest="time", help="Time delay in seconds between sending input from online.lp to server. Default: %default")
parser.add_option("-w", "--wait", dest="wait", choices=["yes","no"], help="Wait for answer set before sending new input, yes or no. Default: %default")
parser.add_option("-d", "--debug", dest="debug", help="show debugging output", action="store_true")
parser.set_defaults(
	host = 'localhost',
	port = 25277,
	time = 1,
	wait = "yes",
	debug = False
)
(opt, args) = parser.parse_args()

exit = False
have_answer_set = False
data_old = ''
online_input = [[]]
FACT  = re.compile("^-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\.?$")
FACTT = re.compile("^-?[a-z_][a-zA-Z0-9_]*\((.*?),?(\d+)\)\.?$")
INTEGRITY = re.compile("^\s*:-(\s*(not\s+)?-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\s*,?\s*)+\.$")
RULE = re.compile("^\s*-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\s*:-(\s*(not\s+)?-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\s*,?\s*)+\.$")
IN_PARSER = {
	'step'			: re.compile("#step (\d+)\."),
	'endstep'		: re.compile("#endstep\."),
	'cumulative'	: re.compile("#cumulative\."),
	'volatile'		: re.compile("#volatile\."),
	'forget'		: re.compile("#forget (\d+)\."),
	'stop'			: re.compile("#stop\."),
}
PARSER = [
	re.compile("^Step:\ (\d+)$"),
	re.compile("^(-?[a-z_][a-zA-Z0-9_]*(\(.+\))?\ *)+$"),
	re.compile("^Input:$"),
	re.compile("^End of Step.$"),
	re.compile("^Warning: (?P<series>.+)$"),
	re.compile("^Error: (?P<series>.+)$"),
]

def main():
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	
	connectToSocket(s)
	prepareInput()
	
	if opt.wait == "no":
		thread = InputThread(s)
		thread.start()
	
	while not exit:
		try:
			try:
				answer_sets = getAnswerSets(s)
			except RuntimeWarning as e:
				print e.args[0]
				continue
			except SyntaxError as e:
				print e.args[0]
			
			processAnswerSets(answer_sets)
			
			if opt.wait == "yes":
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
		s.connect((opt.host, opt.port))
	except socket.error:
		raise EnvironmentError("Could not connect to %s:%d" % (opt.host, opt.port))

def prepareInput():
	if len(args) == 1:
		file = args[0]
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
					raise RuntimeError("Invalid ground fact '%s' after step %d in file '%s'." % (line, i, file))
		else:
			raise RuntimeError("Could not find file '%s'." % file)
	elif len(args) > 1:
		raise RuntimeError("More than one online file was given")

def getAnswerSets(s):
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
					if i == 0 and opt.debug:
						print "Found answer set for step %s." % match.group(1)
					elif i == 1:
						answer_sets.append(line.split())
					elif i == 3:
						return answer_sets
					elif i == 4:
						raise RuntimeWarning(line)
					elif i == 5:
						raise RuntimeError(line)
			if not matched:
				raise SyntaxError("Unkown output received from server: %s" % line)
	return answer_sets

def recv_until(s, delimiter):
	global data_old, exit

	if opt.debug:
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
				if opt.debug:
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
		 print predicate.ljust(row_len[row]+1),
		 row += 1
	print ""

# is called by InputThread, make sure the main thread is not accessing at the same time
def getInput():
	if len(args) == 1:
		time.sleep(float(opt.time))
		if len(online_input) > 0:
			result = ''.join(online_input[0])
			online_input.pop(0)
			result += "#endstep.\n"
		else:
			result = "#stop.\n"
		
		print "Got input:"
		print result
	else:
		result = getInputFromSTDIN()
	
	return result

def getInputFromSTDIN():
	print "Please enter new information, '#endstep.' on its own line to end:"
	input = ''

	while True:
		line = sys.stdin.readline()

		if IN_PARSER['step'].match(line) != None or\
		   IN_PARSER['forget'].match(line) != None or\
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
	s.send('%s\0' % input)
	
	if re.match("#stop\.", input) != None:
		endProgram(s)

def endProgram(s):
	closeSocket(s)
	if opt.debug:
		print "Exiting Program..."
	sys.exit(0)

def closeSocket(s):
	if opt.debug:
		print "Closing socket..."
	try:
		s.shutdown(1)
		s.close()
	except socket.error:
		print "Socket was already closed."


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
	
	if opt.debug:
		sys.exit(main())
	else:
		try:
			sys.exit(main())
		except Exception, err:
			sys.stderr.write('ERROR: %s\n' % str(err))
			sys.exit(1)




