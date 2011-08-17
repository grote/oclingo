#step 1.
#volatile : 1.
e(1).
#endstep.

#step 2.
#volatile : 2.
:- not a.
e(2).
#endstep.

%#step 2.
%#volatile :2.
%:- not b.
%#endstep.

%#step 2.
%#volatile :2.
%:- not a.
%#endstep.

%#step 6.
%#volatile :3.
%:- not b.
%#cumulative.
%e(6).
%#endstep.

%#step 9.
%e(9).
%#endstep.

#stop.
