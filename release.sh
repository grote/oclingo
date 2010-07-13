#!/bin/bash
version=3.0.0
files=$(ls -d {lib{gringo,clasp,lua,luasql,program_opts},lemon,cmake,app,CMakeLists.txt,Makefile,README,INSTALL,CHANGES,COPYING})
gringo_gen=$(ls build/release/libgringo/src/{converter.cpp,converter_impl.cpp,converter_impl.h,parser.cpp,parser_impl.cpp,parser_impl.h})
make static target=all
rm -rf build/dist
mkdir -p build/dist
for x in gringo clingo iclingo; do
	rsync --delete -a ${files} build/dist/${x}-${version}-source/
	sed "s/target=gringo/target=${x}/" < Makefile > build/dist/${x}-${version}-source/Makefile
	cp ${gringo_gen} build/dist/${x}-${version}-source/libgringo/src/
	(cd build/dist; tar -czf ${x}-${version}-source.tar.gz ${x}-${version}-source)
	mkdir -p build/dist/{${x}-${version}-win32,${x}-${version}-x86-linux}
	#cp build/mingw32/bin/$x.exe CHANGES COPYING build/dist/${x}-${version}-win32
	cp build/static/bin/$x CHANGES COPYING build/dist/${x}-${version}-x86-linux
	(cd build/dist; tar -czf ${x}-${version}-x86-linux.tar.gz ${x}-${version}-x86-linux)
	(cd build/dist; zip -r  ${x}-${version}-win32.tar.gz ${x}-${version}-win32)
	rm -rf build/dist/${x}-${version}-{win32,x86-linux,source}
done

