#hide.
#show q/2.

#domain time(K).
#const timesteps=100.

time(1..timesteps).

%#base.

%#cumulative(K).
d(K).

{q(X,K)} :- not l(K-1,X), d(X), X<K.
{q(K,Y): d(Y) : Y <= K}1.

l(K,X) :- l(K-1,X), d(X), X<K.
l(K,X) :- q(X,K), d(X), X<K.
l(K,K) :- 1{q(K,Y): d(Y) : Y <= K}.

c(K,Y) :- c(K-1,Y), d(Y), Y<K.
c(K,Y) :- q(K,Y), d(Y), Y<K.
c(K,K) :- 1{q(X,K): d(X) : X <= K}.

:- q(X,K), q(Z,Y), abs(X-Z)==K-Y, d(X),      d(Y), Y<K, d(Z).
:- q(K,Y), q(X,Z), K-X==abs(Y-Z), d(X), X<K, d(Y), Y<K, d(Z), Z<K.

%#volatile(K).

:- not l(timesteps,X), d(X).
:- not c(timesteps,Y), d(Y).

