% This example provides a (fixed-size) sliding window implementation over static
% inputs. The encoding fits for standard ASP systems and is for comparison only.

#const window=6. % external events in window for 6 steps (by default)

position(behind;nextto;infront). % relative positions
car(red;blue;green).             % some cars

at(C,T) :- at(_,C,T). % projection of observations

% time points (in window)
step(T)   :- at(_,T).
less(T-1) :- step(T), 1 < T.
less(T-1) :- less(T), 1 < T.
last(T)   :- step(T), not less(T).
time((T-window)+1..T) :- last(T).

% completion of overtaking maneuver indicated by state "infront" at last time
state(behind, C,T) :- at(behind, C,T), time(T).
state(nextto, C,T) :- at(nextto, C,T), time(T),
              1 { state(behind,C,T-1), state(nextto,C,T-1) }.
state(infront,C,T) :- at(infront,C,T), state(nextto,C,T-1), last(T).
state(P,      C,T) :- state(P,C,T-1), time(T), not at(C,T).


%%%%%%%%%%%%%%%%%%%%%
% OUTPUT PROJECTION %
%%%%%%%%%%%%%%%%%%%%%

#hide.
#show at/3.    % display observed positions in window
#show state/3. % and progress of overtaking maneuvers
