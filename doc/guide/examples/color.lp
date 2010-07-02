% Generate
1 { color(X,C) : color(C) } 1 :- node(X).
% Test
:- edge(X,Y), color(X,C), color(Y,C).
