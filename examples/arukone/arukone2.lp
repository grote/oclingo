#hide.
#show path/5.

field(1..fx,1..fy).

node(N) :- l(N,X,Y,X_,Y_).
end(N, X, Y)   :- l(N,X_,Y_,X,Y).

neighbor(X, Y, X + 1, Y) :- field(X, Y), field(X + 1, Y).
neighbor(X, Y, X - 1, Y) :- field(X, Y), field(X - 1, Y).
neighbor(X, Y, X, Y + 1) :- field(X, Y), field(X, Y + 1).
neighbor(X, Y, X, Y - 1) :- field(X, Y), field(X, Y - 1).

1 { path(N, X, Y, XN, YN) : neighbor(X, Y, XN, YN) } 1 :- l(N, X, Y, X_, Y_).
1 { path(N, X, Y, XN, YN) : neighbor(X, Y, XN, YN) } 1 :- path(N, X_, Y_, X, Y), node(N), neighbor(X_, Y_, X, Y), not end(N, X, Y).

reached(N, X, Y) :- l(N, X, Y, X_, Y_).
reached(N, X, Y) :- path(N, X_, Y_, X, Y).
reached_end(N, X, Y) :- path(N, X_, Y_, X, Y), end(N, X, Y).

 :- 2 { reached(N, X, Y) : node(N) }, field(X, Y).
 :- not reached_end(N, X, Y), end(N, X, Y).

