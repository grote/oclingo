% This example provides a (fixed-size) sliding window implementation over aging
% logic programs, where recent external events are evaluated in own (volatile)
% programs. Take, e.g., streams/stream01-14E.str as (interesting) logic program
% stream, where comments in streams/stream01-14E.str indicate desired outcomes.

%%%%%%%%%%%%%
% CONSTANTS %
%%%%%%%%%%%%%

#const window=3.                 % external events in window for 3 steps (by default)
#const offset=2.                 % time points/terms may deviate up to 2 (by default)
#const denial=3.                 % account closed when 3 accesses denied (by default)

#iinit 1-offset.

%%%%%%%%
% BASE %
%%%%%%%%

#base.

user(bob;alice;claude). % some users
signal(denied;granted). % some signals

{ account(U,closed) : user(U) }.                      % guess whether accounts closed
  account(U,open)  :- user(U), not account(U,closed). % or open otherwise


%%%%%%%%%%%%%%
% CUMULATIVE %
%%%%%%%%%%%%%%

%%% ONLY THERE TO GET INPUTS TREATED RIGHT
#cumulative t.

% expect user accesses at positive time points up to "offset" time beyond step number
#external access(U,S,t+offset) : user(U) : signal(S).

% revoke user accesses before time point t-offset (atoms declared at step t-2*offest)
% FIXME next line may trigger (yet) unresolved bug ... uncomment if needed
% #forget t : t-2*offset-1.


%%%%%%%%%%%%
% VOLATILE %
%%%%%%%%%%%%

#volatile t.

%%% HAD TO BE MOVED TO CUMULATIVE PART
% % expect user accesses at positive time points up to "offset" time beyond step number
% #external access(U,S,t+offset) : user(U) : signal(S).

% time points in current window
time(t,t-(window+offset)+1..t+offset) :- 0 < t.

% count consecutive denied user accesses up to a "denial" threshold
denied(t,U,1,  T)   :- access(U,denied,T), user(U), time(t,T).
denied(t,U,N+1,T+1) :- access(U,denied,T+1), denied(t,U,N,T), N < denial.
%%% ONLY THERE BECAUSE BELOW CONDITIONAL IN CONJUNCTION DOES NOT BEHAVE AS EXPECTED
denied(t,U,denial,T+1) :- denied(t,U,denial,T), time(t,T+1).

% account closed if "denial" many consecutive denied user accesses observed in window
%%% REPLACED BY SIMPLER RULE USABLE IN VIEW OF WORKAROUND NON-FUNCTIONAL CONDITIONAL
% :- denied(t,U,denial,T), not account(U,closed).
%%% ONLY THERE BECAUSE denied(t,U,denial,T) IMPLIES denied(t,U,denial,t+offset) ANYWAY
:- denied(t,U,denial,t+offset), not account(U,closed).

% account open if consecutive denied user accesses observed in window below threshold
%%% HAD TO BE REPLACED BECAUSE OF NON-FUNCTIONAL CONDITIONAL
% :- account(U,closed), not denied(t,U,denial,T) : time(t,T).
%%% ONLY THERE BECAUSE ABOVE INTEGRITY CONSTRAINT DID NOT BEHAVE AS EXPECTED
:- account(U,closed), not denied(t,U,denial,t+offset).


%%%%%%%%%%%%%%%%%%%%%
% OUTPUT PROJECTION %
%%%%%%%%%%%%%%%%%%%%%

#hide.
#show access/3.  % display user accesses in window
#show account/2. % and the states of user accounts
% #show denied/4.  % and denied user access counters
