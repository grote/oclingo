%*
{ p }.
minimize{ p }.
*%

#const n=70.
p(2 * (1..n)).
n avg [q(X) : p(X) = X] n.

#maximize [ q(X) : p(X) ].

#hide p/1.

%*
3 avg [ p=1 ] 2.
*%
