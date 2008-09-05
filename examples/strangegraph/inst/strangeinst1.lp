start(1).

node(1, 0, 1).
node(2, 1, 1).
node(3, 1, 4).
node(4, 1, 0).
node(5, 1, 1).
node(6, 4, 0).
node(7, 1, 1).
node(8, 1, 2).
node(9, 1, 1).
node(10, 2, 1).
node(11, 1, 2).
node(12, 1, 2).

%edge(X, Y) :- node(X, InX, OutX), node(Y, InY, OutY).

edge(1, 2).
edge(2, 3).
edge(3, 4).
edge(3, 5).
edge(3, 6).
edge(3, 8).
edge(5, 6).
edge(7, 6).
edge(8, 6).
edge(8, 9).
edge(9, 11).
edge(10, 7).
edge(11, 10).
edge(11, 12).
edge(12, 10).
edge(12, 11).
