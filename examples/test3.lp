x(1..10).
a(X) :- x(X), not b(X).
b(X) :- a(X).

