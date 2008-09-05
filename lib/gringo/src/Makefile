CC      = g++
CFLAGS  = -O3 -Wno-deprecated -I../include -I../.. -DNDEBUG
#CFLAGS  = -O0 -Wall -g -I.
LDFLAGS = 

SOURCES = $(wildcard *.cpp) lparselexer.cpp lparseconverter_impl.cpp plainlparselexer.cpp lparseparser_impl.cpp
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

all: ../../../bin/libgringo.a

../../../bin/libgringo.a: $(OBJECTS)
	$(AR) cvrs $@ $(OBJECTS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

../include/gringo.h: ../include/gringo.h.in
ifeq ($(WITH_ICLASP), 1)
	cat ../include/gringo.h.in | sed -e "s/#cmakedefine WITH_CLASP/#define WITH_CLASP/" -e "s/#cmakedefine WITH_ICLASP/#define WITH_ICLASP/" > $@
else 
ifeq ($(WITH_CLASP), 1)
	cat ../include/gringo.h.in | sed -e "s/#cmakedefine WITH_CLASP/#define WITH_CLASP/" -e "s/#cmakedefine WITH_ICLASP//" > $@
else
	cat ../include/gringo.h.in | sed -e "s/#cmakedefine WITH_CLASP//" -e "s/#cmakedefine WITH_ICLASP//" > $@
endif
endif

plainlparselexer.cpp: plainlparselexer.re
	re2c -o $@ $<

lparselexer.cpp: lparselexer.re
	re2c -o $@ $<

lemon: lemon.c
	gcc -o $@ $<

lparseconverter_impl.cpp lparseconverter_impl.h: lparseconverter_impl.y lemon lempar.c
	./lemon lparseconverter_impl.y
	rm lparseconverter_impl.out
	mv lparseconverter_impl.c lparseconverter_impl.cpp
	touch lparseconverter_impl.h lparseconverter_impl.cpp

lparseparser_impl.cpp lparseparser_impl.h: lparseparser_impl.y lemon lempar.c
	./lemon lparseparser_impl.y
	rm lparseparser_impl.out
	mv lparseparser_impl.c lparseparser_impl.cpp
	touch lparseparser_impl.h lparseparser_impl.cpp

clean:
	rm -f $(OBJECTS) lparseparser_impl.h lparseparser_impl.cpp lparseconverter_impl.h lparseconverter_impl.cpp lparselexer.cpp lemon plainlparselexer.cpp ../../../bin/libgringo.a ../include/gringo.h

depend: ../include/gringo.h $(SOURCES) true
	makedepend -Y -- $(CFLAGS) -- $(SOURCES) &> /dev/null
	rm Makefile.bak
true:

# DO NOT DELETE

aggregateliteral.o: ../include/aggregateliteral.h ../include/gringo.h
aggregateliteral.o: ../include/literal.h ../include/printable.h
aggregateliteral.o: ../include/expandable.h ../include/predicateliteral.h
aggregateliteral.o: ../include/groundable.h ../include/value.h
aggregateliteral.o: ../include/funcsymbol.h ../include/conditionalliteral.h
aggregateliteral.o: ../include/term.h ../include/grounder.h
aggregateliteral.o: ../include/output.h ../include/globalstorage.h
aggregateliteral.o: ../include/statementdependencygraph.h
aggregateliteral.o: ../include/literaldependencygraph.h
aggregateliteral.o: ../include/indexeddomain.h ../include/evaluator.h
aggregateliteral.o: ../include/constant.h ../include/dlvgrounder.h
assignmentliteral.o: ../include/assignmentliteral.h ../include/gringo.h
assignmentliteral.o: ../include/literal.h ../include/printable.h
assignmentliteral.o: ../include/term.h ../include/constant.h
assignmentliteral.o: ../include/value.h ../include/funcsymbol.h
assignmentliteral.o: ../include/indexeddomain.h ../include/dlvgrounder.h
assignmentliteral.o: ../include/grounder.h ../include/output.h
assignmentliteral.o: ../include/globalstorage.h
assignmentliteral.o: ../include/literaldependencygraph.h
basicprogramevaluator.o: ../include/basicprogramevaluator.h
basicprogramevaluator.o: ../include/gringo.h ../include/evaluator.h
basicprogramevaluator.o: ../include/output.h ../include/value.h
basicprogramevaluator.o: ../include/funcsymbol.h ../include/grounder.h
basicprogramevaluator.o: ../include/globalstorage.h ../include/domain.h
claspoutput.o: ../include/claspoutput.h ../include/gringo.h
claspoutput.o: ../include/smodelsconverter.h ../include/output.h
claspoutput.o: ../include/value.h ../include/funcsymbol.h
claspoutput.o: ../../clasp/include/lparse_reader.h
claspoutput.o: ../../clasp/include/program_builder.h
claspoutput.o: ../../clasp/include/solver_types.h
claspoutput.o: ../../clasp/include/literal.h
claspoutput.o: ../../clasp/include/util/platform.h
claspoutput.o: ../../clasp/include/pod_vector.h
claspoutput.o: ../../clasp/include/util/pod_vector.h
claspoutput.o: ../../clasp/include/constraint.h
claspoutput.o: ../../clasp/include/program_rule.h
claspoutput.o: ../../clasp/include/util/misc_types.h
claspoutput.o: ../include/gringoexception.h ../include/grounder.h
claspoutput.o: ../include/globalstorage.h
computeliteral.o: ../include/computeliteral.h ../include/gringo.h
computeliteral.o: ../include/literal.h ../include/printable.h
computeliteral.o: ../include/expandable.h ../include/conditionalliteral.h
computeliteral.o: ../include/groundable.h ../include/value.h
computeliteral.o: ../include/funcsymbol.h ../include/grounder.h
computeliteral.o: ../include/output.h ../include/globalstorage.h
computeliteral.o: ../include/statementdependencygraph.h
computeliteral.o: ../include/literaldependencygraph.h
conditionalliteral.o: ../include/conditionalliteral.h ../include/gringo.h
conditionalliteral.o: ../include/literal.h ../include/printable.h
conditionalliteral.o: ../include/groundable.h ../include/expandable.h
conditionalliteral.o: ../include/predicateliteral.h ../include/value.h
conditionalliteral.o: ../include/funcsymbol.h ../include/grounder.h
conditionalliteral.o: ../include/output.h ../include/globalstorage.h
conditionalliteral.o: ../include/term.h ../include/indexeddomain.h
conditionalliteral.o: ../include/dlvgrounder.h
conditionalliteral.o: ../include/literaldependencygraph.h
conditionalliteral.o: ../include/aggregateliteral.h
conjunctionaggregate.o: ../include/conjunctionaggregate.h ../include/gringo.h
conjunctionaggregate.o: ../include/aggregateliteral.h ../include/literal.h
conjunctionaggregate.o: ../include/printable.h ../include/expandable.h
conjunctionaggregate.o: ../include/conditionalliteral.h
conjunctionaggregate.o: ../include/groundable.h ../include/predicateliteral.h
conjunctionaggregate.o: ../include/value.h ../include/funcsymbol.h
conjunctionaggregate.o: ../include/term.h ../include/output.h
constant.o: ../include/constant.h ../include/gringo.h ../include/term.h
constant.o: ../include/printable.h ../include/value.h ../include/funcsymbol.h
constant.o: ../include/grounder.h ../include/output.h
constant.o: ../include/globalstorage.h
countaggregate.o: ../include/countaggregate.h ../include/gringo.h
countaggregate.o: ../include/aggregateliteral.h ../include/literal.h
countaggregate.o: ../include/printable.h ../include/expandable.h
countaggregate.o: ../include/conditionalliteral.h ../include/groundable.h
countaggregate.o: ../include/term.h ../include/value.h
countaggregate.o: ../include/funcsymbol.h ../include/output.h
disjunctionaggregate.o: ../include/disjunctionaggregate.h ../include/gringo.h
disjunctionaggregate.o: ../include/aggregateliteral.h ../include/literal.h
disjunctionaggregate.o: ../include/printable.h ../include/expandable.h
disjunctionaggregate.o: ../include/conditionalliteral.h
disjunctionaggregate.o: ../include/groundable.h ../include/predicateliteral.h
disjunctionaggregate.o: ../include/value.h ../include/funcsymbol.h
disjunctionaggregate.o: ../include/term.h ../include/output.h
dlvgrounder.o: ../include/dlvgrounder.h ../include/gringo.h
dlvgrounder.o: ../include/grounder.h ../include/output.h ../include/value.h
dlvgrounder.o: ../include/funcsymbol.h ../include/globalstorage.h
dlvgrounder.o: ../include/groundable.h ../include/literal.h
dlvgrounder.o: ../include/printable.h ../include/indexeddomain.h
dlvgrounder.o: ../include/literaldependencygraph.h
domain.o: ../include/domain.h ../include/gringo.h ../include/value.h
domain.o: ../include/funcsymbol.h
evaluator.o: ../include/evaluator.h ../include/gringo.h ../include/grounder.h
evaluator.o: ../include/output.h ../include/value.h ../include/funcsymbol.h
evaluator.o: ../include/globalstorage.h
funcsymbol.o: ../include/funcsymbol.h ../include/gringo.h ../include/value.h
funcsymbolterm.o: ../include/funcsymbolterm.h ../include/gringo.h
funcsymbolterm.o: ../include/term.h ../include/printable.h ../include/value.h
funcsymbolterm.o: ../include/funcsymbol.h ../include/grounder.h
funcsymbolterm.o: ../include/output.h ../include/globalstorage.h
funcsymbolterm.o: ../include/assignmentliteral.h ../include/literal.h
funcsymbolterm.o: ../include/constant.h ../include/expandable.h
functionterm.o: ../include/functionterm.h ../include/gringo.h
functionterm.o: ../include/term.h ../include/printable.h ../include/value.h
functionterm.o: ../include/funcsymbol.h
globalstorage.o: ../include/globalstorage.h ../include/gringo.h
globalstorage.o: ../include/funcsymbol.h ../include/domain.h
globalstorage.o: ../include/value.h
gringolexer.o: ../include/gringolexer.h ../include/gringo.h
gringoparser.o: ../include/gringoparser.h ../include/gringo.h
gringoparser.o: ../include/gringolexer.h
grounder.o: ../include/grounder.h ../include/gringo.h ../include/output.h
grounder.o: ../include/value.h ../include/funcsymbol.h
grounder.o: ../include/globalstorage.h ../include/statement.h
grounder.o: ../include/printable.h ../include/groundable.h
grounder.o: ../include/constant.h ../include/term.h
grounder.o: ../include/predicateliteral.h ../include/literal.h
grounder.o: ../include/expandable.h ../include/normalrule.h
grounder.o: ../include/statementdependencygraph.h ../include/program.h
grounder.o: ../include/evaluator.h ../include/domain.h
grounder.o: ../include/gringoexception.h
indexeddomain.o: ../include/indexeddomain.h ../include/gringo.h
indexeddomain.o: ../include/value.h ../include/funcsymbol.h
indexeddomain.o: ../include/grounder.h ../include/output.h
indexeddomain.o: ../include/globalstorage.h ../include/dlvgrounder.h
indexeddomain.o: ../include/literal.h ../include/printable.h
indexeddomain.o: ../include/constant.h ../include/term.h
literal.o: ../include/literal.h ../include/gringo.h ../include/printable.h
literaldependencygraph.o: ../include/literaldependencygraph.h
literaldependencygraph.o: ../include/gringo.h ../include/literal.h
literaldependencygraph.o: ../include/printable.h
literalstatement.o: ../include/literalstatement.h ../include/gringo.h
literalstatement.o: ../include/statement.h ../include/printable.h
literalstatement.o: ../include/groundable.h ../include/expandable.h
literalstatement.o: ../include/statementdependencygraph.h
literalstatement.o: ../include/literaldependencygraph.h
literalstatement.o: ../include/conditionalliteral.h ../include/literal.h
literalstatement.o: ../include/output.h ../include/value.h
literalstatement.o: ../include/funcsymbol.h ../include/grounder.h
literalstatement.o: ../include/globalstorage.h ../include/evaluator.h
literalstatement.o: ../include/gringoexception.h
lparseconverter.o: ../include/lparseconverter.h ../include/gringo.h
lparseconverter.o: ../include/gringoparser.h ../include/globalstorage.h
lparseconverter.o: ../include/funcsymbol.h lparseconverter_impl.h
lparseconverter.o: ../include/plainlparselexer.h ../include/gringolexer.h
lparseconverter.o: ../include/grounder.h ../include/output.h
lparseconverter.o: ../include/value.h
lparseoutput.o: ../include/lparseoutput.h ../include/gringo.h
lparseoutput.o: ../include/output.h ../include/value.h
lparseoutput.o: ../include/funcsymbol.h
lparseparser.o: ../include/lparseparser.h ../include/gringo.h
lparseparser.o: ../include/gringoparser.h lparseparser_impl.h
lparseparser.o: ../include/lparselexer.h ../include/gringolexer.h
lparseparser.o: ../include/grounder.h ../include/output.h ../include/value.h
lparseparser.o: ../include/funcsymbol.h ../include/globalstorage.h
maxaggregate.o: ../include/maxaggregate.h ../include/gringo.h
maxaggregate.o: ../include/aggregateliteral.h ../include/literal.h
maxaggregate.o: ../include/printable.h ../include/expandable.h
maxaggregate.o: ../include/conditionalliteral.h ../include/groundable.h
maxaggregate.o: ../include/term.h ../include/value.h ../include/funcsymbol.h
maxaggregate.o: ../include/output.h ../include/dlvgrounder.h
maxaggregate.o: ../include/grounder.h ../include/globalstorage.h
maxaggregate.o: ../include/indexeddomain.h ../include/constant.h
minaggregate.o: ../include/minaggregate.h ../include/gringo.h
minaggregate.o: ../include/aggregateliteral.h ../include/literal.h
minaggregate.o: ../include/printable.h ../include/expandable.h
minaggregate.o: ../include/conditionalliteral.h ../include/groundable.h
minaggregate.o: ../include/term.h ../include/value.h ../include/funcsymbol.h
minaggregate.o: ../include/output.h ../include/dlvgrounder.h
minaggregate.o: ../include/grounder.h ../include/globalstorage.h
minaggregate.o: ../include/indexeddomain.h ../include/constant.h
multipleargsterm.o: ../include/multipleargsterm.h ../include/gringo.h
multipleargsterm.o: ../include/term.h ../include/printable.h
multipleargsterm.o: ../include/literal.h ../include/expandable.h
multipleargsterm.o: ../include/value.h ../include/funcsymbol.h
normalrule.o: ../include/normalrule.h ../include/gringo.h
normalrule.o: ../include/statement.h ../include/printable.h
normalrule.o: ../include/groundable.h ../include/expandable.h
normalrule.o: ../include/literal.h ../include/predicateliteral.h
normalrule.o: ../include/value.h ../include/funcsymbol.h
normalrule.o: ../include/statementdependencygraph.h
normalrule.o: ../include/literaldependencygraph.h ../include/grounder.h
normalrule.o: ../include/output.h ../include/globalstorage.h
normalrule.o: ../include/dlvgrounder.h ../include/evaluator.h
normalrule.o: ../include/gringoexception.h ../include/indexeddomain.h
normalrule.o: ../include/constant.h ../include/term.h
optimizeliteral.o: ../include/optimizeliteral.h ../include/gringo.h
optimizeliteral.o: ../include/literal.h ../include/printable.h
optimizeliteral.o: ../include/expandable.h ../include/conditionalliteral.h
optimizeliteral.o: ../include/groundable.h ../include/value.h
optimizeliteral.o: ../include/funcsymbol.h ../include/grounder.h
optimizeliteral.o: ../include/output.h ../include/globalstorage.h
optimizeliteral.o: ../include/statementdependencygraph.h
optimizeliteral.o: ../include/literaldependencygraph.h
output.o: ../include/output.h ../include/gringo.h ../include/value.h
output.o: ../include/funcsymbol.h ../include/domain.h ../include/grounder.h
output.o: ../include/globalstorage.h ../include/pilsoutput.h
output.o: ../include/gringoexception.h
pilsoutput.o: ../include/pilsoutput.h ../include/gringo.h ../include/output.h
pilsoutput.o: ../include/value.h ../include/funcsymbol.h
predicateliteral.o: ../include/statementdependencygraph.h ../include/gringo.h
predicateliteral.o: ../include/printable.h
predicateliteral.o: ../include/literaldependencygraph.h
predicateliteral.o: ../include/predicateliteral.h ../include/literal.h
predicateliteral.o: ../include/groundable.h ../include/expandable.h
predicateliteral.o: ../include/value.h ../include/funcsymbol.h
predicateliteral.o: ../include/assignmentliteral.h
predicateliteral.o: ../include/relationliteral.h ../include/grounder.h
predicateliteral.o: ../include/output.h ../include/globalstorage.h
predicateliteral.o: ../include/constant.h ../include/term.h
predicateliteral.o: ../include/domain.h ../include/rangeterm.h
predicateliteral.o: ../include/rangeliteral.h ../include/indexeddomain.h
predicateliteral.o: ../include/dlvgrounder.h ../include/multipleargsterm.h
predicateliteral.o: ../include/evaluator.h
printable.o: ../include/printable.h ../include/gringo.h
program.o: ../include/program.h ../include/gringo.h ../include/printable.h
program.o: ../include/statement.h ../include/groundable.h
program.o: ../include/grounder.h ../include/output.h ../include/value.h
program.o: ../include/funcsymbol.h ../include/globalstorage.h
program.o: ../include/literal.h ../include/evaluator.h
program.o: ../include/basicprogramevaluator.h
rangeliteral.o: ../include/rangeliteral.h ../include/gringo.h
rangeliteral.o: ../include/literal.h ../include/printable.h
rangeliteral.o: ../include/constant.h ../include/term.h ../include/value.h
rangeliteral.o: ../include/funcsymbol.h ../include/indexeddomain.h
rangeliteral.o: ../include/dlvgrounder.h ../include/grounder.h
rangeliteral.o: ../include/output.h ../include/globalstorage.h
rangeliteral.o: ../include/literaldependencygraph.h
rangeterm.o: ../include/rangeterm.h ../include/gringo.h ../include/term.h
rangeterm.o: ../include/printable.h ../include/value.h
rangeterm.o: ../include/funcsymbol.h ../include/grounder.h
rangeterm.o: ../include/output.h ../include/globalstorage.h
rangeterm.o: ../include/expandable.h ../include/constant.h
rangeterm.o: ../include/rangeliteral.h ../include/literal.h
relationliteral.o: ../include/relationliteral.h ../include/gringo.h
relationliteral.o: ../include/literal.h ../include/printable.h
relationliteral.o: ../include/term.h ../include/value.h
relationliteral.o: ../include/funcsymbol.h ../include/indexeddomain.h
relationliteral.o: ../include/literaldependencygraph.h
smodelsconverter.o: ../include/smodelsconverter.h ../include/gringo.h
smodelsconverter.o: ../include/output.h ../include/value.h
smodelsconverter.o: ../include/funcsymbol.h ../include/grounder.h
smodelsconverter.o: ../include/globalstorage.h
smodelsoutput.o: ../include/smodelsoutput.h ../include/gringo.h
smodelsoutput.o: ../include/smodelsconverter.h ../include/output.h
smodelsoutput.o: ../include/value.h ../include/funcsymbol.h
smodelsoutput.o: ../include/grounder.h ../include/globalstorage.h
smodelsoutput.o: ../include/gringoexception.h
statement.o: ../include/statement.h ../include/gringo.h
statement.o: ../include/printable.h ../include/groundable.h
statementdependencygraph.o: ../include/statementdependencygraph.h
statementdependencygraph.o: ../include/gringo.h ../include/printable.h
statementdependencygraph.o: ../include/predicateliteral.h
statementdependencygraph.o: ../include/literal.h ../include/groundable.h
statementdependencygraph.o: ../include/expandable.h ../include/value.h
statementdependencygraph.o: ../include/funcsymbol.h ../include/domain.h
statementdependencygraph.o: ../include/grounder.h ../include/output.h
statementdependencygraph.o: ../include/globalstorage.h ../include/program.h
sumaggregate.o: ../include/sumaggregate.h ../include/gringo.h
sumaggregate.o: ../include/aggregateliteral.h ../include/literal.h
sumaggregate.o: ../include/printable.h ../include/expandable.h
sumaggregate.o: ../include/conditionalliteral.h ../include/groundable.h
sumaggregate.o: ../include/term.h ../include/value.h ../include/funcsymbol.h
sumaggregate.o: ../include/output.h
term.o: ../include/term.h ../include/gringo.h ../include/printable.h
value.o: ../include/value.h ../include/gringo.h ../include/funcsymbol.h
value.o: ../include/gringoexception.h
lparselexer.o: ../include/lparselexer.h ../include/gringo.h
lparselexer.o: ../include/gringolexer.h ../include/lparseparser.h
lparselexer.o: ../include/gringoparser.h lparseparser_impl.h
lparseconverter_impl.o: ../include/gringo.h ../include/lparseconverter.h
lparseconverter_impl.o: ../include/gringoparser.h ../include/globalstorage.h
lparseconverter_impl.o: ../include/funcsymbol.h ../include/output.h
lparseconverter_impl.o: ../include/value.h
plainlparselexer.o: ../include/plainlparselexer.h ../include/gringo.h
plainlparselexer.o: ../include/gringolexer.h ../include/lparseconverter.h
plainlparselexer.o: ../include/gringoparser.h ../include/globalstorage.h
plainlparselexer.o: ../include/funcsymbol.h lparseconverter_impl.h
lparseparser_impl.o: ../include/gringo.h ../include/lparseparser.h
lparseparser_impl.o: ../include/gringoparser.h ../include/literal.h
lparseparser_impl.o: ../include/printable.h ../include/aggregateliteral.h
lparseparser_impl.o: ../include/expandable.h ../include/predicateliteral.h
lparseparser_impl.o: ../include/groundable.h ../include/value.h
lparseparser_impl.o: ../include/funcsymbol.h ../include/conditionalliteral.h
lparseparser_impl.o: ../include/relationliteral.h
lparseparser_impl.o: ../include/assignmentliteral.h
lparseparser_impl.o: ../include/computeliteral.h ../include/optimizeliteral.h
lparseparser_impl.o: ../include/sumaggregate.h ../include/minaggregate.h
lparseparser_impl.o: ../include/maxaggregate.h ../include/countaggregate.h
lparseparser_impl.o: ../include/disjunctionaggregate.h
lparseparser_impl.o: ../include/conjunctionaggregate.h ../include/term.h
lparseparser_impl.o: ../include/constant.h ../include/functionterm.h
lparseparser_impl.o: ../include/funcsymbolterm.h ../include/rangeterm.h
lparseparser_impl.o: ../include/multipleargsterm.h ../include/normalrule.h
lparseparser_impl.o: ../include/statement.h ../include/literalstatement.h
lparseparser_impl.o: ../include/grounder.h ../include/output.h
lparseparser_impl.o: ../include/globalstorage.h
