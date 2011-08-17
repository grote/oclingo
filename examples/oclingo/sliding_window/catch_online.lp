%#step 1.
%window(1).
%#endstep.

#step 2.
window(2).
commit_move(none, 1).
ball(1, 2).
#endstep.

#step 3.
window(3).
commit_move(none, 2).
ball(2, 3).
#endstep.

#step 4.
window(4).
commit_move(left, 3).
#endstep.

#step 5.
window(5).
commit_move(left, 4).
#endstep.

#step 6.
window(6).
commit_move(right, 5).
ball(5, 6).
#endstep.

#stop.
