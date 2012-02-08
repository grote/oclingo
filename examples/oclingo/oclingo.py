"""
Module oClingo
TODO
"""

import socket
import json

class Controller:
	"""A controller class that talks to oClingo"""

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
		except socket.error:
			print "Socket is already closed."


	def send(self, input):
		"""
		TODO
		"""
		if self.connected:
			self.s.send(input + self.delim)
		
#			if re.match("#stop\.", input) != None:
#				disconnect()
#				if opt.debug:
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

	
	def getJSON(self):
		"""
		TODO
		"""
		self._recv_until()
		return self.current_answer


	def getList(self):
		"""
		TODO
		"""
		answer_sets = []
		self._recv_until() # TODO
		res = json.loads(self.current_answer)
 		if "Type" in res:
			if res["Type"] == "Error" and "Text" in res:
				raise RuntimeError(res["Text"])
			elif res["Type"] == "Warning" and "Text" in res:
				raise RuntimeWarning(res["Text"])
			elif res["Type"] == "Answer" and "Result" in res:
				if res["Result"] == "UNSATISFIABLE":
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


