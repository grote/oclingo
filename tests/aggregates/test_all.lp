% run
% cat test_all.lp | bingo | clasp --cautious
% cat test_all.lp | bingo -t | bingo | clasp --cautious
% cat test_all.lp | bingo -t | bingo -g | clasp --cautious
% every answer set has to include the atom "all"
#include "test_parity.lp".
#include "test_avg.lp".
#include "test_min.lp".
all :- all(par), all(avg), all(min).
#show all/0.
