#!/usr/bin/python

import sys

class Program:
	def __init__(self, name):
		self.max    = 0
		self.pos    = []
		self.neg    = []
		self.models = 0
		self.symtab = {}

		self.fin  = file(name, "r")
		self.fout = file(name + ".lpeq", "w")

		for l in self.fin:
			if l[0] == '0':
				break;
			# here it would be better to distinguish the rule types
			for x in l.strip().split(' '):
				if x != '':
					self.max = max(int(x), self.max)
			self.fout.write(l)
		for l in self.fin:
			if l[0] == '0':
				break;
			s = l.strip().split(' ')
			self.symtab[s[1]] = int(s[0])
			self.max = max(int(s[0]), self.max)
		skip = True
		for l in self.fin:
			if skip:
				skip = False
				continue
			if l[0] == '0':
				break;
			self.pos.append(int(l))
			self.max = max(int(l), self.max)
		skip = True
		for l in self.fin:
			if skip:
				skip = False
				continue
			if l[0] == '0':
				break;
			self.neg.append(int(l))
			self.max = max(int(l), self.max)
		for l in self.fin:
			self.models = int(l)
		self.fin.close()
	
	def update(a, b):
		false = -1
		for key, val in b.symtab.items():
			if not a.symtab.has_key(key):
				if false < 0:
					a.max += 1
					false = a.max
					a.neg.append(false)
				a.max += 1
				a.symtab[key] = a.max
				a.fout.write("1 " + str(false) + " 1 0 " + str(a.max) + "\n")

	def __del__(self):
		self.fout.write("0\n")
		for key, val in self.symtab.items():
			self.fout.write(str(val) + " " + key + "\n")
		self.fout.write("0\n")
		self.fout.write("B+\n")
		for x in self.pos:
			self.fout.write(str(x) + "\n")
		self.fout.write("0\n")
		self.fout.write("B-\n")
		for x in self.neg:
			self.fout.write(str(x) + "\n")
		self.fout.write("0\n")
		self.fout.write(str(self.models) + "\n")
		self.fout.close()

a = Program(sys.argv[1])
b = Program(sys.argv[2])
a.update(b)
b.update(a)

