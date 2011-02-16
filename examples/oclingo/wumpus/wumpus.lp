%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Wumpus World encoding
%   based on the example from Norvig/Russell Artificial Intelligence
%
% agent does no reasoning, so breezes and stenches are ignored for now
%

#base.

% generate breezes from pits
%breeze(X, Y) :- pit(X+1, Y), field(X, Y).
%breeze(X, Y) :- pit(X-1, Y), field(X, Y).
%breeze(X, Y) :- pit(X, Y-1), field(X, Y).
%breeze(X, Y) :- pit(X, Y+1), field(X, Y).
%#hide breeze/2.
%#hide stench/3.

direction(up;down;left;right).
#hide direction/1.

at(X, Y, 0) :- start(X, Y).

#cumulative t.

% generate stench from wumpus
%stench(X, Y, t) :- wumpus_at(X+1, Y, t), field(X, Y).
%stench(X, Y, t) :- wumpus_at(X-1, Y, t), field(X, Y).
%stench(X, Y, t) :- wumpus_at(X, Y-1, t), field(X, Y).
%stench(X, Y, t) :- wumpus_at(X, Y+1, t), field(X, Y).

% cause bumps if moved into wall
bump(t) :- go(up, t-1),    at(X, Y, t-1), not field(X, Y+1).
bump(t) :- go(down, t-1),  at(X, Y, t-1), not field(X, Y-1).
bump(t) :- go(left, t-1),  at(X, Y, t-1), not field(X-1, Y).
bump(t) :- go(right, t-1), at(X, Y, t-1), not field(X+1, Y).

% position is inertial
at(X, Y, t) :- at(X, Y, t-1), field(X, Y), not go(D, t-1) : direction(D).

% position change after moving
at(X, Y, t) :- at(X, Y-1, t-1), field(X, Y), go(up, t-1).
at(X, Y, t) :- at(X, Y+1, t-1), field(X, Y), go(down, t-1).
at(X, Y, t) :- at(X+1, Y, t-1), field(X, Y), go(left, t-1).
at(X, Y, t) :- at(X-1, Y, t-1), field(X, Y), go(right, t-1).

% wumpus can move
wumpus_at(X, Y, t) :- wumpus_at(X, Y-1, t-1), field(X, Y), wumpus_moves(up, t-1).
wumpus_at(X, Y, t) :- wumpus_at(X, Y+1, t-1), field(X, Y), wumpus_moves(down, t-1).
wumpus_at(X, Y, t) :- wumpus_at(X+1, Y, t-1), field(X, Y), wumpus_moves(left, t-1).
wumpus_at(X, Y, t) :- wumpus_at(X-1, Y, t-1), field(X, Y), wumpus_moves(right, t-1).

#external wumpus_moves(D, t-1) : direction(D).
#external agent_moves(D, t) : direction(D).

% wumpus waits if it doesn't move
wumpus_at(X, Y, t) :- wumpus_at(X, Y, t-1), field(X, Y), not wumpus_moves(D, t-1) : direction(D).

% no jumping over wumpus allowed
:- go(right, t), wumpus_moves(left, t), at(X, Y, t), wumpus_at(X+1, Y, t), field(X, Y).
:- go(left, t), wumpus_moves(right, t), at(X+1, Y, t), wumpus_at(X, Y, t), field(X, Y).
:- go(down, t), wumpus_moves(up, t), at(X, Y+1, t), wumpus_at(X, Y, t), field(X, Y).
:- go(up, t), wumpus_moves(down, t), at(X, Y, t), wumpus_at(X, Y+1, t), field(X, Y).

% Dying conditions
dead(t) :- at(X, Y, t), pit(X, Y).
dead(t) :- at(X, Y, t), wumpus_at(X, Y, t).

% dead once always dead
dead(t) :- dead(t-1).

%%%%%%%%%%%%%%%%%%%%%%%%%%%
%						  %
%   Agent's perspective   %
%						  %
%%%%%%%%%%%%%%%%%%%%%%%%%%%

% generate moves
1 { go(D, t) : direction(D) } 1 :- not dead(t), not won(t).

% gone if controller said so
go(D, t) :- agent_moves(D, t), direction(D).

% don't run into a wall twice in a row
:- bump(t), go(D, t), go(D, t-1), direction(D).

% grab gold if you see/feel the glitter
grab(t) :- at(X, Y, t), gold(X, Y), field(X, Y).

% carry the gold around after you grabed it
carry(gold, t) :- grab(t).
carry(gold, t) :- carry(gold, t-1).
#hide carry/2.

won(t) :- carry(gold, t), at(X, Y, t), start(X, Y), not dead(t).

% Actions
% shoot
% climb


#volatile t.

:- not won(t).
