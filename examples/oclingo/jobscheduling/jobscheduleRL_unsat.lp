%
% run with:             clingo jobscheduleRL_filtering.lp streaminput
% for relaunching run:  relaunchclingo.py -s streamfile -e jobscheduleRL_filtering.lp -t {max_step}
%

%#const max_duration = 2.
%#const max_job_ids = 2.
%#const num_of_machines = 2.
#const max_step = 4.

%jobids(1..max_job_ids).
%machine(1..num_of_machines).
%duration(1..max_duration).

inwindow(T-max_step+1..T) :- currenttime(T).

{ job_start(I,T,S): S:=T..T+max_step : S+D<=T+max_step } 1 :- schedule_job(I,M,D,T), inwindow(T).

% each machine can only execute one job at a time
occupy(M,I,T,D,S) :- job_start(I,T,S), schedule_job(I,M,D,T).
occupy(M,I,T,D-1,S+1) :- occupy(M,I,T,D,S), D>1.                                                                                                    
occupied(M,I,T,S) :- occupy(M,I,T,D,S).
occupied(M,S) :- occupied(M,I,T,S).
:- occupied(M,I,T,S), occupied(M,II,TT,S), (I,T)<(II,TT).

scheduled(I,TA) :- job_start(I,TA,_).

% no unscheduled jobs
:- schedule_job(I,M,D,TA), inwindow(TA), not scheduled(I,TA).


% OUTPUT
#hide.
#show job_start/3.
#show schedule_job/4.
#show currenttime/1.
