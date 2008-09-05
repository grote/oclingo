p(1..3, 1..3).
{ a(X,Y) : p(Y + 1, X) : p(Y, X + 1) }.
b(X,Y) :- a(X,Y).

p(1..3).
{ a(X) : p(X) }.
{ b(X) : p(X) }.

% X is local and both aggregates have nothing!!! to do with each other
a :- 2 {a(X) : p(X) }, 2 {a(X) : p(X)}.

{c : p(1..3)}.


