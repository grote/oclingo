WITH_ICLASP = 0
WITH_CLASP = 0
GRINGO_VERSION = "2.0.4"
CLASP_VERSION = "1.2.0"
CFLAGS  = -DNDEBUG -O3 -w

ifeq ($(WITH_ICLASP), 1)
export WITH_CLASP = 1
export WITH_ICLASP = 1
export GLOBAL_DEFS = -DWITH_CLASP -DWITH_ICLASP -DGRINGO_VERSION=\"$(GRINGO_VERSION)\" -DCLASP_VERSION=\"$(CLASP_VERSION)\"
else
ifeq ($(WITH_CLASP), 1)
export WITH_CLASP = 1
export GLOBAL_DEFS = -DWITH_CLASP -DGRINGO_VERSION=\"$(GRINGO_VERSION)\" -DCLASP_VERSION=\"$(CLASP_VERSION)\"
else
export GLOBAL_DEFS = -DGRINGO_VERSION=\"$(GRINGO_VERSION)\"
endif
endif
export GLOBAL_CFLAGS = $(CFLAGS)

note:
	@echo "This Makefile file is deprecated."
	@echo "You should use cmake."
	@echo "You can try to type 'make all'."
all: note
	${MAKE} -C lib all
	${MAKE} -C app all

doc: note
	${MAKE} -C doc

clean: note
	${MAKE} -C lib clean
	${MAKE} -C app clean

depend: note
	${MAKE} -C lib depend
	${MAKE} -C app depend

