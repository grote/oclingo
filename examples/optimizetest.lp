b(1..4).
{ a(X) : b(X) }.
minimize [ a(X) : b(X)].
minimize [ a(1) = 1, not a(2) = 2, a(3) = 3, not a(4) = 4].
minimize [ not a(X) : b(X)].
CoMpUtE { a(1), not a(2) }.
