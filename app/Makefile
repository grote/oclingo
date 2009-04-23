ifeq ($(WITH_CLASP), 1)
SOURCES    = $(wildcard *.cpp program_opts/*.cpp)
STATICLIBS = ../bin/libclasp.a
ifeq ($(WITH_ICLASP), 1)
DEFINES = $(GLOBAL_DEFS) -DEXECUTABLE="\"iclingo\""
EXECUTABLE = iclingo
else
DEFINES = $(GLOBAL_DEFS) -DEXECUTABLE="\"clingo\""
EXECUTABLE = clingo
endif
else
SOURCES = main.cpp options.cpp timer.cpp $(wildcard program_opts/*.cpp)
EXECUTABLE = gringo 
DEFINES = $(GLOBAL_DEFS) -DEXECUTABLE="\"gringo\""
endif

CC      = g++
CFLAGS  = $(GLOBAL_CFLAGS) -I../lib/gringo/include -I../lib $(DEFINES)
LDFLAGS = 

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

all: ../bin/$(EXECUTABLE)

../bin/$(EXECUTABLE): $(OBJECTS) $(STATICLIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) ../bin/libgringo.a ${STATICLIBS}

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS)

depend: $(SOURCES)
	makedepend -Y -- $(SOURCES) &> /dev/null
	rm Makefile.bak

# DO NOT DELETE

main.o: options.h program_opts/program_options.h program_opts/value_base.h
main.o: program_opts/detail/smartpointer.h timer.h
options.o: options.h program_opts/program_options.h program_opts/value_base.h
options.o: program_opts/detail/smartpointer.h program_opts/value.h
options.o: program_opts/program_options.h program_opts/errors.h
timer.o: timer.h
program_opts/program_options.o: program_opts/program_options.h
program_opts/program_options.o: program_opts/errors.h
program_opts/value.o: program_opts/value.h program_opts/program_options.h
program_opts/value.o: program_opts/value_base.h program_opts/errors.h
