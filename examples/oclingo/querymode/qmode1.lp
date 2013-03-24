% Technical example for query-mode and wrapper mode. 
%
% Typical usage and invocation: ./controller.py -o ./querymode/qmode1.lp -q --qii --qq q,bq,1,val
%  
% -o activates the oclingo wrapper mode with the oclingo encoding given as parameter
% -q activates the oclingo query mode
% --qii enables automatic increase of the incremental step counter for each new query
% --qq specifies the equivalence mapping, see below
%
% For further details, enter "./controller.py -h"


val(1..5).
{bq(X):val(X)}.
a(X):- bq(X).

#cumulative t.
foo(t).

#show foo/1.

% Equivalence assignment between q/2 and bq/1. The controller will
% add those lines automatically when cl argument '--qq q,bq,1,val' is
% given.

%#volatile t.
%#external q(X0,t) : val(X0).                                                                                            
%:- q(X0,t) , not bq(X0).
%:- not q(X0,t) , bq(X0).