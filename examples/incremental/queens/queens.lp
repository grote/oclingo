#hide.
#show q(X,Y).

#base.

#cumulative k.

dim(k).

{q(X,k) : dim(X)}.
{q(k,Y) : dim(Y) : Y<k}.

:- q(X1,k), q(X2,Y), Y<k, abs(X1-X2)==k-Y, dim(X1), dim(X2), dim(Y).
:- q(k,Y1), q(X,Y2), Y1<k, Y2<k, X<k, k-X==abs(Y1-Y2), dim(X), dim(Y1), dim(Y2).

#volatile k.

:- {q(X,Y) : dim(Y)}0, dim(X).
:- 2{q(X,Y) : dim(Y)}, dim(X).
:- {q(X,Y) : dim(X)}0, dim(Y).
:- 2{q(X,Y) : dim(X)}, dim(Y).

#base.
