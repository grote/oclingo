% #base.

dir(e;w;n;s).
inverse(e,w). inverse(w,e).
inverse(n,s). inverse(s,n).

num_rows(N) :- N = max[field(X,Y) : field(X,Y) = X].
num_cols(N) :- N = max[field(X,Y) : field(X,Y) = Y].

goal(X,Y,0)      :- goal_on(X,Y).
reach(X,Y,0)     :- init_on(X,Y).
connect(X,Y,D,0) :- connect(X,Y,D).

%%  Direct neighbors

dneighbor(n,X,Y,X+1,Y) :- field(X;(X+1),Y).
dneighbor(s,X,Y,X-1,Y) :- field(X;(X-1),Y).
dneighbor(e,X,Y,X,Y+1) :- field(X,Y;(Y+1)).
dneighbor(w,X,Y,X,Y-1) :- field(X,Y;(Y-1)).

%%  All neighboring fields

neighbor(D,X,Y,XX,YY) :- dneighbor(D,X,Y,XX,YY).
neighbor(n,X,Y, 1, Y) :- field(X,Y), num_rows(X).
neighbor(s,1,Y, X, Y) :- field(X,Y), num_rows(X).
neighbor(e,X,Y, X, 1) :- field(X,Y), num_cols(Y).
neighbor(w,X,1, X, Y) :- field(X,Y), num_cols(Y).


% #cumulative t.

%%  Select a row or column to push

1 { push(X,e;w,T) : X=1..M, push(Y,n;s,T) : Y=1..N } 1 :- num_rows(M), num_cols(N), max_steps(S), T = 1..S, goal(X,Y,T-1), not reach(X,Y,T-1).

%%  Determine new position of a (pushed) field

shift(XX,YY,X,Y,T) :- neighbor(e,XX,YY,X,Y), push(XX,e,T), max_steps(S), T = 1..S.
shift(XX,YY,X,Y,T) :- neighbor(w,XX,YY,X,Y), push(XX,w,T), max_steps(S), T = 1..S.
shift(XX,YY,X,Y,T) :- neighbor(n,XX,YY,X,Y), push(YY,n,T), max_steps(S), T = 1..S.
shift(XX,YY,X,Y,T) :- neighbor(s,XX,YY,X,Y), push(YY,s,T), max_steps(S), T = 1..S.
shift( X, Y,X,Y,T) :- field(X,Y), not push(X,e;w,T), not push(Y,n;s,T), max_steps(S), T = 1..S.

%%  Move connections around

connect(X,Y,D,T) :- connect(XX,YY,D,T-1), dir(D), shift(XX,YY,X,Y,T), max_steps(S), T = 1..S.

%%  Location of goal after pushing

goal(X,Y,T) :- goal(XX,YY,T-1), shift(XX,YY,X,Y,T), max_steps(S), T = 1..S.


%%  Locations reachable from new position

reach(X,Y,T) :- reach(XX,YY,T-1), shift(XX,YY,X,Y,T), max_steps(S), T = 1..S.
reach(X,Y,T) :- reach(XX,YY,T), dneighbor(D,XX,YY,X,Y), connect(XX,YY,D,T), connect(X,Y,E,T), inverse(D,E), max_steps(S), T = 1..S.


% #volatile t.

%%  Goal must be reachable in t steps

:- goal(X,Y,S), not reach(X,Y,S), max_steps(S).


% #base.

#hide.
#show push(Z,D,T).
