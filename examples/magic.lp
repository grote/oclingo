#hide.
#show el/2.

num(0..n).

% generate solution candidates
1 { el(X,Y) : num(Y) : X * Y <= n } 1 :- num(X).

% how often does X occur in the sequence
aggr(Y, X) :- Y = { el(Z,X) : num(Z) }, num(X).

% discards wrong solutions 
equal(X) :- el(X, Y), aggr(Y,X).
:- not equal(X), num(X).

