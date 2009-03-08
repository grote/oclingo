% instance
r(1, 1, 3).
r(2, 1, 5).
r(3, 1, 2). r(3, 2, 1). r(3, 3, 2).
r(4, 1, 7).
r(5, 1, 1). r(5, 2, 1).
r(6, 1, 2). r(6, 2, 2).
r(7, 1, 5).
r(8, 1, 3).

c(1, 1, 4).
c(2, 1, 3). c(2, 2, 2).
c(3, 1, 2). c(3, 2, 1). c(3, 3, 2).
c(4, 1, 4). c(4, 2, 2).
c(5, 1, 2). c(5, 2, 1). c(5, 3, 2).
c(6, 1, 3). c(6, 2, 2).
c(7, 1, 4).

% encoding
w(C) :- c(C, N, B).
h(R) :- r(R, N, B).

% idea: select a start point for every row
1 { sr(R, C, N) : w(C) } 1 :- r(R, N, B).
 :- sr(R, C, N), m(R, C - 1).
 :- sr(R, C, N), r(R, N, B), m(R, C + B).
 :- sr(R, C1, N), sr(R, C2, N + 1), C1 > C2.

% idea: select a start point for every col
1 { sc(R, C, N) : h(R) } 1 :- c(C, N, B).
 :- sc(R, C, N), m(R - 1, C).
 :- sc(R, C, N), c(C, N, B), m(R + B, C).
 :- sc(R1, C, N), sc(R2, C, N + 1), R1 > R2.

% the result
mr(R, SC .. SC + B - 1) :- sr(R, SC, N), r(R, N, B).
mc(SR .. SR + B - 1, C) :- sc(SR, C, N), c(C, N, B).

m(R, C) :- mr(R, C).
m(R, C) :- mc(R, C).

 :- mr(R, C), not mc(R, C).
 :- mc(R, C), not mr(R, C).

#hide.
#show m/2.

