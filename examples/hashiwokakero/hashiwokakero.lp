#hide vertex(X, Y, Z).
#hide between(X, Y, Z, T).
#hide adjacent(X, Y, Z, T).
#hide bridgeNum(X).
#hide innerOutX(X, Y).
#hide innerInX(X, Y).
#hide innerOutY(X, Y).
#hide innerInY(X, Y).
#hide xpos(X).
#hide ypos(X).
#hide bridgesInY(X, Y, N).
#hide bridgesInX(X, Y, N).
#hide bridgesOutY(X, Y, N).
#hide bridgesOutX(X, Y, N).
#hide reached(X, Y).
#hide start(X, Y).

xpos(X) :- vertex(X, Y, N).
ypos(Y) :- vertex(X, Y, N).

between(X, Y1, X, Y3) :- vertex(X, Y1, N1), vertex(X, Y2, N2), vertex(X, Y3, N3), Y1 < Y2, Y2 < Y3.
adjacent(X, Y1, X, Y2) :- vertex(X, Y1, N1), vertex(X, Y2, N2), Y1 < Y2, not between(X, Y1, X, Y2).

between(X1, Y, X3, Y) :- vertex(X1, Y, N1), vertex(X2, Y, N2), vertex(X3, Y, N3), X1 < X2, X2 < X3.
adjacent(X1, Y, X2, Y) :- vertex(X1, Y, N1), vertex(X2, Y, N2), X1 < X2, not between(X1, Y, X2, Y).

innerOutX(X, Y1) :- adjacent(X, Y1, X, Y2).
innerOutY(X1, Y) :- adjacent(X1, Y, X2, Y).
innerInX(X, Y2) :- adjacent(X, Y1, X, Y2).
innerInY(X2, Y) :- adjacent(X1, Y, X2, Y).

bridgeNum(0..2).

1 { bridge(X1, Y1, X2, Y2, B) : bridgeNum(B) } 1 :- adjacent(X1, Y1, X2, Y2).

bridgesOutX(X, Y1, N) :- adjacent(X, Y1, X, Y2), bridgeNum(N), bridge(X, Y1, X, Y2, N).
bridgesOutX(X, Y, 0) :- vertex(X, Y, N), not innerOutX(X, Y).

bridgesOutY(X1, Y, N) :- adjacent(X1, Y, X2, Y), bridgeNum(N), bridge(X1, Y, X2, Y, N).
bridgesOutY(X, Y, 0) :- vertex(X, Y, N), not innerOutY(X, Y).

bridgesInX(X, Y2, N) :- adjacent(X, Y1, X, Y2), bridgeNum(N), bridge(X, Y1, X, Y2, N).
bridgesInX(X, Y, 0) :- vertex(X, Y, N), not innerInX(X, Y).

bridgesInY(X2, Y, N) :- adjacent(X1, Y, X2, Y), bridgeNum(N), bridge(X1, Y, X2, Y, N).
bridgesInY(X, Y, 0) :- vertex(X, Y, N), not innerInY(X, Y).

 :- xpos(X), ypos(Y), bridgeNum(N1), bridgeNum(N2), bridgeNum(N3), bridgeNum(N4), bridgesOutX(X, Y, N1), bridgesOutY(X, Y, N2), bridgesInX(X, Y, N3), bridgesInY(X, Y, N4), vertex(X, Y, N), N != N1 + N2 + N3 + N4.
 :- adjacent(X, Y1, X, Y2), adjacent(X1, Y, X2, Y), 
    bridgeNum(N1), bridgeNum(N2), 
    bridge(X, Y1, X, Y2, N1), bridge(X1, Y, X2, Y, N2), N1 > 0, N2 > 0, Y1 < Y, Y < Y2, X1 < X, X < X2.

reached(X, Y) :- start(X, Y).
reached(X2, Y2) :- reached(X1, Y1), bridge(X1, Y1, X2, Y2, N), N > 0, bridgeNum(N), adjacent(X1, Y1, X2, Y2).
reached(X1, Y1) :- reached(X2, Y2), bridge(X1, Y1, X2, Y2, N), N > 0, bridgeNum(N), adjacent(X1, Y1, X2, Y2).

 :- vertex(X, Y, N), not reached(X, Y).

