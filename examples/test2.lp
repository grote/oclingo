% hashiwokakero.lp
%
% Copyright Martin Brain 20/10/06, see COPYING for licence
% mjb@cs.bath.ac.uk
% 07/10/06
%
% An encoding to Hashiwokakero

#domain xpos(X).
#domain xpos(X1).
#domain xpos(X2).

#domain ypos(Y).
#domain ypos(Y1).
#domain ypos(Y2).

% Some squares have numbers
number(1 .. 8).

#domain number(N).

% Bridge types
bridge(twoHorizontal).
bridge(twoVerticle).
bridge(oneHorizontal).
bridge(oneVerticle).
bridge(blank).

#domain bridge(B).
#domain bridge(B1).
#domain bridge(B2).

horizontalBridge(oneHorizontal).
horizontalBridge(twoHorizontal).

#domain horizontalBridge(H).

verticleBridge(oneVerticle).
verticleBridge(twoVerticle).

#domain verticleBridge(V).


% Pick some bridges if the square doesn't have a number
 island(X,Y) :- status(X,Y,N).
1 { status(X,Y,D) : bridge(D) } 1 :- not island(X,Y).


% Bridges must go in a straight line and not cut down
 :- status(X,Y,H), status(X+1,Y,B), B != H.
 :- status(X,Y,H), status(X-1,Y,B), B != H.
 :- status(X,Y,V), status(X,Y+1,B), B != V.
 :- status(X,Y,V), status(X,Y-1,B), B != V.

% Must start and finish on islands
 :- status(1,Y,H).
 :- status(W,Y,H), maxsize(W).
 :- status(X,1,V).
 :- status(X,H,V), maxsize(H).


% Number of bridges must match number of node
% Do as four steps

count2(0..8).
#domain count2(C).

incomingCount(X,Y,0,1) :-  status(X-1,Y,B), B != oneHorizontal, B !=
twoHorizontal.
incomingCount(X,Y,0,1) :-  status(X-1,Y,C).
incomingCount(X,Y,0,1) :-  status(X-1,Y,blank).
incomingCount(X,Y,0,1) :-  not xpos(X-1).
incomingCount(X,Y,1,1) :-  status(X-1,Y,oneHorizontal).
incomingCount(X,Y,2,1) :-  status(X-1,Y,twoHorizontal).

incomingCount(X,Y,C,2) :-  status(X,Y-1,B), B != oneVerticle, B != twoVerticle,
incomingCount(X,Y,C,1).
incomingCount(X,Y,C,2) :-  status(X,Y-1,C), incomingCount(X,Y,C,1).
incomingCount(X,Y,C,2) :-  status(X,Y-1,blank), incomingCount(X,Y,C,1).
incomingCount(X,Y,C,2) :-  not ypos(Y-1), incomingCount(X,Y,C,1).
incomingCount(X,Y,C+1,2) :-  status(X,Y-1,oneVerticle), incomingCount(X,Y,C,1),
count2(C+1).
incomingCount(X,Y,C+2,2) :-  status(X,Y-1,twoVerticle), incomingCount(X,Y,C,1),
count2(C+2).

incomingCount(X,Y,C,3) :-  status(X+1,Y,B), B != oneHorizontal, B !=
twoHorizontal, incomingCount(X,Y,C,2).
incomingCount(X,Y,C,3) :-  status(X+1,Y,C), incomingCount(X,Y,C,2).
incomingCount(X,Y,C,3) :-  status(X+1,Y,blank), incomingCount(X,Y,C,2).
incomingCount(X,Y,C,3) :-  not xpos(X+1), incomingCount(X,Y,C,2).
incomingCount(X,Y,C+1,3) :-  status(X+1,Y,oneHorizontal),
incomingCount(X,Y,C,2), count2(C+1).
incomingCount(X,Y,C+2,3) :-  status(X+1,Y,twoHorizontal),
incomingCount(X,Y,C,2), count2(C+2).

incomingCount(X,Y,C,4) :-  status(X,Y+1,B), B != oneVerticle, B != twoVerticle,
incomingCount(X,Y,C,3).
incomingCount(X,Y,C,4) :-  status(X,Y+1,C), incomingCount(X,Y,C,3).
incomingCount(X,Y,C,4) :-  status(X,Y+1,blank), incomingCount(X,Y,C,3).
incomingCount(X,Y,C,4) :-  not ypos(Y+1), incomingCount(X,Y,C,3).
incomingCount(X,Y,C+1,4) :-  status(X,Y+1,oneVerticle), incomingCount(X,Y,C,3),
count2(C+1).
incomingCount(X,Y,C+2,4) :-  status(X,Y+1,twoVerticle), incomingCount(X,Y,C,3),
count2(C+2).

 :- status(X,Y,N), incomingCount(X,Y,C,4), N != C.

% Every numbered point must be able to reach another
canReach(X,Y,X,Y) :- island(X,Y).

canReach(X,Y,X,Y) :- status(X,Y,B), horizontalBridge(B).
canReach(X,Y,X+1,Y) :- status(X,Y,B), horizontalBridge(B), xpos(X+1).
canReach(X,Y,X-1,Y) :- status(X,Y,B), horizontalBridge(B), xpos(X-1).
canReach(X+1,Y,X,Y) :- status(X,Y,B), horizontalBridge(B), xpos(X+1).
canReach(X-1,Y,X,Y) :- status(X,Y,B), horizontalBridge(B), xpos(X-1).

canReach(X,Y,X,Y) :- status(X,Y,B), verticleBridge(B).
canReach(X,Y,X,Y+1) :- status(X,Y,B), verticleBridge(B), ypos(Y+1).
canReach(X,Y,X,Y-1) :- status(X,Y,B), verticleBridge(B), ypos(Y-1).
canReach(X,Y+1,X,Y) :- status(X,Y,B), verticleBridge(B), ypos(Y+1).
canReach(X,Y-1,X,Y) :- status(X,Y,B), verticleBridge(B), ypos(Y-1).

canReach(X1,Y1,X2 + 1,Y2) :- canReach(X1,Y1,X2,Y2), canReach(X2,Y2,X2 + 1,Y2),
xpos(X2 + 1).
canReach(X1,Y1,X2 - 1,Y2) :- canReach(X1,Y1,X2,Y2), canReach(X2,Y2,X2 - 1,Y2),
xpos(X2 - 1).
canReach(X1,Y1,X2,Y2 + 1) :- canReach(X1,Y1,X2,Y2), canReach(X2,Y2,X2,Y2 + 1),
ypos(Y2 + 1).
canReach(X1,Y1,X2,Y2 - 1) :- canReach(X1,Y1,X2,Y2), canReach(X2,Y2,X2,Y2 - 1),
ypos(Y2 - 1).

 :- island(X1,Y1), island(X2,Y2), not canReach(X1,Y1,X2,Y2).

%#hide.
%#show status(X,Y,A).
xpos(1). ypos(1).
xpos(2). ypos(2).
xpos(3). ypos(3).
xpos(4). ypos(4).
xpos(5). ypos(5).
xpos(6). ypos(6).
xpos(7). ypos(7).
xpos(8). ypos(8).
xpos(9). ypos(9).
xpos(10). ypos(10).
xpos(11). ypos(11).
xpos(12). ypos(12).
xpos(13). ypos(13).
xpos(14). ypos(14).
xpos(15). ypos(15).
xpos(16). ypos(16).
xpos(17). ypos(17).
xpos(18). ypos(18).
xpos(19). ypos(19).
xpos(20). ypos(20).
xpos(21). ypos(21).
xpos(22). ypos(22).
xpos(23). ypos(23).
xpos(24). ypos(24).
xpos(25). ypos(25).
xpos(26). ypos(26).
status(22,3,1).
status(15,3,1).
status(12,3,1).
status(24,22,1).
status(7,13,1).
status(10,14,1).
status(24,3,1).
status(15,11,1).
status(25,8,1).
status(18,16,1).
status(18,11,1).
status(12,26,1).
status(21,11,1).
status(15,9,1).
status(3,15,1).
status(18,24,1).
status(14,21,1).
status(20,10,1).
status(17,9,1).
status(9,1,1).
status(14,18,1).
status(22,4,1).
status(6,24,1).
status(19,13,1).
status(3,15,1).
status(24,6,1).
status(8,20,1).
status(20,14,1).
status(16,4,1).
status(20,13,1).
status(13,26,1).
status(17,21,1).
status(20,1,1).
status(7,20,1).
status(3,22,1).
status(14,20,1).
status(23,1,1).
status(1,20,1).
status(15,10,1).
status(11,3,1).
status(7,15,1).
status(22,5,1).
status(13,25,1).
status(3,6,1).
status(5,8,1).
status(21,22,1).
status(17,19,1).
status(7,19,1).
status(11,8,1).
status(18,18,1).
status(11,24,1).
status(4,19,1).
status(18,6,1).
status(6,9,1).
status(13,13,1).
status(15,8,1).
status(10,24,1).
status(6,5,1).
status(21,16,1).
status(20,7,1).
status(21,18,1).
status(11,22,1).
status(6,13,1).
status(25,19,1).
status(24,5,1).
status(4,19,1).
status(6,3,1).
status(26,9,1).
status(13,4,1).
status(7,23,1).
status(19,18,1).
status(2,2,1).
status(5,8,1).
status(13,7,1).
status(4,4,1).
status(7,7,1).
status(17,12,1).
status(24,11,1).
status(11,24,1).
status(13,5,1).
status(21,23,1).
status(21,25,1).
status(17,4,1).
status(26,16,1).
status(1,19,1).
status(6,25,1).
status(4,16,1).
status(5,13,1).
status(14,24,1).
status(26,10,1).
status(9,5,1).
status(9,26,1).
status(11,16,1).
status(20,16,1).
status(10,16,1).
status(2,22,1).
status(1,25,1).
status(11,4,1).
status(6,17,1).
status(24,17,1).
status(21,26,1).
status(15,8,1).
status(7,19,1).
status(3,13,1).
status(14,22,1).
status(19,23,1).
status(22,24,1).
status(14,8,1).
status(19,14,1).
status(21,12,1).
status(7,7,1).
status(23,21,1).
status(13,2,1).
status(17,5,1).
status(12,18,1).
status(21,10,1).
status(26,6,1).
status(4,19,1).
status(11,11,1).
status(9,6,1).
status(23,13,1).
status(10,10,1).
status(5,22,1).
status(18,13,1).
status(18,24,1).
status(13,19,1).
status(6,19,1).
status(21,22,1).
status(19,7,1).
status(3,15,1).
status(25,11,1).
status(1,20,1).
status(24,25,1).
status(22,16,1).
status(24,8,1).
status(18,21,1).
status(23,26,1).
maxsize(26).

