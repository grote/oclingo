#hide.
#show moved(X,Y).

#const blocks=10.
#const timesteps=blocks-1.

% the objects
time(1..timesteps).
loc(0).
block(1..blocks).
loc(X) :- block(X).

% initial situation
on(X, X - 1, 0) :- block(X).

% select 1 move per timestep
1 { mv(X,Y,T) : block(X) : loc(Y) : X != Y } 1 :- time(T).

% mark moved stones
moved(X, T) :- mv(X,Y,T).

% move the stone
on(X,Y,T) :- mv(X,Y,T).
on(X,Y,T) :- time(T), block(X), loc(Y), not moved(X, T), on(X,Y,T - 1).

% assert incorrect moves
% only blocks on top can be moved
 :- mv(X, Y, T), T1 = T - 1, on(Z, X, T1).
% blocks can only be put on topmost blocks
 :- mv(X, Y, T), T1 = T - 1, block(Y), on(Z, Y, T1).

% the speed difference between this and the above rules is amazing!!!
% :- mv(X, Y, T), on(Z, X, T - 1).
% :- mv(X, Y, T), block(Y), on(Z, Y, T - 1).

% goal
% all blocks have to be on the table
 :- block(X), not on(X, 0, timesteps).

% only the upper block may lie on the table 
% this goal doenst make a big difference
% #const timesteps=blocks.
% :- on(X, 0, timesteps), X < blocks.

