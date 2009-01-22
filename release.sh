#!/bin/bash

VERSION="2.0.3"

# completely rebuild everything
make clean
rm -rf build

./init.sh --mingw32 --x86-pc-linux-gnu --clingo --iclingo --debug
rm -rf build/gringo/debug
rm -rf build/clingo/debug

mkdir -p build/release
mkdir -p build/release/all-${VERSION}-win32
mkdir -p build/release/all-${VERSION}-x86-linux

./build.sh

# create the windows binary release
cp build/gringo/mingw32/bin/gringo.exe build/release/all-${VERSION}-win32/gringo-${VERSION}-win32.exe
cp build/clingo/mingw32/bin/clingo.exe build/release/all-${VERSION}-win32/clingo-${VERSION}-win32.exe
cp build/iclingo/mingw32/bin/iclingo.exe build/release/all-${VERSION}-win32/iclingo-${VERSION}-win32.exe
zip -r build/release/all-${VERSION}-win32.zip build/release/all-${VERSION}-win32/
zip -j build/release/gringo-${VERSION}-win32.exe.zip build/release/all-${VERSION}-win32/gringo-${VERSION}-win32.exe
zip -j build/release/clingo-${VERSION}-win32.exe.zip build/release/all-${VERSION}-win32/clingo-${VERSION}-win32.exe
zip -j build/release/iclingo-${VERSION}-win32.exe.zip build/release/all-${VERSION}-win32/iclingo-${VERSION}-win32.exe
rm -rf build/release/all-${VERSION}-win32/

# create the linux binary release
cp build/gringo/x86-pc-linux-gnu/bin/gringo build/release/all-${VERSION}-x86-linux/gringo-${VERSION}-x86-linux
cp build/clingo/x86-pc-linux-gnu/bin/clingo build/release/all-${VERSION}-x86-linux/clingo-${VERSION}-x86-linux
cp build/iclingo/x86-pc-linux-gnu/bin/iclingo build/release/all-${VERSION}-x86-linux/iclingo-${VERSION}-x86-linux
tar -czf build/release/all-${VERSION}-x86-linux.tar.gz build/release/all-${VERSION}-x86-linux/
gzip -c > build/release/gringo-${VERSION}-x86-linux.gz build/release/all-${VERSION}-x86-linux/gringo-${VERSION}-x86-linux
gzip -c > build/release/clingo-${VERSION}-x86-linux.gz build/release/all-${VERSION}-x86-linux/clingo-${VERSION}-x86-linux
gzip -c > build/release/iclingo-${VERSION}-x86-linux.gz build/release/all-${VERSION}-x86-linux/iclingo-${VERSION}-x86-linux
rm -rf build/release/all-${VERSION}-x86-linux/

# create the source release
cd build/gringo/release/
make package_source
cd ../../..

# this must be possible with cmake too
cp build/gringo/release/GrinGo-${VERSION}-Source.tar.gz build/release/
tar -xf build/release/GrinGo-${VERSION}-Source.tar.gz -C build/release
rm build/release/GrinGo-${VERSION}-Source.tar.gz
mv build/release/GrinGo-${VERSION}-Source build/release/gringo-${VERSION}-source
# copy the generated files into the source release
mkdir build/release/gringo-${VERSION}-source/lib/gringo/generated
cp build/gringo/release/lib/gringo/lparselexer.cpp build/release/gringo-${VERSION}-source/lib/gringo/generated
cp build/gringo/release/lib/gringo/plainlparselexer.cpp build/release/gringo-${VERSION}-source/lib/gringo/generated
cp build/gringo/release/lib/gringo/lparseconverter_impl.cpp build/release/gringo-${VERSION}-source/lib/gringo/generated
cp build/gringo/release/lib/gringo/lparseconverter_impl.h build/release/gringo-${VERSION}-source/lib/gringo/generated
cp build/gringo/release/lib/gringo/lparseparser_impl.cpp build/release/gringo-${VERSION}-source/lib/gringo/generated
cp build/gringo/release/lib/gringo/lparseparser_impl.h build/release/gringo-${VERSION}-source/lib/gringo/generated
cd build/release
tar -czf gringo-${VERSION}-source.tar.gz gringo-${VERSION}-source
rm -rf gringo-${VERSION}-source

#svn copy https://potassco.svn.sourceforge.net/svnroot/potassco/trunk/gringo https://potassco.svn.sourceforge.net/svnroot/potassco/tags/gringo-${VERSION}
