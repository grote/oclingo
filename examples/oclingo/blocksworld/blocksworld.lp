%%%%%%%%%%%
% Blocksworld with raining blocks

#base.

% normal blocks are blocks
block(B) :- nblock(B).
% raining blocks are blocks
block(B) :- rblock(B).
#hide block/1.

% normal blocks are locations
loc(L) :- nblock(L).
loc(table).
#hide loc/1.

#cumulative t.

#external rain(B1, B2, t) : rblock(B1) : nblock(B2).

% move one block each step
1 { move(B, L, t) : block(B) : loc(L) : B!=L } 1 :- not goal(t).

% block is at new location if moved to it
on(B, L, t) :- move(B, L, t-1), block(B), loc(L).

% block's location is inertial
on(B, L, t) :- on(B, L, t-1), block(B), loc(L), not move(B, L2, t-1) : loc(L2).

% only move blocks that don't have blocks on top of them
:- on(B, C, t), move(C, L, t), block(B;C), loc(L), B!=C.

% only move blocks on blocks that don't have blocks on top of them
:- on(B, C, t), move(D, C, t), block(B;C;D), B!=C, B!=D.

% Don't move blocks to the same location
:- on(B, L, t), move(B, L, t), block(B), loc(L).

% Don't move blocks that did not rain yet
:- move(B, L, t), not rained(B, t), rblock(B), loc(L).

% It's raining blocks
on(B, L, t) :- rain(B, L, t-1), rblock(B), loc(L).

% remember blocks that came down raining
rained(B, t) :- rain(B, L, t), rblock(B), loc(L).
rained(B, t) :- rained(B, t-1), rblock(B).
#hide rained/2.

% get goal condition form instance file
goal(t) :- on(B, L, t) : goal_on(B, L).

#volatile t.

:- not goal(t).
