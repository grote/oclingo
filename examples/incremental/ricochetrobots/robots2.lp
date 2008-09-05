% Ricochet Robots (Rasende Roboter) by Alex Randolph
% ---
% http://en.wikipedia.org/wiki/Ricochet_Robot
% http://www.ricochetrobots.com
%
% ASP encoding by torsten Schaub
%
% --- remarks
%   - (1,1) is north-west corner
%   - boards are specified in terms of
%     - robot positions at time 0:       pos/4
%     - barriers relative to directions: barrier/3
%   - a goal is a single robot position,
%      eg. :- not position(red,6,7,42),
%     given in the respective board description files,
%      eg. board16-1.

%#domain time(t).

%time(1..lastime).

#base.

dim(1..dimension).

robot(red).
robot(blue).
robot(green).
robot(yellow).

direction(north).
direction(east).
direction(south).
direction(west).

% blockages
blocked(I,J,D) :- barrier(I,J,D),direction(D),dim(I;J).

blocked(1..dimension,1           ,north).
blocked(   dimension,1..dimension,east ).
blocked(1..dimension,   dimension,south).
blocked(           1,1..dimension,west ).

blocked(I+1,J,west ) :- blocked(I,J,east ),dim(I;J),I<dimension.
blocked(I,J+1,north) :- blocked(I,J,south),dim(I;J),J<dimension.
blocked(I-1,J,east ) :- blocked(I,J,west ),dim(I;J),I>1.
blocked(I,J-1,south) :- blocked(I,J,north),dim(I;J),J>1.

% this should simplify sth.
dir(west, -1,  0).
dir(east,  1,  0).
dir(north, 0, -1).
dir(south, 0,  1).

#cumulative t.

% which places are reachable by a robot
reachable(R, I + DI, J + DJ, D, t) :- not blocked2(I, J, t), dir(D, DI, DJ), go(R, D, t), position(R, I, J, t - 1), dim(I;J).
reachable(R, I + DI, J + DJ, D, t) :- not blocked2(I, J, t), reachable(R, I, J, D, t), dir(D, DI, DJ), go(R, D, t), dim(I;J).

% no crossing of blockages or robots
blocked2(I, J, t) :- blocked(I, J, D), go(R, D, t).
blocked2(I - DI, J - DJ, t) :- position(R, I, J, t - 1), selectDir(D, t), dir(D, DI, DJ).

% move it (no stopping if there is neither a blockage nor a robot)
position(R, I, J, t) :- reachable(R, I, J, D, t), not reachable(R, I + DI, J + DJ, D, t), dir(D, DI, DJ), go(R, D, t).

% one has to be moved
moved(t) :- reachable(R, I, J, D, t).
 :- not moved(t).

% inertia
position(R, I, J, t) :- position(R, I, J, t - 1), not selectRobot(R, t), robot(R), dim(I;J).

% plan generator
1 { selectRobot(R, t) : robot(R) } 1.
1 { selectDir(D,t)    : direction(D) } 1.

go(R, D, t) :- selectRobot(R, t), selectDir(D,t).

#hide.
%#show moved/4.
#show go/3.
%#show position/4.
%#show reachable/4.

