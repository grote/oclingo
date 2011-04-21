% instance

%*
         ^
         |
 -->a<---f<---h
    |    ^    ^
    v    |    |
    b----e    |
    |    ^    |
    v    |    v
    c--->d--->g

solution: a->b->c->d->e->f
*%

edge(a,b).
edge(b,c).
edge(c,d).
edge(d,e;g).
edge(e,f).
edge(f,a).
edge(g,h).
edge(h,f;g).

start(a).
goal(f).

% encoding
reach(G) :- goal(G).
reach(I) :- reach(J), edge(I,J).

open(I,0)   :- reach(I), not goal(I).
open(J,S+1) :- open(J,S), open(I,S) : edge(J,I) : reach(I).

dist(G,0)   :- goal(G).
dist(J,S+1) :- dist(I,S), edge(J,I), open(J,S).

first(MS) :- MS := #min { <S> : dist(I,S) : start(I) }.

path(I,1)    :- start(I), not first(#supremum).
path(MJ,S+1) :- first(D), path(I,S), S <= D, MJ := #min { <J> : dist(J,D-S) : edge(I,J) }.

#hide.
#show path/2.
