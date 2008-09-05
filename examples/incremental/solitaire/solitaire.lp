#hide.
#show jump/4.

#base.

%     -------
%     |o|o|o|
%     -------
%     |o|o|o|
% ---------------
% |o|o|o|o|o|o|o|
% ---------------
% |o|o|o| |o|o|o|
% ---------------
% |o|o|o|o|o|o|o|
% ---------------
%     |o|o|o|
%     -------
%     |o|o|o|
%     -------

% the field
field(0, 0).
field(X, Y) :- stone(X, Y).

% the 3x3 quads
stone(-1 ..  1, -3 .. -1).
stone(-3 .. -1, -1 ..  1).
stone( 1 ..  3, -1 ..  1).
stone(-1 ..  1,  1 ..  3).

dir(west;east;north;south).

dir(west , -1,  0).
dir(east ,  1,  0).
dir(north,  0, -1).
dir(south,  0,  1).

stone(X, Y, 0) :- stone(X, Y).

% possible jumps
field(X, Y, D) :- field(X, Y), field(X + 2 * DX, Y + 2 * DY), dir(D, DX, DY).

#cumulative t.

% fix the first jump to avoid symmetries
jump(0,-2,south,t) :- t == 1.

% choose one stone
1 { jump(X, Y, D, t) : field(X, Y, D)  } 1.

% fail if no stone at position
 :- jump(X, Y, D, t), not stone(X, Y, t - 1).
% fail if stone at target
 :- jump(X, Y, D, t), stone(X + 2 * DX, Y + 2 * DY, t - 1), dir(D, DX, DY).
% fail if no stone to jump over
 :- jump(X, Y, D, t), not stone(X + DX, Y + DY, t - 1), dir(D, DX, DY).

% place the new stone
stone(X + 2 * DX, Y + 2 * DY, t) :- jump(X, Y, D, t), dir(D, DX, DY).
% mark the moved stone
jumped(X, Y, t) :- jump(X, Y, D, t).
% remove the stone
removed(X + DX, Y + DY, t) :- jump(X, Y, D, t), dir(D, DX, DY).
% keep other stones
stone(X, Y, t) :- field(X, Y), stone(X, Y, t - 1), not jumped(X, Y, t), not removed(X, Y, t).

#volatile t.

% the last stone shall be in the center
 :- not stone(0, 0, t).
% there shall be at least n free places
 :- { not stone(X, Y, t) : field(X, Y) } n.


