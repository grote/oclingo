#base.
#const window_size=3.

#external window(1..window_size).

#cumulative t.

#external window(t + window_size).

atom(t) :- not window(t + window_size).

#volatile t.

