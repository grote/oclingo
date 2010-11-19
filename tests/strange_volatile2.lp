% should not succeed in second step
#cumulative k.
p(k).
1 { b(k) : p(k) } 1.
#volatile k.
v(k).
 :- 1 { b(X) : v(X) } 1.

