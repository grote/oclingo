% -----------------
% |   |   |   | G |
% -----------------
% |   | W | P |   |
% -----------------
% |   |   | P |   |
% -----------------
% | S |   |   |   |
% -----------------

#base.

#const height=4.
#const width=4.

field(1..width, 1..height).
#hide field/2.

start(1, 1).
#hide start/2.

% initial location of wumpus
wumpus_at(2, 3, 0).

gold(4, 4).
#hide gold/2.

pit(3, 2).  
pit(3, 3).
#hide pit/2.
