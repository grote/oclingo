#cumulative t.

1 { atFloor(F-1;F+1, t) } 1 :- atFloor(F, t-1), floor(F).
:- atFloor(F, T), not floor(F).

#external request(F, t) : floor(F).

requested(F, t) :- request(F, t), not atFloor(F, t), floor(F).
requested(F, t) :- requested(F, t-1), not atFloor(F, t), floor(F).
goal(t) :- not requested(F, t) : floor(F).

#volatile t.

:- not goal(t).
