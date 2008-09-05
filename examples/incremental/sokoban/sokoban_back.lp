#hide.
#show push_from(X,Y,K).
#show push_to(X,Y,K).

#base.

% convert my instances
square(X, Y)        :- field(X, Y).
target_square(X, Y) :- target(X, Y).
initial_box(X, Y)   :- stone(X, Y).
initial_at(X, Y)    :- start(X, Y).

box(X,Y,0) :- target_square(X,Y).

direction(u).
direction(d).
direction(r).
direction(l).

#cumulative k.

1{push_to(X,Y,-k) : square(X,Y)}1.
:- push_to(X,Y,-k), not box(X,Y,-k+1), square(X,Y).

1{push_dir(D,-k) : direction(D)}1.
:- push_to(X,Y,-k), push_dir(u,-k), square(X,Y), not square(X,Y-1).
:- push_to(X,Y,-k), push_dir(d,-k), square(X,Y), not square(X,Y+1).
:- push_to(X,Y,-k), push_dir(r,-k), square(X,Y), not square(X-1,Y).
:- push_to(X,Y,-k), push_dir(l,-k), square(X,Y), not square(X+1,Y).

push(X,Y-1,u,-k) :- push_to(X,Y,-k), push_dir(u,-k), square(X,Y).
push(X,Y+1,d,-k) :- push_to(X,Y,-k), push_dir(d,-k), square(X,Y).
push(X-1,Y,r,-k) :- push_to(X,Y,-k), push_dir(r,-k), square(X,Y).
push(X+1,Y,l,-k) :- push_to(X,Y,-k), push_dir(l,-k), square(X,Y).
{push(X,Y-1,u,-k)} :- push(X,Y,u,-k), square(X,Y).
{push(X,Y+1,d,-k)} :- push(X,Y,d,-k), square(X,Y).
{push(X-1,Y,r,-k)} :- push(X,Y,r,-k), square(X,Y).
{push(X+1,Y,l,-k)} :- push(X,Y,l,-k), square(X,Y).

:- push(X,Y,D,-k), box(X,Y,-k+1), square(X,Y), direction(D).
:- push(X,Y,u,-k), square(X,Y), not square(X,Y-1).
:- push(X,Y,d,-k), square(X,Y), not square(X,Y+1).
:- push(X,Y,r,-k), square(X,Y), not square(X-1,Y).
:- push(X,Y,l,-k), square(X,Y), not square(X+1,Y).

push_from(X,Y,-k) :- push(X,Y,u,-k), not push(X,Y-1,u,-k), square(X,Y).
push_from(X,Y,-k) :- push(X,Y,d,-k), not push(X,Y+1,d,-k), square(X,Y).
push_from(X,Y,-k) :- push(X,Y,r,-k), not push(X-1,Y,r,-k), square(X,Y).
push_from(X,Y,-k) :- push(X,Y,l,-k), not push(X+1,Y,l,-k), square(X,Y).

box(X,Y,-k) :- push_from(X,Y,-k), square(X,Y).
box(X,Y,-k) :- box(X,Y,-k+1), not push_to(X,Y,-k), square(X,Y).

at(X,Y-1,-k) :- push_from(X,Y,-k), push_dir(u,-k), square(X,Y).
at(X,Y+1,-k) :- push_from(X,Y,-k), push_dir(d,-k), square(X,Y).
at(X-1,Y,-k) :- push_from(X,Y,-k), push_dir(r,-k), square(X,Y).
at(X+1,Y,-k) :- push_from(X,Y,-k), push_dir(l,-k), square(X,Y).

:- at(X,Y,-k), box(X,Y,-k+1), square(X,Y).

route(X,Y,-k) :- at(X,Y,-k), square(X,Y).
route(X,Y,-k) :- route(X+1,Y,-k), not box(X,Y,-k), square(X,Y).
route(X,Y,-k) :- route(X-1,Y,-k), not box(X,Y,-k), square(X,Y).
route(X,Y,-k) :- route(X,Y+1,-k), not box(X,Y,-k), square(X,Y).
route(X,Y,-k) :- route(X,Y-1,-k), not box(X,Y,-k), square(X,Y).

:- push_to(X,Y,-k), push_dir(u,-k), not route(X,Y-1,-k+1), square(X,Y), k > 1.
:- push_to(X,Y,-k), push_dir(d,-k), not route(X,Y+1,-k+1), square(X,Y), k > 1.
:- push_to(X,Y,-k), push_dir(r,-k), not route(X-1,Y,-k+1), square(X,Y), k > 1.
:- push_to(X,Y,-k), push_dir(l,-k), not route(X+1,Y,-k+1), square(X,Y), k > 1.

:- push_to(X,Y,-k), push_from(X,Y,-k+1), push_dir(D,-k), push_dir(D,-k+1), direction(D), square(X,Y), k > 1.

#volatile k.

:- initial_box(X,Y), not box(X,Y,-k).
:- initial_at(X,Y), not route(X,Y,-k).

#base.
