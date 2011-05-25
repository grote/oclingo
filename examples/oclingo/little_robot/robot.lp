
#base.

direction(up;down;left;right).
#hide direction/1.

at(X, Y, 0) :- start(X, Y).

#external goal(X, Y) : field(X, Y).

#cumulative t.

% position is inertial
at(X, Y, t) :- at(X, Y, t-1), field(X, Y), not go(D, t-1) : direction(D).

% position change after moving
at(X, Y, t) :- at(X, Y-1, t-1), field(X, Y), go(up, t-1).
at(X, Y, t) :- at(X, Y+1, t-1), field(X, Y), go(down, t-1).
at(X, Y, t) :- at(X+1, Y, t-1), field(X, Y), go(left, t-1).
at(X, Y, t) :- at(X-1, Y, t-1), field(X, Y), go(right, t-1).

% use external predicate to allow commitment to moves
#external robot_moved/2.

% generate moves
1 { go(D, t) : direction(D) } 1 :- not won(t).

% commit to movement of the robot
go(D, t) :- robot_moved(D, t), direction(D).

won(t) :- at(X, Y, t), goal(X, Y).


#volatile t.

:- not won(t), t>1.
