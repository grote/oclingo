#base.

dir(e;w;n;s).
inverse(e,w). inverse(w,e).
inverse(n,s). inverse(s,n).

num_rows(N) :- N = max[field(X,Y) : field(X,Y) = X].
num_cols(N) :- N = max[field(X,Y) : field(X,Y) = Y].

goal(X,Y,0)      :- goal_on(X,Y).
move(X,Y,0)      :- init_on(X,Y).
connect(X,Y,D,0) :- connect(X,Y,D).

pushed(0).

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


#cumulative t.

%%  Select a row or column to push

{ pushed(t) } :- pushed(t-1).
1 { push(X,e;w,t) : X=1..M, push(Y,n;s,t) : Y=1..N } 1 :- pushed(t), num_rows(M), num_cols(N).

%%  Determine new position of a (pushed) field

shift(XX,YY,X,Y,t) :- neighbor(e,XX,YY,X,Y), push(XX,e,t).
shift(XX,YY,X,Y,t) :- neighbor(w,XX,YY,X,Y), push(XX,w,t).
shift(XX,YY,X,Y,t) :- neighbor(n,XX,YY,X,Y), push(YY,n,t).
shift(XX,YY,X,Y,t) :- neighbor(s,XX,YY,X,Y), push(YY,s,t).
shift( X, Y,X,Y,t) :- field(X,Y), not push(X,e;w,t), not push(Y,n;s,t).

%%  Move connections around

connect(X,Y,D,t) :- connect(XX,YY,D,t-1), dir(D), shift(XX,YY,X,Y,t).

%%  Location of goal after pushing

goal(X,Y,t) :- goal(XX,YY,t-1), shift(XX,YY,X,Y,t).


%%  Locations reachable from new position

reach(X,Y,t) :- move(XX,YY,t-1), shift(XX,YY,X,Y,t).
reach(X,Y,t) :- reach(XX,YY,t), dneighbor(D,XX,YY,X,Y), connect(XX,YY,D,t), connect(X,Y,E,t), inverse(D,E).

%%  Move to some reachable location

1 { move(X,Y,t) : field(X,Y) } 1.
:- move(X,Y,t), not reach(X,Y,t).


#volatile t.

%%  Goal must be reached in t steps

:- goal(X,Y,t), not move(X,Y,t).


#base.

#hide.
#show push/3.
#show move/3.
