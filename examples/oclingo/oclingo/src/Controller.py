"""
a controller class that connects as a client to an oClingo server
"""

import sys # TODO remove when not needed anymore
import os
import socket
import json
import re

class Controller:
	"""
	A controller class that talks to oClingo
	takes port, host and debug value as constructor arguments
	"""

	def __init__(self, port=25277, host="localhost", debug=False):
		self.port = port
		self.host = host
		self.debug = debug
		self.connected = False
		self.delim = '\0'
		self.current_answer = '';


	def connect(self):
		"""
		connect to socket.
		needs to be called before other functions can be used.
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
		disconnect from socket
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
		send input to connected oclingo server
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
		receive answer from connected oclingo server and wait for result.
		returns list of answer sets
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
		receive answer from connected oclingo server and do not wait for result.
		returns list of answer sets
		"""
		self.s.setblocking(0)
		result = self.recv()
		self.s.setblocking(1)

		return result

