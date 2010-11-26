d(1..400).
p(X,Y) :- d(X), d(Y).
q(X,Y,Z) :- p(X,Y),p(Z),Z=X+Y.
r(G) :- q(A,B,C), q(B,C,D), q(C,D,E), q(D,E,F), q(E,F,G).
