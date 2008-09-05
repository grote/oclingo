#hide.
#show moved(X,Y).

#const blocks=10.

#base.

% the table
loc(0).
% the blocks
block(1..blocks).
% blocks are locations
loc(X) :- block(X).

% initial situation the blocks are stacked
on(X, X - 1, 0) :- block(X).

#cumulative t.

% move one block
1 { mv(X,Y,t) : block(X) : loc(Y) : X != Y } 1.

% mark moved blocks
moved(X, t) :- mv(X,Y,t).

% move the stone
on(X,Y,t) :- mv(X,Y,t).
% all other stones stay where they are
on(X,Y,t) :- block(X), loc(Y), not moved(X, t), on(X,Y,t - 1).

% assert incorrect moves
% only blocks on top can be moved
 :- mv(X, Y, t), on(Z, X, t-1).
% blocks can only be put on topmost blocks
 :- mv(X, Y, t), block(Y), on(Z, Y, t - 1).

#volatile t.
% all blocks have to be on the table
 :- block(X), not on(X, 0, t).

