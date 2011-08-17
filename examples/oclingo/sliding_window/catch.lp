%
% Catching Balls
%
% An agents has to catch balls that are thrown into its world.
% It can only move horizontally and is not allowed to miss a ball.
%
% Known Issue: sliding window depends only on external clock
%              and does not adjust when planning ahead offline
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
#const window_size=4. % needs to be >= height
#const height=4.
#const width=5.

grid(1..height, 1..width).
#hide grid/2.

% initial position of agent
agent(3, 1).

#external window(1..window_size).

#cumulative t.

% external clock giver
#external window(t + window_size).
#hide window/1.

% auxilliary predicate for sliding window
w(t) :- not window(t + window_size).

%													 sliding window
% BALL														|
%															v

% dropping balls
#external ball(1..width, t).
#hide ball/2.

% extent ball position by initial height
ball(X, height, t) :- ball(X, t),							w(t).

% each step ball falls by one grid field
ball(X, Y-1, t+1) :- ball(X, Y, t), Y > 1,					w(t).

% only stop planning if there are no balls in the air
balls(t) :- ball(X, Y, t), Y > 1,							w(t).
#hide balls/1.

%													 sliding window
% AGENT														|
%															v

% committing to moves
#external commit_move(left;right;none, t).
#hide commit_move/2.

% generate moves
1 { move(left, t), move(none, t), move(right, t) } 1 :-		w(t).

% don't move if there are no balls in the air
move(none, t) :- not balls(t), 								w(t).

% really commit to moves
move(Dir, t) :- commit_move(Dir, t).						% NO

% moving constraint
:- move(right, t), agent(width, t),							w(t).
:- move(left , t), agent(1, t),								w(t).

% effect of moves
agent(X  , t+1) :- agent(X, t), move(none , t).				% NO
agent(X+1, t+1) :- agent(X, t), move(right, t).				% NO
agent(X-1, t+1) :- agent(X, t), move(left , t).				% NO.

% agent has to catch ball
:- ball(X, 1, t+1), not agent(X, t+1),						w(t).


#volatile t.

:- balls(t).
