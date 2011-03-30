% -----------------
% |   |   |   | P |
% -----------------
% | W | G | P |   |
% -----------------
% |   |   |   |   |
% -----------------
% | S |   | P |   |
% -----------------

#base.

#const height=4.
#const width=4.

field(1..width, 1..height).
#hide field/2.

start(1, 1).
#hide start/2.

% initial location of wumpus
wumpus_at(1, 3, 0).

gold(2, 3).
#hide gold/2.

pit(3, 1).  
pit(3, 3).
pit(4, 4).
#hide pit/2.
