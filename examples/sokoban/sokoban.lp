#hide.
#show push(X,Y,T).

time(1..timesteps).

stone(X, Y, 0) :- stone(X, Y).
pos(X, Y, 0)   :- start(X, Y).

% a place is reachable if we are at the same place 1 timestep ago
reachable(X, Y, T + 1) :- pos(X, Y, T).
% a place is reachable if it is next to a reachable place and there is no stone
reachable(X, Y, T) :- reachable(X, Y - 1, T), field(X, Y), not stone(X, Y, T-1), time(T).
reachable(X, Y, T) :- reachable(X, Y + 1, T), field(X, Y), not stone(X, Y, T-1), time(T).
reachable(X, Y, T) :- reachable(X + 1, Y, T), field(X, Y), not stone(X, Y, T-1), time(T).
reachable(X, Y, T) :- reachable(X - 1, Y, T), field(X, Y), not stone(X, Y, T-1), time(T).

% perform a valid move (somewhere)
1 { pos(X, Y, T) : field(X, Y) } 1 :- time(T).
 :- pos(X, Y, T), not reachable(X, Y, T), T > 0.

% push a box
1 { push(X, Y, T) : field(X, Y) } 1 :- time(T).
% only one can be true
pushed(X + 1, Y, T) :- push(X, Y, T), pos(X + 2, Y, T), not stone(X, Y, T - 1), stone(X + 1, Y, T - 1).
pushed(X - 1, Y, T) :- push(X, Y, T), pos(X - 2, Y, T), not stone(X, Y, T - 1), stone(X - 1, Y, T - 1).
pushed(X, Y + 1, T) :- push(X, Y, T), pos(X, Y + 2, T), not stone(X, Y, T - 1), stone(X, Y + 1, T - 1).
pushed(X, Y - 1, T) :- push(X, Y, T), pos(X, Y - 2, T), not stone(X, Y, T - 1), stone(X, Y - 1, T - 1).

testP1(T) :- pushed(X, Y, T).
testP2(T) :- push(X, Y, T).

% 1 box has to be pushed
:- testP2(T), not testP1(T).

stone(X, Y, T) :- push(X, Y, T).
stone(X, Y, T) :- stone(X, Y, T - 1), not pushed(X, Y, T), time(T), field(X, Y).

final(X, Y) :- stone(X, Y, timesteps).
% :- target(X, Y), not final(X, Y).
 :- not target(X, Y), final(X, Y).

%detect very simple deadlocks
% :- stone(X, Y, T), deadlock(X, Y).

% deadlock(X, Y) :- field(X, Y), not target_reachable(X, Y).

% target_reachable(X, Y) :- target(X, Y).
% target_reachable(X + 1, Y) :- target_reachable(X, Y), field(X, Y), field(X + 1, Y), field(X + 2, Y).
% target_reachable(X - 1, Y) :- target_reachable(X, Y), field(X, Y), field(X - 1, Y), field(X - 2, Y).
% target_reachable(X, Y + 1) :- target_reachable(X, Y), field(X, Y), field(X, Y + 1), field(X, Y + 2).
% target_reachable(X, Y - 1) :- target_reachable(X, Y), field(X, Y), field(X, Y - 1), field(X, Y - 2).

