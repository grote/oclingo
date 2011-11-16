%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% LAUNCH: oclingo --iinit="2-modulo" accessBase.lp %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% This example provides a (fixed-size) sliding window implementation over aging
% logic programs, where recent external events are mapped down to a fixed base
% program. Take, e.g., streams/stream01-14E.str as (interesting) logic program
% stream, where comments in streams/stream01-14E.str indicate desired outcomes.

%%%%%%%%%%%%%
% CONSTANTS %
%%%%%%%%%%%%%

#const window=3.                 % external events in window for 3 steps (by default)
#const offset=2.                 % time points/terms may deviate up to 2 (by default)
#const modulo=window+2*offset+1. % number of time points in base program (calculated)
#const denial=3.                 % account closed when 3 accesses denied (by default)


%%%%%%%%
% BASE %
%%%%%%%%

#base.

user(bob;alice;claude). % some users
signal(denied;granted). % some signals

next(T,(T+1) #mod modulo) :- T := 0..modulo-1. % successors in fixed-size time circle

{ baseaccess(U,S,T) : user(U) : signal(S) : next(T,_) }. % guess base window contents
:- baseaccess(U,S,T), S != denied.                       % use "denied" accesses only

% count consecutive denied user accesses along time circle up to a "denial" threshold
denied(U,1,  T) :- baseaccess(U,denied,T).
denied(U,N+1,T) :- baseaccess(U,denied,T), denied(U,N,S), next(S,T), N < denial.

account(U,closed) :- denied(U,denial,T).             % account closed if at threshold
account(U,open)   :- user(U), not account(U,closed). % or open otherwise


%%%%%%%%%%%%%%
% CUMULATIVE %
%%%%%%%%%%%%%%

#cumulative t.

% expect user accesses at positive time points up to "offset" time beyond step number
#external access(U,S,t+offset) : user(U) : signal(S) : 0 < t+offset.

% denied user access implies baseaccess at corresponding time circle position to hold
:- user(U), access(U,denied,T), not baseaccess(U,denied,T #mod modulo), T := t+offset.


%%%%%%%%%%%%
% VOLATILE %
%%%%%%%%%%%%

#volatile t : modulo. % life span limited to time circle size to reflect window slide

% baseaccess at time circle position needs corresponding denied user access in window
:- baseaccess(U,denied,(T+modulo) #mod modulo), not access(U,denied,T), T := t+offset.
% "modulo" added above to get non-negative number when #mod applied w.r.t. past event


%%%%%%%%%%%%%%%%%%%%%
% OUTPUT PROJECTION %
%%%%%%%%%%%%%%%%%%%%%

#hide.
#show access/3.  % display user accesses in window
#show account/2. % and the states of user accounts
#show baseaccess/3. % derived via baseaccess atoms
