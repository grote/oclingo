#!/bin/bash
set -eu
cd "$(dirname "$(readlink -f "$0")")"
version=$(grep GRINGO_VERSION libgringo/gringo/gringo.h | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+')
svn=https://potassco.svn.sourceforge.net/svnroot/potassco/
svn cp $svn/trunk/gringo $svn/tags/gringo-$version -m "gringo tag: $version"
cd ../../tags
svn up
cd gringo-$version
svn propset svn:externals libclasp -F <(echo -e)
svn propset svn:externals libprogram_opts -F <(echo -e)
#TODO an svn cp might be better
find libclasp/src libclasp/clasp libprogram_opts/src libprogram_opts/program_opts -name ".svn"  | xargs rm -r
svn add libclasp/src
svn add libclasp/clasp
svn add libprogram_opts/src
svn add libprogram_opts/program_opts
svn commit -m "gringo tag: $version"
