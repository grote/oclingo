% Test for Volatile Query Bug (http://sourceforge.net/p/potassco/bugs/73/)

#base.
{p(1),p(2)}.

#volatile t.
:- p((t #mod 2)+1).

