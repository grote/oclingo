#base.

player(o).
player(x).
#hide player/1.

field(0..2,0..2).
#hide field/2.

f(X,Y,b,0) :- field(X,Y).
#hide f/4.

turn(x,0).
#hide turn/2.

#cumulative t.

% Whose turn is it?
turn(o,t) :- turn(x,t-1).
turn(x,t) :- turn(o,t-1).

% Each player marks once if game is not over
1 { mark(X,Y,P,t-1) : field(X,Y) } 1 :- turn(P,t-1), not win(P2,t-1) : player(P2).

#external ext_mark(X,Y,P,t-1) : field(X,Y) : player(P).

mark(X,Y,P,t-1) :- ext_mark(X,Y,P,t-1), field(X,Y), player(P).

% field markings are inertial
f(X,Y,b,t) :- f(X,Y,b,t-1), field(X,Y), not mark(X,Y,P,t-1) : player(P).
f(X,Y,P,t) :- f(X,Y,P,t-1), field(X,Y), player(P).

% mark field after being marked
f(X,Y,P,t) :- mark(X,Y,P,t-1), field(X,Y), player(P).

% Winning conditions
win(P,t) :- f(X,0,P,t), f(X,1,P,t), f(X,2,P,t), field(X,Y), player(P).
win(P,t) :- f(0,Y,P,t), f(1,Y,P,t), f(2,Y,P,t), field(X,Y), player(P).
win(P,t) :- f(0,0,P,t), f(1,1,P,t), f(2,2,P,t), field(X,Y), player(P).
win(P,t) :- f(2,0,P,t), f(1,1,P,t), f(0,2,P,t), field(X,Y), player(P).

% Only one player can win
:- win(P1,t), win(P2,t), player(P1;P2), P1!=P2.

% Don't override old markings
:- mark(X,Y,P,t-1), not f(X,Y,b,t-1), player(P).

step(t).

#volatile t.

:- not win(x,t).
