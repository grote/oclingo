#!/bin/bash

lparse --true-negation $* > compare.lparse
gringo2 -l $* > compare.gringo

awk '
func seek0(fin, fout) {
	max = 1
	do {
		getline l < fin
		if (l != 0) {
			print l > fout
			split(l, a, " ")
			for(i in a) {
				if (a[i] + 0 > max) {
					max = a[i] + 0
				}
			}
		}
	} while( l != "0" )
	return max
}

func symtab(fin, fout, a, ka) {
	j = 1
	do {
		getline l < fin
		if( l != "0" ) {
			split(l, s, " ")
			a[s[2]] = s[1]
			ka[j++] = s[2]
		}
	} while( l != "0" )
	asort(ka)
	return j - 1
}

func rest(fin, fout) {
	while ((getline l < fin) > 0) {
		print l > fout
	}
}

func intersect(a, ka, la, ma, fa, b, kb, lb, mb, fb)
{
	ia = 1;
	ib = 1;
	i = 1
	while (ia <= la && ib <= lb) {
		na = ka[ia]
		nb = kb[ib]
		if (na == nb) {
			ra[i] = a[na] " " na
			rb[i] = b[nb] " " nb
			ia++;
			ib++;
		} else if (na > nb) {
			ra[i] = (++ma) " " nb
			rb[i] = b[nb] " " nb
			print "1 1 1 0 " ma > fa
			ib++;
		} else {
			ra[i] = a[na] " " na
			rb[i] = (++mb) " " na
			print "1 1 1 0 " mb > fb
			ia++;
		}
		i++
	}
	print "0" > fa
	print "0" > fb
	for(j = 1; j < i; j++) {
		print ra[j] > fa
		print rb[j] > fb
	}
	print "0" > fa
	print "0" > fb
}

BEGIN {
	ma = seek0("compare.lparse", "compare.lparse.reduced")
	mb = seek0("compare.gringo", "compare.gringo.reduced")
	la = symtab("compare.lparse", "compare.lparse.reduced", a, ka)
	lb = symtab("compare.gringo", "compare.gringo.reduced", b, kb)
	intersect(a, ka, la, ma, "compare.lparse.reduced", b, kb, lb, mb, "compare.gringo.reduced")
	rest("compare.lparse", "compare.lparse.reduced")
	rest("compare.gringo", "compare.gringo.reduced")
}
'

lpeq compare.gringo.reduced compare.lparse.reduced > compare1.lp
lpeq compare.lparse.reduced compare.gringo.reduced > compare2.lp

echo "first comparison"
if ! clasp < compare1.lp | grep "Models      : 0"; then
	rm -f compare.lparse compare.lparse.reduced 
	rm -f compare.gringo compare.gringo.reduced 
	rm -f compare1.lp compare2.lp
	echo False
	exit 1
fi
echo "second comparison"
if ! clasp < compare2.lp | grep "Models      : 0"; then
	rm -f compare.lparse compare.lparse.reduced 
	rm -f compare.gringo compare.gringo.reduced 
	rm -f compare1.lp compare2.lp
	echo False
	exit 1
fi

rm -f compare.lparse compare.lparse.reduced 
rm -f compare.gringo compare.gringo.reduced 
rm -f compare1.lp compare2.lp

echo True
exit 0

