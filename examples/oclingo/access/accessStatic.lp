% This example provides a (fixed-size) sliding window implementation over static
% inputs. The encoding fits for standard ASP systems and is for comparison only.
% ATTENTION: Semantics of transient stream data cannot be reflected one-to-one
%            because offset blurs correspondence between step numbers and terms.

#const window=3. % external events in window for 3 steps (by default)
#const offset=2. % time points/terms may deviate up to 2 (by default)
#const denial=3. % account closed when 3 accesses denied (by default)

onedenial :- denial == 1.
muldenial :- denial  > 1.

user(bob;alice;claude). % some users
signal(denied;granted). % some signals

% time points (in window ... actually an overestimation)
step(T)   :- access(_,_,T).
less(T-1) :- step(T), 1 < T.
less(T-1) :- less(T), 1 < T.
last(T)   :- step(T), not less(T).
time(T-window-2*offset+1..T) :- last(T).

% count consecutive denied user accesses up to a "denial" threshold
denied(U,1,  T)   :- access(U,denied,T), user(U), time(T), muldenial.
denied(U,N+1,T+1) :- access(U,denied,T+1), denied(U,N,T), N < denial-1.
account(U,closed) :- access(U,denied,T+1), denied(U,denial-1,T).
account(U,closed) :- access(U,denied,_), user(U), time(T), onedenial.
account(U,open)   :- user(U), not account(U,closed).


%%%%%%%%%%%%%%%%%%%%%
% OUTPUT PROJECTION %
%%%%%%%%%%%%%%%%%%%%%

#hide.
#show access/3.  % display user accesses in window
#show account/2. % and the states of user accounts
% #show denied/3.  % and denied user access counters
