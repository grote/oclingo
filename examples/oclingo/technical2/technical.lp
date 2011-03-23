#base.


#cumulative t.

#external e/1.

a(t) :- not na(t).
na(t) :- not a(t).
b(t) :- not nb(t).
nb(t) :- not b(t).

na(t) :- e(t).
step(t).

goal(t) :- a(t), b(t).

#volatile t.

:- not goal(t).
