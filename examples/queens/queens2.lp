1 { q(X,Y) : d(Y) } 1 :- d(X).
1 { q(X,Y) : d(X) } 1 :- d(Y).

%:- d(X), d(Y), d(X1), d(Y1), q(X,Y), q(X1,Y1), X!=X1,  Y!=Y1, abs(X-X1) == abs(Y-Y1).
:- d(X), d(X1), X!=X1, d(Y), d(Y1), Y!=Y1, abs(X-X1) == abs(Y-Y1), q(X,Y), q(X1,Y1).

d(1..10).
