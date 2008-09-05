#hide.
#show mv/3.

#base.

#const n = 10.

%*
% a more interesting goal
#const blocks = n * (n + 1) / 2.

p(T + X - 1, T .. T + X - 1) :- X = 1..n, T = blocks - X * (X + 1) / 2 + 1.

goal(S, t) :- p(S, X).
goal(X, X + 1) :- p(S, X), p(S ,X + 1).
*%

% a very simple goal
#const blocks = n.

goal(1, t).
goal(X + 1, X) :- bl(X), bl(X + 1).

% all blocks
bl(1..blocks).

% all possible locations
loc(t).
loc(X) :- bl(X).

% the initial stack of blocks
on(blocks, t, 0).
on(X, X + 1, 0) :- bl(X), X < blocks.

#cumulative k.

% plan generator
1 { mv(X,Y,k) : bl(X) : loc(Y) : X != Y } 1.
moved(X, k) :- mv(X, Y, k).

% move the block
on(X,Y,k) :- mv(X,Y,k).
% inertia
on(X,Y,k) :- on(X,Y,k-1), bl(X), loc(Y), not moved(X, k).

% a block is blocked if there is sth. on top of it
blocked(Y, k) :- bl(Y), on(X, Y, k - 1).

% blocked blocks cannot be moved
 :- mv(X, Y, k), blocked(X, k).
% nothing can be put on a blocked block
 :- mv(X, Y, k), blocked(Y, k).

#volatile k.

% the goal is the reversed stack
 :- goal(X, Y), not on(X, Y, k).

