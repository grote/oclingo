#step 0.
ext_mark(2,1,x,0).
#endstep.

#step 1.
ext_mark(1,1,o,1).
#endstep.

#step 2.
ext_mark(2,2,x,2).
#endstep.

#step 3.
ext_mark(2,0,o,3).
#endstep.

#step 4.
ext_mark(0,2,x,4).
#endstep.

#step 5.
% player x can not win after this move
ext_mark(1,2,o,5).
#endstep.

#stop.
