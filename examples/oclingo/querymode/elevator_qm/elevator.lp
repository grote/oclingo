%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% Elevator control program		%
% (c) 2010 Torsten Grote		%
% GPLv3+						%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#base.

% Actions
% ^^^^^^^
% up(t)
% down(t)
% board(t)
% depart(t)

% Fluents
% ^^^^^^^
% currentFloor(F, t).
% on(F, t).
% open(t).

#domain floor(F).

currentFloor(0, 0).
closed(0).

#hide.
#show up/1.
#show down/1.
#show goal/1.
%#show board/1.
%#show depart/1.
#show currentFloor/2.
#show on/2.
#show ext_on/2.
#show open/1.
%#show keep_direction/2.

#cumulative t.

%%%%%%%%%%%%%%%%%%%
% Auxiliary rules %
%%%%%%%%%%%%%%%%%%%

{on(F, t) : floor(F)}.
%on(F, t) :- ext_on(F, t).

% on and off are inertial and exclusive
on(F, t) :- not off(F, t), on(F, t-1).
off(F, t) :- not on(F, t), off(F, t-1).
:- on(F, t), off(F, t).

% open and closed are exclusive
closed(t) :- not open(t).
:- open(t), closed(t).

% up and down negations
neg_up(t) :- not up(t).
:- up(t), neg_up(t).
neg_down(t) :- not down(t).
:- down(t), neg_down(t).

% currentFloor is inertial if elevator did not move
currentFloor(F, t) :- currentFloor(F, t-1), not up(t-1), not down(t-1).

%#external ext_on/2.

%%%%%%%%%%%%%%%%%%%%
% Elevator control %
%%%%%%%%%%%%%%%%%%%%

% up and down happen non-deterministically
up(t) :-
	currentFloor(F, t),
	F<f-1,			% don't go over the roof top
	closed(t),
	not board(t),
	not down(t),
	not keep_direction(down, t),	% don't change direction
	not goal(t),	% don't move after goal has been reached
	not neg_up(t).	% triggers non-determism
down(t) :-
	currentFloor(F, t),
	F>0,				% don't go down into basement
	closed(t),
	not board(t),
	not up(t),
	not keep_direction(up, t),	% don't change direction
	not goal(t),		% don't move after goal has been reached
	not neg_down(t).	% triggers non-determism

% keep current direction if there are still targets along the way
keep_direction(up, t) :-
	was_up(t),				% was going up before
	currentFloor(F, t),	
	on_above(F, t).				% light on at higher floor
keep_direction(down, t) :-
	was_down(t),			% was going down before
	currentFloor(F, t),
	on_below(F, t).			% light on at lower floor

% define current direction
was_up(t) :- up(t-1).
was_up(t) :- board(t-1), up(t-2).
was_down(t) :- down(t-1).
was_down(t) :- board(t-1), down(t-2).

% light is on above F
on_above(F-1, t) :- on(F, t), F>0.
on_above(F-1, t) :- on_above(F, t), F>0.
%#show on_above/2.

% light is on below F
on_below(F+1, t) :- on(F, t), F<f-1.
on_below(F+1, t) :- on_below(F, t), F<f-1.
%#show on_below/2.

% elevator moves after up/down actions
currentFloor(F+1, t) :- currentFloor(F, t-1), up(t-1).
currentFloor(F-1, t) :- currentFloor(F, t-1), down(t-1).

% board/serve the floor if light is on
board(t) :- currentFloor(F, t), on(F, t).
open(t) :- board(t).

% depart after being open and close door
depart(t) :- board(t-1).
closed(t) :- depart(t).

% turn light off after departed from floor
off(F, t) :- depart(t), currentFloor(F, t).

% don't sit around idle, if there's work to do
:- not up(t-1), not down(t-1), not board(t-1), t>1, not goal(t-1).

% don't be at two floors at the same time
:- currentFloor(F1, t), currentFloor(F2, t), F1!=F2.

% the goal is to serve all floors resp. turn all lights off
goal(t) :- not on(Floor, t) : floor(Floor).

#volatile t.

% fulfill goal
:- not goal(t).
