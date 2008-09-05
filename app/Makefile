CC      = g++
CFLAGS  = -O3 -Wno-deprecated -I../lib/gringo/include -I../lib -DNDEBUG
#CFLAGS  = -O0 -Wall -g -I.
LDFLAGS = 
STATICLIBS = ../bin/libgringo.a

ifeq ($(WITH_CLASP), 1)
SOURCES     = $(wildcard *.cpp program_opts/*.cpp)
STATICLIBS += ../bin/libclasp.a
else
SOURCES = main.cpp
endif

OBJECTS = $(patsubst %.cpp,%.o,$(SOURCES))

all: ../bin/gringo

../bin/gringo: $(OBJECTS) $(STATICLIBS)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS) ../bin/libgringo.a ../bin/libclasp.a

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS)

depend: $(SOURCES)
	makedepend -Y -- $(SOURCES) &> /dev/null
	rm Makefile.bak

# DO NOT DELETE

options.o: options.h program_opts/program_options.h program_opts/value_base.h
options.o: program_opts/detail/smartpointer.h program_opts/value.h
options.o: program_opts/program_options.h program_opts/errors.h
timer.o: timer.h
program_opts/program_options.o: program_opts/program_options.h
program_opts/program_options.o: program_opts/errors.h
program_opts/value.o: program_opts/value.h program_opts/program_options.h
program_opts/value.o: program_opts/value_base.h program_opts/errors.h
