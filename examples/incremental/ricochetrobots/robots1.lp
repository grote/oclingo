% Ricochet Robots (Rasende Roboter) by Alex Randolph
% ---
% http://en.wikipedia.org/wiki/Ricochet_Robot
% http://www.ricochetrobots.com
%
% ASP encoding by torsten Schaub
%
% --- remarks
%   - (1,1) is north-west corner
%   - boards are specified in terms of
%     - robot positions at time 0:       position/4
%     - barriers relative to directions: barrier/3
%   - a goal is a single robot position,
%      eg. :- not position(red,6,7,42),
%     given in the respective board description files,
%      eg. board16-1.

#base.

dim(1..dimension).

robot(red).
robot(blue).
robot(green).
robot(yellow).

direction(north).
direction(east).
direction(south).
direction(west).

% --- blockages
%
blocked(I,J,D) :- barrier(I,J,D),direction(D),dim(I;J).

blocked(1..dimension,1           ,north).
blocked(   dimension,1..dimension,east ).
blocked(1..dimension,   dimension,south).
blocked(           1,1..dimension,west ).

blocked(I+1,J,west ) :- blocked(I,J,east ),dim(I;J),I<dimension.
blocked(I,J+1,north) :- blocked(I,J,south),dim(I;J),J<dimension.
blocked(I-1,J,east ) :- blocked(I,J,west ),dim(I;J),I>1.
blocked(I,J-1,south) :- blocked(I,J,north),dim(I;J),J>1.

#cumulative t.

% --- effects of actions
%
1 {position(R,I+K,J,t):dim(K):K>0:dimension>=I+K} 1 :- go(R,east,t - 1),position(R,I,J,t - 1), dim(I;J).
1 {position(R,I-K,J,t):dim(K):K>0:        1<=I-K} 1 :- go(R,west,t - 1),position(R,I,J,t - 1), dim(I;J).
1 {position(R,I,J+K,t):dim(K):K>0:dimension>=J+K} 1 :- go(R,south,t - 1),position(R,I,J,t - 1), dim(I;J).
1 {position(R,I,J-K,t):dim(K):K>0:        1<=J-K} 1 :- go(R,north,t - 1),position(R,I,J,t - 1), dim(I;J).

% --- no crossing of blockages or robots
%
:- go(R,east,t - 1),position(R,I,J,t - 1),position(R,K,J,t),I<K,
   1 { blocked(L,J,east ):dim(L):I<=L:L<K, position(P,L,J,t - 1):dim(L):I<L:L<=K:robot(P):P!=R }.
:- go(R,west,t - 1),position(R,I,J,t - 1),position(R,K,J,t),K<I,
   1 { blocked(L,J,west ):dim(L):K<L:L<=I, position(P,L,J,t - 1):dim(L):K<=L:L<I:robot(P):P!=R }.
:- go(R,south,t - 1),position(R,I,J,t - 1),position(R,I,K,t),J<K,
   1 { blocked(I,L,south):dim(L):J<=L:L<K, position(P,I,L,t - 1):dim(L):J<L:L<=K:robot(P):P!=R }.
:- go(R,north,t - 1),position(R,I,J,t - 1),position(R,I,K,t),K<J,
   1 { blocked(I,L,north):dim(L):K<L:L<=J, position(P,I,L,t - 1):dim(L):K<=L:L<J:robot(P):P!=R }.

% --- no stopping if there is neither a blockage nor a robot
%
:- go(R,east,t - 1),position(R,I,J,t - 1),position(R,K,J,t),I<K,
   { blocked(K,J,east ), position(P,K+1,J,t - 1):robot(P):P!=R } 0.
:- go(R,west,t - 1),position(R,I,J,t - 1),position(R,K,J,t),K<I,
   { blocked(K,J,west ), position(P,K-1,J,t):robot(P):P!=R } 0.
:- go(R,south,t - 1),position(R,I,J,t - 1),position(R,I,K,t),J<K,
   { blocked(I,K,south), position(P,I,K+1,t):robot(P):P!=R } 0.
:- go(R,north,t - 1),position(R,I,J,t - 1),position(R,I,K,t),K<J,
   { blocked(I,K,north), position(P,I,K-1,t - 1):robot(P):P!=R } 0.

% --- inertia
position(R,I,J,t) :- position(R,I,J,t - 1), {go(R,D,t - 1):direction(D)} 0, robot(R), dim(I;J).

% --- plan generator
%
1 {go(R,D,t - 1):robot(R):direction(D)} 1.

#hide time/1,robot/1,dim/1,direction/1.
#hide position/4.
#hide blocked/3, barrier/3.
%#hide go/3.
