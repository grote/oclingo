number(0..3).
even(0).
even(X+1) :- odd(X).
odd(X+1) :- number(X), even(X).
dull(X) :- number(X), not interesting(X).
interesting(X) :- number(X), not dull(X).
two_divides(X) :- even(X).
interesting_odd(X) :- odd(X), interesting(X).
