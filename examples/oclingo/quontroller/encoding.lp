% n colors
#const n = 3.
color(1..n).

% extract nodes from edges
node(X) :- edge(X,_).
node(X) :- edge(_,X).

% generate n-coloring
1 { mark(X,C) : color(C) } 1 :- node(X).
:- edge(X,Y), mark(X;Y,C).

% display n-coloring
#hide.
#show mark/2.
