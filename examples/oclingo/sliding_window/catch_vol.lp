%
% Catching Balls
%
% An agents has to catch balls that are thrown into its world.
% It can only move horizontally and is not allowed to miss a ball.
%
% - - - - - -
% |B| | | | | 4
% -----------
% | | | | | | 3
% -----------
% | | | | | | 2
% ===========
% | | |A| | | 1
% -----------
%  1 2 3 4 5 

#base.
#const height=4.
#const width=5.

grid(1..height, 1..width).
#hide grid/2.

% initial position of agent
agent(3, 1).

#cumulative t.

% dropping balls
#external ball(1..width, t).
#hide ball/2.

% extent ball position by initial height
ball(X, height, t) :- ball(X, t).

% each step ball falls by one grid field
ball(X, Y-1, t+1) :- ball(X, Y, t), Y > 1.

% committing to moves
#external commit_move(left;right;none, t).
#hide commit_move/2.

% really commit to moves
:- not move(Dir, t), commit_move(Dir, t).

% generate moves
1 { move(left, t), move(none, t), move(right, t) } 1.

% effect of moves
agent(X  , t+1) :- agent(X, t), move(none , t).
agent(X+1, t+1) :- agent(X, t), move(right, t).
agent(X-1, t+1) :- agent(X, t), move(left , t).

#volatile t.

:- balls(t).

#volatile t : 4.

% only stop planning if there are no balls in the air
balls(t) :- ball(X, Y, t), Y > 1.
#hide balls/1.

% don't move if there are no balls in the air
:- not move(none, t), not balls(t).

% moving constraint
:- move(right, t), agent(width, t).
:- move(left , t), agent(1, t).

% agent has to catch ball
:- ball(X, 1, t+1), not agent(X, t+1).
