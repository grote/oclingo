#!/bin/bash
cd build || exit
for x in *; do
	
	[[ $x == "release" ]] && continue
	[[ -d "$x" ]] || continue
	cd "$x"
	for y in *; do
		[[ -d "$y" ]] || continue
		echo "========== building build/$x/$y... =========="
		cd "$y"
		make || exit
		cd ..
	done
	cd ..
done

