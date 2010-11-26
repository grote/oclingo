#!/bin/bash

# warning this script is intended to be run on MY machine!!!

#svn update
rev=$(svn status -v 2>/dev/null | grep -o "^ *[0-9]* *[0-9]*" | head -n1 | grep -o "[0-9]*$")
cpus=$[$(cat /proc/cpuinfo| grep processor | wc -l)+1]
sqlite="sqlite3"
prefix32="/home/wv/bin/linux/32"
prefix64="/home/wv/bin/linux/64"
prefixmingw="${HOME}/local/mingw32/opt"
public_html="${HOME}/public_html/gringo"

make release target=gringo-app -j${cpus} || exit 1

BOOST_ROOT=$prefix32/boost \
SQLITE3_ROOT=$prefix32/sqlite3 \
	make cross32 target=all -j${cpus} || exit 1

BOOST_ROOT=$prefix64/boost \
SQLITE3_ROOT=$prefix64/sqlite3 \
	make cross64 target=all -j${cpus} || exit 1

BOOST_ROOT=$prefixmingw/boost \
SQLITE3_ROOT=$prefixmingw/sqlite3 \
	make mingw32 target=all -j${cpus} || exit 1

# 32 bit stuff
for x in gringo clingo iclingo; do
	name=$x-r$rev
	cp build/cross32/bin/$x $prefix32/$name
	chmod g+w $prefix64/$name
	strip $prefix32/$name
	(cd $prefix32; rm -f $x-banane; ln -s $name $x-banane)
done

# 64 bit stuff
for x in gringo clingo iclingo; do
	name=$x-r$rev
	cp build/cross64/bin/$x $prefix64/$name
	chmod g+w $prefix64/$name
	strip $prefix64/$name
	(cd $prefix64; rm -f $x-banane; ln -s $name $x-banane)
done

# linux download
rm -rf gringo-x86-linux gringo-x86-linux.tar.bz2
mkdir gringo-x86-linux
cp build/cross32/bin/{gringo,clingo,iclingo} gringo-x86-linux
strip gringo-x86-linux/{gringo,clingo,iclingo}
tar -cjf gringo-x86-linux.tar.bz2 gringo-x86-linux
cp gringo-x86-linux.tar.bz2 $public_html
rm -rf gringo-x86-linux gringo-x86-linux.tar.bz2

# windows download
rm -rf gringo-win32 gringo-win32.zip
mkdir gringo-win32
cp build/mingw32/bin/{gringo,clingo,iclingo}.exe gringo-win32
i586-mingw32msvc-strip gringo-win32/*.exe
zip -r gringo-win32.zip gringo-win32
cp gringo-win32.zip $public_html
rm -rf gringo-win32 gringo-win32.zip

