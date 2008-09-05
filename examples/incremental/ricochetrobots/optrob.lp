% Some optimizations

#cumulative t.

% --- forbid redundant return moves
:- go(R,east,t - 1),go(R,west,t).
:- go(R,west,t - 1),go(R,east,t).
:- go(R,south,t - 1),go(R,north,t).
:- go(R,north,t - 1),go(R,south,t).


%:- not position(red,           1,        1,t),time(t). % don't move red    robot
%:- not position(green, dimension,        1,t),time(t). % don't move green  robot
%:- not position(blue,          1,dimension,t),time(t). % don't move blue   robot
%:- not position(yellow,dimension,dimension,t),time(t). % don't move yellow robot
