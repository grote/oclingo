% strange feedback from volatile to cumulative
#cumulative k.
r(k) :- v(k-1).
r(k-1).
#volatile k.
v(k).
