#!/bin/bash
cd build
for x in *; do
	cd "$x"
	for y in *; do
		echo "========== building build/$x/$y... =========="
		cd "$y"
		make
		cd ..
	done
	cd ..
done

