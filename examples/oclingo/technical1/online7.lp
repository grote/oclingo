#step 1 : 2.
#volatile : 3.
:- a.
:- b.
#endstep.

#step 2 :0.
e(1).
:- not a.
#endstep.

#step 3 : 1 .
e(2).
#endstep.

#step 5 :  0 .
:- not e(4).
#endstep.

#step 5   : 0.
e(3).
#endstep.

#step 5 :  0.
e(4).
#endstep.

#step 5 : 1.
#volatile : 4.
:- e(3).
#endstep.

#step 5.
e(5).
#endstep.

#stop.
