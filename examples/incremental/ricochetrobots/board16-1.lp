% Authentic board from Ricochet Robots (Rasende Roboter) by Alex Randolph

#const dimension=16.

#base.

instance(inst).

% --- Unlike the real game we fix the robots initial status to outer corners
%
position(red,1,1,0).
position(blue,1,dimension,0).
position(green,dimension,1,0).
position(yellow,dimension,dimension,0).

#volatile t.

% --- Uncomment an integrity constraint to pose a query
%   - numbers indicated minimum number of moves  
%   - sometimes a heuristics was better than another

 :- instance(1), not position(red,    5, 2,t).  % red moon:        9, vmtf 
 :- instance(2), not position(red,   15, 2,t).  % red triangle:   10, ?
 :- instance(3), not position(green,  2, 3,t).  % green triangle:  9, berkmin
 :- instance(4), not position(blue,  12, 3,t).  % blue star:       8, ?
 :- instance(5), not position(yellow, 7, 4,t).  % yellow star:    11, ?
 :- instance(6), not position(blue,   4, 7,t).  % blue saturn:     8, ?
 :- instance(7), not position(green, 14, 7, t).  % green moon:      3, ?
 :- instance(8), not position(yellow,11, 8,t).  % yellow saturn:  13, vmtf??
 :- instance(9), not position(yellow, 5,10,t).  % yellow moon:     2, ?
 :- instance(10), not position(green,  2,11,t).  % green star:     11, ?
 :- instance(11), not position(red,   14,11,t).  % red star:        9, ?
 :- instance(12), not position(green, 11,12,t).  % green saturn:   11, ?
 :- instance(13), not position(yellow,15,13,t).  % yellow star:     9, ?
 :- instance(14), not position(blue,   7,14,t).  % blue star:       6, ?
 :- instance(15), not position(red,    3,15,t).  % red saturn:      5, ?
 :- instance(16), not position(blue,  10,15,t).  % blue moon:      12, vsids?

#base.

% --- on board barriers 
%
barrier( 2, 1,east).
barrier(10, 1,east).
barrier( 4, 2,east).
barrier(14, 2,east).
barrier( 2, 3,east).
barrier( 11,3,east).
barrier( 7, 4,east).
barrier( 3, 7,east).
barrier( 14,7,east).
barrier( 7, 8,east). % middle
barrier( 10,8,west). % middle
barrier( 11,8,east).
barrier( 7, 9,east). % middle
barrier( 10,9,west). % middle
barrier( 4,10,east).
barrier( 2,11,east).
barrier( 8,11,east).
barrier(13,11,east).
barrier(11,12,east).
barrier(14,13,east).
barrier( 6,14,east).
barrier(3, 15,east).
barrier(10,15,east).
barrier(4, 16,east).
barrier(12,16,east).

barrier( 5, 1,south).
barrier(15, 1,south).
barrier( 2, 2,south).
barrier(12, 3,south).
barrier( 7, 4,south).
barrier(16, 4,south).
barrier( 1, 6,south).
barrier( 4, 7,south).
barrier( 8, 7,south). % middle
barrier( 9, 7,south). % middle
barrier(11, 7,south).
barrier(14, 7,south).
barrier(16, 9,south).
barrier( 2,10,south).
barrier( 5,10,south).
barrier( 8,10,north). % middle
barrier( 9,10,north). % middle
barrier( 9,10,south).
barrier(14,10,south).
barrier( 1,12,south).
barrier(11,12,south).
barrier( 7,13,south).
barrier(15,13,south).
barrier(10,14,south).
barrier( 3,15,south).
