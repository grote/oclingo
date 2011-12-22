% This example provides a (fixed-size) sliding window implementation over aging
% logic programs, where recent external events are mapped down to a fixed base
% program. As a particular feature, the encoding exploits dynamic linearization
% of a time cycle in order to enable the faithful application of frame axioms.

#const modulo=6. % number of time slots provided by base program (6 by default)

#iinit 2-modulo.

%%%%%%%%
% BASE %
%%%%%%%%

#base.
position(behind;nextto;infront). % relative positions
car(red;blue;green).             % some cars
time(0..modulo-1).               % time slots

{ now(T) : time(T) } 1. % guess a current time slot

next(T,(T+1) #mod modulo) :- time(T), not now(T). % successors in time circle

{ baseat(P,C,T) : position(P) : car(C) : time(T) }. % guess base window contents
baseat(C,T) :- baseat(_,C,T).                       % projection of observations

% completion of overtaking maneuver indicated by state "infront" at current slot
state(behind, C,T) :- baseat(behind,C,T).
state(nextto, C,S) :- baseat(nextto,C,S), next(T,S),
                   1 { state(behind,C,T), state(nextto,C,T) }.
state(infront,C,S) :- baseat(infront,C,S), now(S),
                      next(T,S), state(nextto,C,T).
state(P,      C,S) :- state(P,C,T), P != infront,
                      next(T,S), not baseat(C,S).


%%%%%%%%%%%%%%
% CUMULATIVE %
%%%%%%%%%%%%%%

#cumulative t.

% expect observed relative positions at time point matching step number
#external at(P,C,t) : position(P) : car(C).

% revoke observations at time points below step number
#forget t : t-1.

% observed position implies baseat at corresponding time circle position to hold
:- at(P,C,t), not baseat(P,C,t #mod modulo).


%%%%%%%%%%%%
% VOLATILE %
%%%%%%%%%%%%

#volatile t : modulo. % life span limited to time circle size to reflect window slide

% baseat at time circle position needs corresponding observed position in window
:- baseat(P,C,(t+modulo) #mod modulo), not at(P,C,t).

#volatile t. % life span 1 of integrity constraint forcing atom for current time slot

:- not now(t #mod modulo).


%%%%%%%%%%%%%%%%%%%%%
% OUTPUT PROJECTION %
%%%%%%%%%%%%%%%%%%%%%

#hide.
#show at/3.    % display observed positions in window
#show state/3. % and progress of overtaking maneuvers
% #show state(infront,_,_). % currently oclingo complains about anonymous variables
% #show state(infront,C,S). % and completed overtakings % this gives internal error
