% This example provides a (fixed-size) sliding window implementation over aging
% logic programs, where recent external events are evaluated in the cumulative
% program.

%%%%%%%%
% BASE %
%%%%%%%%

#base.
position(behind;nextto;infront). % relative positions
car(red;blue;green).             % some cars


%%%%%%%%%%%%%%
% CUMULATIVE %
%%%%%%%%%%%%%%

#cumulative t.

% expect observed relative positions at time point matching step number
#external at(P,C,t) : position(P) : car(C).

% revoke observations at time points below step number
% FIXME next line may trigger (yet) unresolved bug ... uncomment if needed
% #forget t : t-1.

{ now(t) }.  % guess current time point
:- now(t-1). % drop previous time point

at(C,t) :- at(_,C,t). % projection of observations

% completion of overtaking maneuver indicated by state "infront" at current time
state(behind, C,t) :- at(behind, C,t).
state(nextto, C,t) :- at(nextto, C,t),
              1 { state(behind,C,t-1), state(nextto,C,t-1) }.
state(infront,C,t) :- at(infront,C,t), state(nextto,C,t-1), now(t).
state(P,      C,t) :- state(P,C,t-1), P != infront, not at(C,t).


%%%%%%%%%%%%
% VOLATILE %
%%%%%%%%%%%%

#volatile t.

:- not now(t). % fix current time point


%%%%%%%%%%%%%%%%%%%%%
% OUTPUT PROJECTION %
%%%%%%%%%%%%%%%%%%%%%

#hide.
#show at/3.    % display observed positions in window
#show state/3. % and progress of overtaking maneuvers
