#hide.
#show q(X,Y).

% N-queens problem by Ilkka Niemelä 
%
% q(X,Y) gives the legal positions of the queens
% d(1..queens): facts for the dimension of the board. 

q(X,Y) :- d(X), d(Y), not negq(X,Y).
negq(X,Y) :- d(X), d(Y), not q(X,Y).

:- d(X), d(Y), d(X1), q(X,Y), q(X1,Y),  X1 != X.
:- d(X), d(Y), d(Y1), q(X,Y), q(X,Y1), Y1 != Y.
:- d(X), d(Y), d(X1), d(Y1), q(X,Y), q(X1,Y1), X != X1,  Y != Y1, abs(X - X1) == abs(Y - Y1).
:- d(X), not hasq(X). 
hasq(X) :-  d(X), d(Y), q(X,Y). 

d(1..10).

% Typical command line 
% lparse -1 -d none -c queens=8 queens.lp  | smodels
