#step 1.
#assert :id(1).
e(1).
:- not a.
#endstep.

#step 2.
#endstep.

#step 3.
e(2).
#endstep.

#step 4.
#retract : id(1).
#endstep.

#step 6.
#assert :id(2).
e(3).
:- not b.
#assert : id(2,1).
e(5).
#endstep.

#step 7.
e(4).
#endstep.

#step 8.
#retract : id(2).
#endstep.

#step 9.
e(8).
#retract : id(2,1).
#endstep.

#stop.
