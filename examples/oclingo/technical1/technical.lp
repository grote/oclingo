#base.

a :- not b.
b :- not a.

#cumulative t.

#external e/1.

c(t) :- e(t).
step(t).

#volatile t.

