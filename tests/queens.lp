#hide.
#show q(X,Y).

#cumulative k.
dim(k).

{ q(X,Y) } :- dim(X;Y).
:- q(X1,Y1), q(X2,Y2), X1 < X2, #abs(X1-X2)==#abs(Y2-Y1).
:- q(X,Y1;Y2), Y1 < Y2.
:- q(X1;X2,Y), X1 < X2.

#volatile k.
:- not 1 { q(X,Y) }, dim(Y).
:- not 1 { q(X,Y) }, dim(X).

