#!/usr/bin/env python3

# small script that brings the includes in line with the bingo project

import glob
import os.path

includes = glob.glob("luasql/*.h")
sources  = glob.glob("src/*.c")
lua      = [ os.path.relpath(x, "../liblua") for x in glob.glob("../liblua/lua/*.h") ]

for x in includes + sources:
	content = open(x).read()
	for y in includes + lua:
		z = os.path.basename(y)
		content = content.replace('#include "{0}"'.format(z), '#include <{0}>'.format(y))
	open(x, "w").write(content)

