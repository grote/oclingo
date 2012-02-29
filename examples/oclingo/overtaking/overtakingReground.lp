% This example provides a (fixed-size) sliding window implementation over aging
% logic programs, where recent external events are evaluated in own (volatile)
% programs.

#const window=6. % external events in window for 6 steps (by default)

%%%%%%%%
% BASE %
%%%%%%%%

#base.
position(behind;nextto;infront). % relative positions
car(red;blue;green).             % some cars


%%%%%%%%%%%%
% VOLATILE %
%%%%%%%%%%%%

#volatile t.

% expect observed relative positions at time point matching step number
#external at(P,C,t) : position(P) : car(C).

% revoke observations at time points below step number
% FIXME next line may trigger (yet) unresolved bug ... uncomment if needed
% #forget t : t-1.

% time points in current window
time(t,(t-window)+1..t).

ta(t,C,T) :- at(_,C,T), time(t,T). % projection of observations

% completion of overtaking maneuver indicated by state "infront" at current time
state(t,behind, C,T) :- at(behind, C,T),  time(t,T).
state(t,nextto, C,T) :- at(nextto, C,T),  time(t,T),
              1 { state(t,behind,C,T-1), state(t,nextto,C,T-1) }.
state(t,infront,C,t) :- at(infront,C,t),  state(t,nextto,C,t-1).
state(t,P,      C,T) :- state(t,P,C,T-1), time(t,T), not ta(t,C,T).


%%%%%%%%%%%%%%%%%%%%%
% OUTPUT PROJECTION %
%%%%%%%%%%%%%%%%%%%%%

#hide.
#show at/3.    % display observed positions in window
#show state/4. % and progress of overtaking maneuvers
