#hide.
#show queen/2.

num(1..10).

#sum [ queen(X,Y) : num(X;Y) ].

d1(X,Y,X-Y) :- num(X;Y).
d1(D) :- d1(X,Y,D).
 :- 2 #sum [ queen(X,Y) : d1(X,Y,D) ], d1(D).

d2(X,Y,X+Y) :- num(X;Y).
d2(D) :- d2(X,Y,D).
 :- 2 #sum [ queen(X,Y) : d2(X,Y,D) ], d2(D).

% TODO: Y should not be unsafe
 :- not 1 #sum [ queen(X,Y) : num(Y) ] 1, num(X).
 :- not 1 #sum [ queen(X,Y) : num(X) ] 1, num(Y).

