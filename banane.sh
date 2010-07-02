#!/bin/bash

svn update
REV=$(svn status -v | grep -o "^ *[0-9]* *[0-9]*" | head -n1 | grep -o "[0-9]*$")
make debug release static target=all || exit 1

prefix=/home/wv/bin/linux/32
public_html=${HOME}/public_html/bingo/bingo-x86-linux
rm -rf $public_html
mkdir -p $public_html


for target in bingo clabingo iclabingo
do
	for variant in debug static
	do
		case $variant in
			debug)
				rm -f /home/wv/bin/linux/32/$target-dbg*
				cp build/$variant/bin/$target $prefix/$target-dbg-r$REV
				chmod g+w $prefix/$target-dbg-r$REV
				(cd $prefix; rm -f $target-dbg-banane; ln -s $target-dbg-r$REV $target-dbg-banane)
				;;
			static)
				name=$public_html/$target
				cp build/$variant/bin/$target $name
				strip $name
				(
					cd $public_html
					cd ..
					rm -f bingo-x86-linux.tar.bz2
					tar -cjf bingo-x86-linux.tar.bz2 bingo-x86-linux
				)
				name=$target-r$REV
				cp build/$variant/bin/$target $prefix/$name
				chmod g+w $prefix/$name
				strip $prefix/$name
				(cd $prefix; rm -f $target-banane; ln -s $name $target-banane)
				;;
		esac
	done
done

rm -rf $public_html
