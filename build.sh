#!/bin/bash
procs=1
[[ -e /proc/cpuinfo ]] && procs=$(($(grep processor /proc/cpuinfo | wc -l)+1))
cd build || exit
for x in *; do
	
	[[ $x == "release" ]] && continue
	[[ -d "$x" ]] || continue
	cd "$x"
	for y in *; do
		[[ -d "$y" ]] || continue
		echo "========== building build/$x/$y... =========="
		cd "$y"
		cmake .
		make -j${procs} || exit
		cd ..
	done
	cd ..
done

