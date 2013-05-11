% COMMAND LINE: oclingo encoding.lp append.lp --import=all 0

%*
% Setup information initially provided to qontroller wrapper
#setup.
#domain edge(X,Y) : X := 1..4, Y := 1..4, X != Y.
#domain mark(X,C) : X := 1..4, color(C).
#choose edge/2.
#define edge/2.
#query mark/2.
#show edge/2.
#endsetup.
*%

%* Proposed mappings of setup statements to be appended to encoding.lp follow *%

%* Additions to be appended as base program part *%

#base.

% #domain edge(X,Y) : X := 1..4, Y := 1..4, X != Y.
_domain_edge(X,Y) :- X := 1..4, Y := 1..4, X != Y.

% #domain mark(X,C) : X := 1..4, color(C).
_domain_mark(X,C) :- X := 1..4, color(C).

% #choose edge/2.
{ edge(X1,X2) : _domain_edge(X1,X2) }.

% #show edge/2.
#show edge(X1,X2) : _domain_edge(X1,X2).

%* Additions to be appended as cumulative program part*%

#cumulative t.

% #query mark/2.
#external _assert_mark(X1,X2,t) : _domain_mark(X1,X2).
_derive_mark(X1,X2,t) :- _domain_mark(X1,X2), _assert_mark(X1,X2,t).
_derive_mark(X1,X2,t) :- _domain_mark(X1,X2), _derive_mark(X1,X2,t-1).

% #define edge/2.
#external _assert_edge(X1,X2,t) : _domain_edge(X1,X2).
_derive_edge(X1,X2,t) :- _domain_edge(X1,X2), _assert_edge(X1,X2,t).
_derive_edge(X1,X2,t) :- _domain_edge(X1,X2), _derive_edge(X1,X2,t-1).

%* Additions to be appended as volatile program part *%

#volatile t.

% #query mark/2.
:- _domain_mark(X1,X2), _derive_mark(X1,X2,t), not mark(X1,X2).

% #define edge/2.
:- _domain_edge(X1,X2), _derive_edge(X1,X2,t), not edge(X1,X2).
:- _domain_edge(X1,X2), edge(X1,X2), not _derive_edge(X1,X2,t).
