#const p=15.
#const h=14.

pidgeon(1..p).
hole(1..h).

% every pidgeon needs a hole
1 { put(P, H) : hole(H) } 1 :- pidgeon(P).
% there may be at most 2 pidgeons in a hole
0 { put(P, H) : pidgeon(P) } 1 :- hole(H).
