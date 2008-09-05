a(1..10).
{ b(X) : a(X) }.

 :- b(1), not b(2).
 :- b(3), b(4).
 :- b(2), not b(7).
 :- b(3), b(5), b(9).
 :- b(10), not b(3).

maximize [ b(X) : a(X) = X ].
maximize [ b(X) : a(X) = 10 - X ].

