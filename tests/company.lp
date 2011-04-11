%*
pcontrollable(Y) :- company(Y),  51 [ owns(X,Y,N) : owns(X,Y,N) = N ].
pcontroller(X)   :- owns(X,Y,M), 51 [ owns(X,Y,N) : owns(X,Y,N) = N ].

reach(X,Y) :- pcontrollable(X;Y),                 owns(X,Y,N), N > 0.
reach(X,Y) :- pcontroller(X), pcontrollable(Y),   owns(X,Y,N), N > 0.
reach(X,Z) :- pcontroller(X), pcontrollable(Y;Z), owns(X,Y,N), N > 0, reach(Y,Z).

controls(X,Y) :- 51 [ owns(X,Y,S) = S, owns(Z,Y,S) = S : controls(X,Z) : X != Y ], pcontroller(X), reach(X,Y).
*%

% make that one as good as the above!!!
controls1(X,Y) :- 51 #sum+ [ owns(X,Y,S) = S, owns(Z,Y,S) = S : controls1(X,Z) : X != Y ], company(X), company(Y), X != Y.
controls2(X,Y) :- 51 #sum [ owns(X,Y,S) = S, owns(Z,Y,S) = S : controls2(X,Z) : X != Y ], company(X), company(Y), X != Y.
controls3(X,Y) :- not [ owns(X,Y,S) = S, owns(Z,Y,S) = S : controls3(X,Z) : X != Y ] 50, company(X), company(Y), X != Y.

company(c1).
company(c2).
company(c3).
company(c4).
owns(c1,c2,60).
owns(c1,c3,20).
owns(c2,c3,40).
owns(c3,c4,51).
