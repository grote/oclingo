"""
Module oClingo
TODO
"""

import sys # TODO remove when not needed anymore
import os
import socket
import json
import re

class Controller:
	"""
	A controller class that talks to oClingo
	"""

	def __init__(self, port=25277, host="localhost", debug=False):
		"""
        TODO
		"""
		self.port = port
		self.host = host
		self.debug = debug
		self.connected = False
		self.delim = '\0'
		self.current_answer = '';


	def connect(self):
		"""
		TODO
		"""
		if(not self.connected):
			try:
				self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
				self.s.connect((self.host, int(self.port)))
				self.connected = True
			except socket.error:
				raise EnvironmentError("Could not connect to %s:%s" % (self.host, self.port))


	def disconnect(self):
		"""
		TODO
		"""
		if self.debug:
			print "Closing socket..."
		try:
			self.s.shutdown(1)
			self.s.close()
			self.connected = False
		except socket.error:
			print "Socket is already closed."


	def send(self, input):
		"""
		TODO
		"""
		if self.connected:
			self.s.sendall(input + self.delim)
		
#			if re.match("#stop\.", input) != None:
#				self.disconnect()
#				if self.debug:
#					print "Exiting Program..."
		else:
			raise EnvironmentError("Controller is not yet connected to server.")

		
	def _recv_until(self):
		all_data = []
		data = ''
		while True:
			data = self.s.recv(8192)
			if data == '':
				raise IOError("Socket was closed by server, probably because '#stop.' was sent, the program is not satisfiable anymore or --imax was reached.")
			elif self.delim in data:
				all_data.append(data[:data.find(self.delim)])
				break
			all_data.append(data)
		self.current_answer = ''.join(all_data)

	
	def recv(self):
		"""
		TODO
		"""
		answer_sets = []

		if self.connected:
			self._recv_until()
			res = json.loads(self.current_answer)
			if "Type" in res:
				if res["Type"] == "Error" and "Text" in res:
					raise RuntimeError(res["Text"])
				elif res["Type"] == "Warning" and "Text" in res:
					raise RuntimeWarning(res["Text"])
				elif res["Type"] == "Answer" and "Result" in res:
					if res["Result"] == "UNSATISFIABLE":
						# TODO raise own Warning and handle print out user client side
						sys.stdout.write("Program was unsatisfiable and stopped")
						if "Step" in res:
							sys.stdout.write(" at step " + str(res["Step"]))
							print "!"
						return answer_sets
					elif res["Result"] == "SATISFIABLE" and "Witnesses" in res:
						for witness in res["Witnesses"]:
							answer_sets.append(witness["Value"])
						return answer_sets
			raise SyntaxError("Unkown output received from server: %s" % self.current_answer)
		else:
			raise EnvironmentError("Controller is not yet connected to server.")


	def irecv(self):
		"""
		TODO
		"""
		self.s.setblocking(0)
		result = self.recv()
		self.s.setblocking(1)

		return result


def formatList(as_list):
	"""
	TODO
	"""
	i = 1
	result = ''
	for answer_set in as_list:
		result += "Answer: %d\n" % i
		result += _formatAnswerSet(answer_set) + "\n"
		i += 1

	return result

def _formatAnswerSet(answer_set):
	plan = {}
	pred_exp = re.compile("^-?[a-z_][a-zA-Z0-9_]*\((.*?),?(\d+)\)$")

	# stores all predicates in dictionary plan
	for predicate in answer_set:
		match = pred_exp.match(predicate)
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
	
	result = ''

	# format predicates in rows
	if 'B' in plan:
		result += _formatRow(plan, row_len, 'B')
	for time in sorted(plan.iterkeys()):
		if time != 'B':
			result += _formatRow(plan, row_len, time)

	return result

def _formatRow(plan, row_len, time):
	result = ''
	time_str = str(time).rjust(len(str(len(plan))))
	result += "  %s. " % time_str
	row = 0
	for predicate in plan[time]:
		 result += predicate.ljust(row_len[row]+1)
		 row += 1
	
	return result + "\n"


def getInputFromFile(file):
	"""
	Reads input stream from a file and returns list of predicate lists
	"""
	if os.path.exists(file):
		online_input = [[]]
		
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

		f = open(file, 'r')
		i = 0
		
		for line in f:
			# match various statements
			if PARSER['step'].match(line) != None \
			or PARSER['fact'].match(line) != None \
			or PARSER['integrity'].match(line) != None \
			or PARSER['rule'].match(line) != None \
			or PARSER['cumulative'].match(line) != None \
			or PARSER['volatile'].match(line) != None \
			or PARSER['forget'].match(line) != None \
			or PARSER['assert'].match(line) != None \
			or PARSER['retract'].match(line) != None:
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
