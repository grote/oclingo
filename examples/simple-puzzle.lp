%  /\  /\  /\
% /1 \/2 \/3 \
% \  /\  /\  /
%  \/5 \/4 \/
%  /\  /\  /\
% /6 \/7 \/8 \
% \  /\  /\  /
%  \/  \/  \/
%
%   /\
%  /C1\
% /C  C\
% \4  2/
%  \C3/
%   \/

hide places(X).
hide stones(X).
hide colors(X).
hide stone(N, C1, C2, C3, C4).
hide adj1(P1, P2).
hide adj2(P1, P2).
hide adjacent1(P1, P2).
hide adjacent2(P1, P2).

places(1..8).
stones(1..4).
colors(r;g;b;y).

stone(1,g,b,y,r).
stone(2,g,r,y,b).
stone(3,g,y,r,b).
stone(4,g,r,b,y).

% to evaluate this rule basic program evaluation is needed
stone(N, C1, C2, C3, C4) :- stone(N, C2, C3, C4, C1), colors(C1), colors(C2), colors(C3), colors(C4), stones(N).

% each stone can be rotated (without basic program evaluation)
% stone(N, C1, C2, C3, C4) :- stoneI(N, C1, C2, C3, C4).
% stone(N, C1, C2, C3, C4) :- stoneI(N, C2, C3, C4, C1).
% stone(N, C1, C2, C3, C4) :- stoneI(N, C3, C4, C1, C2).
% stone(N, C1, C2, C3, C4) :- stoneI(N, C4, C1, C2, C3).

% choose one stone for every place
1 { place(P, N, C1, C2, C3, C4) : stone(N, C1, C2, C3, C4) } 1 :- places(P).

% choose every stone 2 times
2 { place(P, N, C1, C2, C3, C4) : places(P) : stone(N, C1, C2, C3, C4) } 2 :- stones(N).

% top-left and bottom-right stone fit together
adj1(1, 5).
adj1(2, 4).
adj1(5, 7).
adj1(4, 8).
adjacent1(P1, P2) :- adj1(P1, P2), stone(N1, C1, C2, C3, C4), stone(N2, C2, C5, C6, C3),
                     place(P1, N1, C1, C2, C3, C4), place(P2, N2, C2, C5, C6, C3).
 :- adj1(P1, P2), not adjacent1(P1, P2).

% top-right and bottom-left stone fit together
adj2(2, 5).
adj2(3, 4).
adj2(5, 6).
adj2(4, 7).
adjacent2(P1, P2) :- adj2(P1, P2), stone(N1, C1, C2, C3, C4), stone(N2, C4, C3, C5, C6),
                     place(P1, N1, C1, C2, C3, C4), place(P2, N2, C4, C3, C5, C6).
 :- adj2(P1, P2), not adjacent2(P1, P2).

