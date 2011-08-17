#step 1.
#volatile : 1.
e(1).
#endstep.

#step 2.
#volatile : 3.
e(2).
#endstep.

#step 3.
#volatile :1.
:- not a.
e(3).
#endstep.

#step 4.
#volatile :2.
e(4).
#endstep.

#step 5.
#volatile :3.
e(5).
#endstep.

#step 6.
#endstep.

#step 7.
#volatile :2.
:- not b.
#cumulative.
e(7).
#endstep.

#step 9.
e(9).
#endstep.

#step 8.
#volatile.
e(8).
:- not a.
#endstep.

#step 9.
#volatile.
:- not b.
#endstep.

#step 9.
#endstep.

#stop.
