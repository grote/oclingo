p(1, 1..30000).
q(X) :- p(X, Y), not r(X).
r(X) :- q(X).
