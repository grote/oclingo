h(a(1)).
{ h(b(X)), h(c(X)) } :- h(a(X)).
h(d(X)) :- h(b(X)), h(c(X)).

a(X,X,X) :- h(a(X)), c.
b      :- a(1,2,X).
b      :- a(1,1,X).
c      :- b.
