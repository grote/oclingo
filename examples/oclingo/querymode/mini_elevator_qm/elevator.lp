#cumulative t.

% Elevator can only be at floors which are reachable in from previous time step ...
1 { atFloor(F-1;F+1, t) } 1 :- atFloor(F, t-1), floor(F).
% ... and which are defined
:- atFloor(F, T), not floor(F).

% Choice for requests by user at floor F
{ request(F,t) : floor(F) }.

% requested/2 memorizes yet unfullfiled jobs at time point t
requested(F, t) :- request(F,t), not atFloor(F, t), floor(F).
requested(F, t) :- requested(F, t-1), not atFloor(F, t), floor(F).

% Solutions must fullfil all requests
goal(t) :- not requested(F, t) : floor(F).

#volatile t.
:- not goal(t).
