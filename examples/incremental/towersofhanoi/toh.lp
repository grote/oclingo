#hide.
#show put(M,N,K).

#cumulative k.

1{move(N,k)  : disk(N) : N > 3}1.
1{where(N,k) : disk(N)}1.

on(M,N,k) :- disk(M;N), move(M,k), where(N,k).
on(M,N,k) :- disk(M;N), on(M,N,k-1), not move(M,k).

:- disk(N), move(N,k-1), move(N,k).
:- disk(M;N), on(M,N,k-1), move(N,k).
:- disk(M;N), on(M,N,k-1), where(N,k).
:- disk(M;N), on(M,N,k), M <= N.

put(M,N,k) :- disk(M;N), move(M,k), where(N,k).

#volatile k.

:- goal_on(X,Y), not on(X,Y,k).

#base.

on(X,Y,0) :- init_on(X,Y).
