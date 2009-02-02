#const rows=5.
#const cols=5.

field(1..rows,1..cols).

connect(X,Y,n) :- field(X,Y), not nconnect(X,Y,n).
nconnect(X,Y,n) :- field(X,Y), not connect(X,Y,n).
connect(X,Y,s) :- field(X,Y), not nconnect(X,Y,s).
nconnect(X,Y,s) :- field(X,Y), not connect(X,Y,s).
connect(X,Y,e) :- field(X,Y), not nconnect(X,Y,e).
nconnect(X,Y,e) :- field(X,Y), not connect(X,Y,e).
connect(X,Y,w) :- field(X,Y), not nconnect(X,Y,w).
nconnect(X,Y,w) :- field(X,Y), not connect(X,Y,w).
connect(X,Y,n) :- field(X,Y), not connect(X,Y,s), not connect(X,Y,e), not connect(X,Y,w).
connect(X,Y,s) :- field(X,Y), not connect(X,Y,n), not connect(X,Y,e), not connect(X,Y,w).
connect(X,Y,e) :- field(X,Y), not connect(X,Y,n), not connect(X,Y,s), not connect(X,Y,w).
connect(X,Y,w) :- field(X,Y), not connect(X,Y,n), not connect(X,Y,s), not connect(X,Y,e).

1 { init_on(X,Y) : field(X,Y) } 1.
1 { goal_on(X,Y) : field(X,Y) } 1.
:- init_on(X,Y), goal_on(X,Y).

#hide nconnect(X,Y,D).

% CALL:
%
% gringo -c rows=X -c cols=Y laby_gen.lp | clasp --eq=0 --rand-freq=1 --seed=S
