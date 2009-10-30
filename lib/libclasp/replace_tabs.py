#!/usr/bin/python

import sys

def transform(l):
	result  = ""
	replace = False
	pos     = 0
	for x in l:
		if x == "\t":
			inc = 2 - pos % 2
		else:
			inc = 1
		pos+= inc

		if not replace:
			result+=x
			if x != "\t" and x != " ":
				replace = True
		else:
			if x == "\t":
				if inc == 1:
					result+= " "
				else:
					result+= "  "
			else:
				result+= x
	return result

for arg in sys.argv[1:]:
	try:
		# read and convert the file
		fin = open(arg, "r")
		result = ""
		for l in fin:
			result+= transform(l)
		fin.close()
		# write the converted file
		fout = open(arg, "w")
		fout.write(result)
		fout.close()
	except IOError:
		print "Couldn't open ", arg

