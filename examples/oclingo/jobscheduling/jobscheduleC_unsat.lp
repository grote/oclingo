%
% run with:     oclingo jobscheduleC.lp
% input stream: controller.py online1
%

#const max_duration = 2.
#const max_job_ids = 2.
#const num_of_machines = 2.
#const max_step = 4.

#iinit 1-max_step.

#base.

jobids(1..max_job_ids).
machine(1..num_of_machines).
duration(1..max_duration).

#cumulative t.

%#external schedule_job(I,M,D,t+max_step): jobids(I) : machine(M) : duration(D).
%#external now(t+max_step) : (t+max_step)>0.
#external schedule_job/4.
#external now/1.


{ job_start(I,T,S): S:=T..T+max_step : T<=S+D : S+D<=T+max_step } 1 :- schedule_job(I,M,D,T), T:=t+max_step,
    jobids(I), machine(M), duration(D).

% each machine can only execute one job at a time
occupy(M,I,T,D,t+max_step) :- job_start(I,T,t+max_step), schedule_job(I,M,D,T),
                              jobids(I), machine(M), duration(D).
occupy(M,I,T,D-1,t+max_step) :- occupy(M,I,T,D,t+max_step-1), D>1.                                                                                                    
occupied(M,I,T,t+max_step) :- occupy(M,I,T,D,t+max_step).
occupied(M,t+max_step) :- occupied(M,I,T,t+max_step).
:- occupied(M,I,T,t+max_step), occupied(M,II,TT,t+max_step), (I,T)<(II,TT).

scheduled(I,t) :- job_start(I,t,_), t>0.

% no unscheduled jobs
:- schedule_job(I,M,D,t), jobids(I), machine(M), duration(D), not scheduled(I,t), t>0.

% OUTPUT
#hide.
#show job_start/3.
#show schedule_job/4.
#show now/1.
