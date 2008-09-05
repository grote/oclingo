#hide.
#show path/3.
#show reached/3.

field(1..fx,1..fy).

node(N) :- l(N,X,Y,X_,Y_).
path(N, X, Y) :- l(N,X,Y,X_,Y_).
end(N, X, Y)   :- l(N,X_,Y_,X,Y).
 
 { path(N, X + 1, Y) : X < fx, path(N, X - 1, Y) : X > 1, path(N, X, Y + 1) : Y < fy, path(N, X, Y - 1) : Y > 1 } :- path(N, X, Y), field(X, Y), node(N).

 :- 2 { path(N, X, Y) : node(N) }, field(X, Y).
 :- not path(N,X,Y), end(N,X,Y).


