%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% LAUNCH: oclingo --iinit="2-retain" accessVolatile.lp %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

% This example provides a (fixed-size) sliding window implementation over aging
% logic programs, where recent external events are evaluated in volatile program
% parts. Take, e.g., streams/stream01-14E.str as (interesting) logic program
% stream, where comments in streams/stream01-14E.str indicate desired outcomes.

%%%%%%%%%%%%%
% CONSTANTS %
%%%%%%%%%%%%%

#const window=3.                 % external events in window for 3 steps (by default)
#const offset=2.                 % time points/terms may deviate up to 2 (by default)
#const retain=window+2*offset.   % number of conjoined volatile programs (calculated)
#const denial=3.                 % account closed when 3 accesses denied (by default)


%%%%%%%%
% BASE %
%%%%%%%%

#base.

user(bob;alice;claude). % some users
signal(denied;granted). % some signals

{ account(U,closed) : user(U) }.                      % guess whether accounts closed
  account(U,open)  :- user(U), not account(U,closed). % or open otherwise


%%%%%%%%%%%%
% VOLATILE %
%%%%%%%%%%%%

#volatile t : retain. % life span limited viewing that time points drop out of window

% expect user accesses at positive time points up to "offset" time beyond step number
#external access(U,S,t+offset) : user(U) : signal(S) : 0 < t+offset.

% denied user access counting works backwards, and next-step counter values are input
#external denied(U,1..denial,t+1) : user(U).

% count consecutive denied user accesses up to a "denial" threshold backwards in time
denied(U,1,     t) :- user(U), access(U,denied,t+offset).
denied(U,N+1,   t) :- user(U), access(U,denied,t+offset),
                      denied(U,N,t+1), N := 1..denial-1.
denied(U,denial,t) :- user(U), denied(U,denial,t+1).

% account closed iff "denial" many consecutive denied user accesses in oldest program
:- denied(U,denial,t), not account(U,closed), user(U).
:- account(U,closed), not denied(U,denial,T) : T := (t-retain)+1..t.


%%%%%%%%%%%%%%%%%%%%%
% OUTPUT PROJECTION %
%%%%%%%%%%%%%%%%%%%%%

#hide.
#show access/3.  % display user accesses in window
#show account/2. % and the states of user accounts
% #show denied/3.  % and denied user access counters
