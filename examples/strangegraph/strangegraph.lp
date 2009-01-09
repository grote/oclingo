#hide.
#show path(X,Y).

reached(X) :- start(X).

% a node has at most out outgoing edges
 { path(X,Y) : edge(X,Y)  } Out :- reached(X), node(X, In, Out).

% a node has at least in incoming edges
 reached(Y) :- In { path(X,Y) : edge(X,Y) }, node(Y, In, Out).

 :- not reached(X), node(X, In, Out).

