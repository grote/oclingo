CC      = g++
#AR      = gar
CFLAGS  = -O3 -Wno-deprecated -I../.. -DNDEBUG
#CFLAGS  = -O0 -Wall -g -I.
LDFLAGS = 

SOURCES = $(wildcard *.cpp)
OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

all: ../../../bin/libclasp.a

../../../bin/libclasp.a: $(OBJECTS)
	$(AR) cvrs $@ $(OBJECTS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) ../../../bin/libclasp.a

depend: $(SOURCES) true
	makedepend -Y -- $(CFLAGS) -- $(SOURCES) &> /dev/null
	rm Makefile.bak
true:

# DO NOT DELETE

cb_enumerator.o: ../../clasp/include/cb_enumerator.h
cb_enumerator.o: ../../clasp/include/solve_algorithms.h
cb_enumerator.o: ../../clasp/include/literal.h
cb_enumerator.o: ../../clasp/include/util/platform.h
cb_enumerator.o: ../../clasp/include/pod_vector.h
cb_enumerator.o: ../../clasp/include/util/pod_vector.h
cb_enumerator.o: ../../clasp/include/constraint.h
cb_enumerator.o: ../../clasp/include/program_builder.h
cb_enumerator.o: ../../clasp/include/solver_types.h
cb_enumerator.o: ../../clasp/include/program_rule.h
cb_enumerator.o: ../../clasp/include/util/misc_types.h
cb_enumerator.o: ../../clasp/include/solver.h ../../clasp/include/clause.h
cb_enumerator.o: ../../clasp/include/smodels_constraints.h
clause.o: ../../clasp/include/clause.h ../../clasp/include/constraint.h
clause.o: ../../clasp/include/literal.h ../../clasp/include/util/platform.h
clause.o: ../../clasp/include/pod_vector.h
clause.o: ../../clasp/include/util/pod_vector.h
clause.o: ../../clasp/include/util/misc_types.h ../../clasp/include/solver.h
clause.o: ../../clasp/include/solver_types.h
constraint.o: ../../clasp/include/constraint.h ../../clasp/include/literal.h
constraint.o: ../../clasp/include/util/platform.h
constraint.o: ../../clasp/include/pod_vector.h
constraint.o: ../../clasp/include/util/pod_vector.h
heuristics.o: ../../clasp/include/heuristics.h ../../clasp/include/solver.h
heuristics.o: ../../clasp/include/solver_types.h
heuristics.o: ../../clasp/include/literal.h
heuristics.o: ../../clasp/include/util/platform.h
heuristics.o: ../../clasp/include/pod_vector.h
heuristics.o: ../../clasp/include/util/pod_vector.h
heuristics.o: ../../clasp/include/constraint.h
heuristics.o: ../../clasp/include/util/indexed_priority_queue.h
heuristics.o: ../../clasp/include/util/pod_vector.h
lparse_reader.o: ../../clasp/include/lparse_reader.h
lparse_reader.o: ../../clasp/include/program_builder.h
lparse_reader.o: ../../clasp/include/solver_types.h
lparse_reader.o: ../../clasp/include/literal.h
lparse_reader.o: ../../clasp/include/util/platform.h
lparse_reader.o: ../../clasp/include/pod_vector.h
lparse_reader.o: ../../clasp/include/util/pod_vector.h
lparse_reader.o: ../../clasp/include/constraint.h
lparse_reader.o: ../../clasp/include/program_rule.h
lparse_reader.o: ../../clasp/include/util/misc_types.h
lparse_reader.o: ../../clasp/include/clause.h ../../clasp/include/solver.h
model_enumerators.o: ../../clasp/include/model_enumerators.h
model_enumerators.o: ../../clasp/include/solve_algorithms.h
model_enumerators.o: ../../clasp/include/literal.h
model_enumerators.o: ../../clasp/include/util/platform.h
model_enumerators.o: ../../clasp/include/pod_vector.h
model_enumerators.o: ../../clasp/include/util/pod_vector.h
model_enumerators.o: ../../clasp/include/constraint.h
model_enumerators.o: ../../clasp/include/program_builder.h
model_enumerators.o: ../../clasp/include/solver_types.h
model_enumerators.o: ../../clasp/include/program_rule.h
model_enumerators.o: ../../clasp/include/util/misc_types.h
model_enumerators.o: ../../clasp/include/clause.h
model_enumerators.o: ../../clasp/include/solver.h
model_enumerators.o: ../../clasp/include/smodels_constraints.h
preprocessor.o: ../../clasp/include/preprocessor.h
preprocessor.o: ../../clasp/include/literal.h
preprocessor.o: ../../clasp/include/util/platform.h
preprocessor.o: ../../clasp/include/pod_vector.h
preprocessor.o: ../../clasp/include/util/pod_vector.h
preprocessor.o: ../../clasp/include/program_builder.h
preprocessor.o: ../../clasp/include/solver_types.h
preprocessor.o: ../../clasp/include/constraint.h
preprocessor.o: ../../clasp/include/program_rule.h
preprocessor.o: ../../clasp/include/util/misc_types.h
program_builder.o: ../../clasp/include/program_builder.h
program_builder.o: ../../clasp/include/solver_types.h
program_builder.o: ../../clasp/include/literal.h
program_builder.o: ../../clasp/include/util/platform.h
program_builder.o: ../../clasp/include/pod_vector.h
program_builder.o: ../../clasp/include/util/pod_vector.h
program_builder.o: ../../clasp/include/constraint.h
program_builder.o: ../../clasp/include/program_rule.h
program_builder.o: ../../clasp/include/util/misc_types.h
program_builder.o: ../../clasp/include/preprocessor.h
program_builder.o: ../../clasp/include/solver.h ../../clasp/include/clause.h
program_builder.o: ../../clasp/include/smodels_constraints.h
program_builder.o: ../../clasp/include/unfounded_check.h
program_rule.o: ../../clasp/include/program_rule.h
program_rule.o: ../../clasp/include/literal.h
program_rule.o: ../../clasp/include/util/platform.h
program_rule.o: ../../clasp/include/pod_vector.h
program_rule.o: ../../clasp/include/util/pod_vector.h
program_rule.o: ../../clasp/include/program_builder.h
program_rule.o: ../../clasp/include/solver_types.h
program_rule.o: ../../clasp/include/constraint.h
program_rule.o: ../../clasp/include/util/misc_types.h
satelite.o: ../../clasp/include/satelite.h ../../clasp/include/solver.h
satelite.o: ../../clasp/include/solver_types.h ../../clasp/include/literal.h
satelite.o: ../../clasp/include/util/platform.h
satelite.o: ../../clasp/include/pod_vector.h
satelite.o: ../../clasp/include/util/pod_vector.h
satelite.o: ../../clasp/include/constraint.h
satelite.o: ../../clasp/include/util/indexed_priority_queue.h
satelite.o: ../../clasp/include/util/pod_vector.h
satelite.o: ../../clasp/include/clause.h
satelite.o: ../../clasp/include/util/misc_types.h
smodels_constraints.o: ../../clasp/include/smodels_constraints.h
smodels_constraints.o: ../../clasp/include/constraint.h
smodels_constraints.o: ../../clasp/include/literal.h
smodels_constraints.o: ../../clasp/include/util/platform.h
smodels_constraints.o: ../../clasp/include/pod_vector.h
smodels_constraints.o: ../../clasp/include/util/pod_vector.h
smodels_constraints.o: ../../clasp/include/clause.h
smodels_constraints.o: ../../clasp/include/util/misc_types.h
smodels_constraints.o: ../../clasp/include/solver.h
smodels_constraints.o: ../../clasp/include/solver_types.h
solve_algorithms.o: ../../clasp/include/solve_algorithms.h
solve_algorithms.o: ../../clasp/include/literal.h
solve_algorithms.o: ../../clasp/include/util/platform.h
solve_algorithms.o: ../../clasp/include/pod_vector.h
solve_algorithms.o: ../../clasp/include/util/pod_vector.h
solve_algorithms.o: ../../clasp/include/constraint.h
solve_algorithms.o: ../../clasp/include/solver.h
solve_algorithms.o: ../../clasp/include/solver_types.h
solve_algorithms.o: ../../clasp/include/smodels_constraints.h
solver.o: ../../clasp/include/solver.h ../../clasp/include/solver_types.h
solver.o: ../../clasp/include/literal.h ../../clasp/include/util/platform.h
solver.o: ../../clasp/include/pod_vector.h
solver.o: ../../clasp/include/util/pod_vector.h
solver.o: ../../clasp/include/constraint.h ../../clasp/include/clause.h
solver.o: ../../clasp/include/util/misc_types.h
unfounded_check.o: ../../clasp/include/unfounded_check.h
unfounded_check.o: ../../clasp/include/solver.h
unfounded_check.o: ../../clasp/include/solver_types.h
unfounded_check.o: ../../clasp/include/literal.h
unfounded_check.o: ../../clasp/include/util/platform.h
unfounded_check.o: ../../clasp/include/pod_vector.h
unfounded_check.o: ../../clasp/include/util/pod_vector.h
unfounded_check.o: ../../clasp/include/constraint.h
unfounded_check.o: ../../clasp/include/program_builder.h
unfounded_check.o: ../../clasp/include/program_rule.h
unfounded_check.o: ../../clasp/include/util/misc_types.h
unfounded_check.o: ../../clasp/include/clause.h
