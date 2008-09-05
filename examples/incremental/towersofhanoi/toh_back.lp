#hide.
#show put(M,N,K).

#cumulative k.

1{on(M,N,0) : disk(N) : M > N}1 :- disk(M), M > 3, k < 2.
 {on(M,N,0) : disk(M) : M > N : M > 3}1 :- disk(N), k < 2.
:- goal_on(M,N), not on(M,N,0), k < 2.

1{move(N,-k) : disk(N) : N > 3}1.
1{from(N,-k) : disk(N)}1.

on(M,N,-k) :- disk(M;N), move(M,-k), from(N,-k), M > 3.
on(M,N,-k) :- disk(M;N), on(M,N,-k+1), not move(M,-k), M > N, M > 3.

:- disk(N), move(N,-k+1), move(N,-k), N > 3.
:- disk(M;N), on(M,N,-k+1), from(N,-k), M > N, M > 3.
:- disk(M;N), on(M,N,-k+1), move(N,-k), M > N, N > 3.
:- disk(M;N), on(M,N,-k), M <= N.

put(M,N,-k) :- disk(M;N), move(M,-k), on(M,N,-k+1).

#volatile k.

:- init_on(M,N), not on(M,N,-k).

#base.
