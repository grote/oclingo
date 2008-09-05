#hide.
#show push(X,Y,T).

#base.

% convert incasp sokoban instances
field(X, Y)  :- square(X, Y).
target(X, Y) :- target_square(X, Y).
stone(X, Y)  :- initial_box(X, Y).
start(X, Y)  :- initial_at(X, Y).

stone(X, Y, 0) :- stone(X, Y).
pos(X, Y, 0)   :- start(X, Y).

#cumulative t.

% a place is reachable if we are at the same place 1 timestep ago
reachable(X, Y, t) :- pos(X, Y, t-1).
% a place is reachable if it is next to a reachable place and there is no stone
reachable(X, Y, t) :- reachable(X, Y - 1, t), field(X, Y), not stone(X, Y, t-1).
reachable(X, Y, t) :- reachable(X, Y + 1, t), field(X, Y), not stone(X, Y, t-1).
reachable(X, Y, t) :- reachable(X + 1, Y, t), field(X, Y), not stone(X, Y, t-1).
reachable(X, Y, t) :- reachable(X - 1, Y, t), field(X, Y), not stone(X, Y, t-1).

% perform a valid move (somewhere)
1 { pos(X, Y, t) : field(X, Y) } 1.
 :- pos(X, Y, t), not reachable(X, Y, t), t > 0.

% push a box
1 { push(X, Y, t) : field(X, Y) } 1.
% only one can be true
pushed(X + 1, Y, t) :- push(X, Y, t), pos(X + 2, Y, t), not stone(X, Y, t - 1), stone(X + 1, Y, t - 1).
pushed(X - 1, Y, t) :- push(X, Y, t), pos(X - 2, Y, t), not stone(X, Y, t - 1), stone(X - 1, Y, t - 1).
pushed(X, Y + 1, t) :- push(X, Y, t), pos(X, Y + 2, t), not stone(X, Y, t - 1), stone(X, Y + 1, t - 1).
pushed(X, Y - 1, t) :- push(X, Y, t), pos(X, Y - 2, t), not stone(X, Y, t - 1), stone(X, Y - 1, t - 1).

testP1(t) :- pushed(X, Y, t).
testP2(t) :- push(X, Y, t).

% 1 box has to be pushed
:- testP2(t), not testP1(t).

stone(X, Y, t) :- push(X, Y, t).
stone(X, Y, t) :- stone(X, Y, t - 1), not pushed(X, Y, t), field(X, Y).

#volatile t.
 :- not target(X, Y), stone(X, Y, t).

#base.
