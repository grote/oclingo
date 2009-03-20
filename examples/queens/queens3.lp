#hide.
#show q(X,Y). 

#const queens = 1000.
#const diags  = queens * 2 - 1.

d(1..queens).
diag(1..diags).

diagX(X,Y,X - Y + queens) :- d(X), d(Y).
diagY(X,Y,X + Y - 1) :- d(X), d(Y).

% exactly 1 queen in each row/col
1 { q(X,Y) : d(Y) } 1 :- d(X).
1 { q(X,Y) : d(X) } 1 :- d(Y).

% at most 1 queen on each diagonal
 { q(X,Y) : diagX(X,Y,D) } 1 :- diag(D).
 { q(X,Y) : diagY(X,Y,D) } 1 :- diag(D).

