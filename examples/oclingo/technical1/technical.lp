#base.

a :- not b.
b :- not a.

#cumulative t.

#external e(t).

c(t) :- e(t).
step(t).

#volatile t.

