#base.

nblock(0..4).
#hide nblock/1.

% raining blocks
rblock(50..55).
#hide rblock/1.

on(B, table, 0) :- nblock(B).


goal_on(0, table).
goal_on(1, 0).
goal_on(2, 1).
goal_on(3, 2).
goal_on(4, 3).
#hide goal_on/2.
