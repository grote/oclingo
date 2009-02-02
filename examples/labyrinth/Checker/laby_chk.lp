% #base.

dir(e;w;n;s).
inverse(e,w). inverse(w,e).
inverse(n,s). inverse(s,n).

num_rows(N) :- N = max[field(X,Y) : field(X,Y) = X].
num_cols(N) :- N = max[field(X,Y) : field(X,Y) = Y].

goal(X,Y,0)      :- goal_on(X,Y).
reach(X,Y,0)     :- init_on(X,Y).
connect(X,Y,D,0) :- connect(X,Y,D).

pushed(0).

%%  All neighboring fields in horizontal direction D

dneighbor(e,X,Y,X,Y+1) :- field(X,Y;(Y+1)).
xneighbor(e,X,Y,X,  1) :- field(X,Y), num_cols(Y).
dneighbor(w,X,Y,X,Y-1) :- field(X,Y;(Y-1)).
xneighbor(w,X,1,X,  Y) :- field(X,Y), num_cols(Y).

%%  All neighboring fields in vertical direction D

dneighbor(n,X,Y,X+1,Y) :- field(X;(X+1),Y).
xneighbor(n,X,Y,1,  Y) :- field(X,Y), num_rows(X).
dneighbor(s,X,Y,X-1,Y) :- field(X;(X-1),Y).
xneighbor(s,1,Y,X,  Y) :- field(X,Y), num_rows(X).

%%  All neighboring fields

neighbor(D,X,Y,XX,YY) :- dneighbor(D,X,Y,XX,YY).
neighbor(D,X,Y,XX,YY) :- xneighbor(D,X,Y,XX,YY).


% #cumulative t.

%%  Check row or column pushed

pushed(T) :- pushed(T-1), 1 { push(X,D,T) : push(X,D,T) } 1, max_steps(S), T = 1..S.

:- push(X,D,T), X < 1.
:- push(X,D,T), not dir(D).
:- push(X,D,T), T < 1.
:- push(X,D,T), T > S, max_steps(S).
:- push(X,D,T), not pushed(T).
:- push(X,e,T), num_rows(M), X > M, max_steps(S), T = 1..S.
:- push(X,w,T), num_rows(M), X > M, max_steps(S), T = 1..S.
:- push(Y,n,T), num_cols(N), Y > N, max_steps(S), T = 1..S.
:- push(Y,s,T), num_cols(N), Y > N, max_steps(S), T = 1..S. 

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
#show push/3.
