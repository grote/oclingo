#ishift a -> next_a / 2.
#ishift c / 2.

#base.
{ p(1) }.
c(1,1) :- p(1).
next_a(1,1) :- p(1).

#cumulative t.
{ p(t) }.
c(1,t+1) :- c(1,t), p(t).
next_a(1,t+1) :- a(1,t), p(t).

x(X,t) :- c(X,t).
y(X,t) :- a(X,t).
z(X,t+1) :- next_a(X,t+1).

