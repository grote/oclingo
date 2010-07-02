zig(0).
zag(1).
zigzag(X,Y) :- zig(X), zag(Y).
zigzag(Y,X) :- zigzag(X,Y).
