%
% robot malfunctions at the end and moves in wrong direction
% causing plan to become longer than needed
%

#step 2.
goal(3,3).
#endstep.

#step 3.
robot_moved(right,2).
#endstep.

#step 4.
robot_moved(right, 3).
#endstep.

#step 5.
robot_moved(down, 4).
#endstep.

#step 6.
robot_moved(left, 5).
#endstep.

#stop.
