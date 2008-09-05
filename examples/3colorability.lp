%sample from the documentation
#hide.
#show colored/2.

color(red). color(green). color(blue).

colored(V,C) :- not colored(V,D), not colored(V,E), C != D, C != E, D != E, color(E), color(C), color(D), vertex(V).

:- edge(V,U), colored(V,C), colored(U,C).%, vertex(V), vertex(U), color(C).

vertex(1..4).
edge(1,2). edge(1,4).
edge(3,1). edge(3,2).
