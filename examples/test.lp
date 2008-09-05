a.
b(3).
c(1..5).
b(X) :- c(X), a, not d(X).
d(X) :- c(X), not b(X).
num(1..2).
x :- L { d(5), d(1) } U, num(L), num(U).

x :- Z { d(5), d(1) : num(X) } Y, num(Y), num(Z).

%test :- d

num(1..3).
test(1..5, X) :- num(X).

% a :- b(1..10).
% is equivalent to
% a :- B(_NEWVAR), 1 <= _NEWVAR, _NEWVAR <= 10.

1 { p(1..10) } 2 :- body.
% is equivalent to
% _p(1..10) :- body.
% 1 { p(X) : _p(X) } 2 :- body.

