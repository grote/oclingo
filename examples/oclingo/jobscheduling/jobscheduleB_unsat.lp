%
% run with:     oclingo jobscheduleB.lp
% input stream: controller.py streams/schedule1
%

#const max_duration = 2.
#const max_job_ids = 2.
#const num_of_machines = 2.
#const max_step = 4.
#const modulo = 2*max_step+1.     % reserve max_step amount of steps for future and past

#iinit 2-modulo.

#base.

jobids(1..max_job_ids).
machine(1..num_of_machines).
duration(1..max_duration).

basetime(0..modulo-1).
succ(T, (T+1) #mod modulo) :- basetime(T).
horizon(T,F) :- basetime(T;F), A:= T..T+max_step, F = A #mod modulo.

{ basejob_arrived(I,M,D,T) : jobids(I) : machine(M) : duration(D) : basetime(T) }.

{ basenow(T) : basetime(T) }.

% SCHEDULING

{ basejob_start(I,T,S) : horizon(T,S) : horizon(T,(S+D) #mod modulo) } 1 :- basejob_arrived(I,M,D,T).

% each machine can only execute one job at a time
occupy(M,I,TA,D,S) :- basejob_start(I,TA,S), basejob_arrived(I,M,D,TA).
occupy(M,I,TA,D-1,TT) :- occupy(M,I,TA,D,T), succ(T,TT), D>1.
occupied(M,I,TA,T) :- occupy(M,I,TA,D,T).
occupied(M,T) :- occupied(M,I,TA,T).
:- occupied(M,I,TA,T), occupied(M,II,TTA,T), (I,TA)<(II,TTA).

scheduled(I,TA) :- basejob_start(I,TA,S).

% no unscheduled jobs
:- basejob_arrived(I,M,D,TA), not scheduled(I,TA).

% Don't schedule in a way that there is an unscheduled job although it is possible to execute it
%:- basejob_arrived(I,M,D,TA), not scheduled(I,TA), D { not occupied(M,T) : A:=TA..TA+max_step-1 : T:= A #mod modulo }.

% Don't wait for starting the job if you don't need to
%:- basejob_arrived(I,M,D,TA), basejob_start(I,TA,S), not occupied(M,T), succ(T,S), horizon(TA,T).

% unscheduled jobs
%unscheduled(I,TA) :- basejob_arrived(I,M,D,TA), not scheduled(I,TA).

% SYNC STREAM WITH BASE PART

#cumulative t. 

#external schedule_job(I,M,D,t): jobids(I) : machine(M) : duration(D) : t>0.
#external now(t) : t>0.

% forgets considering not arrived job requests up to time point t-max_step
%#forget t : t-max_step-1.

:- schedule_job(I,M,D,t), not basejob_arrived(I,M,D,t #mod modulo), jobids(I), machine(M), duration(D).
:- now(t), not basenow(t #mod modulo).

#volatile t : modulo.

:- basejob_arrived(I,M,D,(t+modulo) #mod modulo), not schedule_job(I,M,D,t).
:- basenow((t+modulo) #mod modulo), not now(t).

% OUTPUT
#hide.
#show schedule_job/4.
#show basejob_arrived/4.
#show basejob_start/3.
#show unscheduled/2.
#show basenow/1.
#show now/1.
