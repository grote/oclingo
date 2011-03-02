#step 1.
#volatile.
:- not a.
#endstep.

#step 1.
e(1).
#volatile.
:- not b.
#endstep.

#step 6.
#volatile.
:- not a.
#cumulative.
e(6).
#endstep.

#step 9.
e(9).
#endstep.

#stop.
