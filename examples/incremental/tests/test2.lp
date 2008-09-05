#base.

p(0) :- not -p(0).
-p(0) :- not p(0).
:- p(0), -p(0).

#cumulative k.

a(k) :- not -a(k).
-a(k) :- not a(k).
p(k) :- a(k).
p(k) :- p(k-1), not -p(k).
-p(k) :- -p(k-1), not p(k).
:- p(k), -p(k).
:- a(k), -a(k).

#volatile k.

:- not -p(0).
:- not p(k).
:- not -a(k).
