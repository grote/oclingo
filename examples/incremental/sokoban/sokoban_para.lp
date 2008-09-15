#hide.
#show push_box/4.
#show push_to/4.
#show at/3.

#base.

%*
% convert my instances
square(X, Y)        :- field(X, Y).
target_square(X, Y) :- target(X, Y).
initial_box(X, Y)   :- stone(X, Y).
initial_at(X, Y)    :- start(X, Y).
*%

% possible directions
direction(u).
direction(d).
direction(r).
direction(l).

% possible moves
dir(u,  0,  1).
dir(d,  0, -1).
dir(r,  1,  0).
dir(l, -1,  0).

% box positions at timestep 0
box(X, Y, 0) :- initial_box(X, Y).

% player at timestep 0
at(X, Y, 0) :- initial_at(X, Y).

#cumulative k.

% reachable places at timestep k
routePre(X, Y, k) :- at(X, Y, k - 1), square(X, Y).
routePre(X, Y, k) :- dir(D, DX, DY), routePre(X + DX, Y + DY, k), not box(X, Y, k - 1), square(X, Y).

routePost(X, Y, k) :- at(X, Y, k), square(X, Y).
routePost(X, Y, k) :- dir(D, DX, DY), routePost(X + DX, Y + DY, k), not box(X, Y, k - 1), not box(X, Y, k), square(X, Y).

% move the player (this could be restricted to moves behind boxes)
1 { at(X, Y, k) : square(X, Y) } 1.
 :- not routePre(X, Y, k), at(X, Y, k).
 :- box(X, Y, k), at(X, Y, k).

% select at least one box to push into exactly one direction (must be reachable and have a box to push)
1 { push_box(X, Y, D, k) : square(X, Y) : direction(D) }.
 :- push_box(X, Y, D, k), not routePre(X - DX, Y - DY, k), dir(D, DX, DY).
 :- push_box(X, Y, D, k), not routePost(X - DX, Y - DY, k), dir(D, DX, DY).
 :- push_box(X, Y, D, k), not box(X, Y, k - 1).
 :- push_box(X, Y, D1, k), push_box(X, Y, D2, k), D1 != D2.

% push the box 1 or more fields into direction D (but at least one)
push(X + DX, Y + DY, D, k) :- push_box(X, Y, D, k), dir(D, DX, DY).
{ push(X + DX, Y + DY, D, k) } :- push(X, Y, D, k), square(X, Y), dir(D, DX, DY).
 :- push(X, Y, D, k), not square(X, Y).
 :- push(X, Y, D, k), box(X, Y, k - 1).

% the last push is the final location
push_to(X, Y, D, k) :- push(X, Y, D, k), not push(X + DX, Y + DY, D, k), dir(D, DX, DY).
 :- push_to(X, Y, D1, k), push_to(X, Y, D2, k), D1 != D2.

% put the pushed box at the new location
box(X, Y, k) :- push_to(X, Y, D, k).
% all other boxes stay where they are
box(X, Y, k) :- box(X, Y, k - 1), square(X, Y), not push_box(X, Y, D, k) : direction(D).

% assert redundant moves
% :- push_box(X, Y, D, k - 1), push_from(X, Y, D, k).

#volatile k.

% the goal :)
 :- target_square(X, Y), not box(X, Y, k).

#base.

