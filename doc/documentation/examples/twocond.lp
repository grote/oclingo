weekend(sat).  weekend(sun).
on_week :- day(X) : day(X) : not weekend(X).
