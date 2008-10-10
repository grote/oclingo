#note if WITH_ICLASP is defined also WITH_CLASP has to be defined
export WITH_ICLASP = 1
export WITH_CLASP = 1

ifeq ($(WITH_ICLASP), 1)
export WITH_CLASP = 1
endif

note:
	echo "U should use cmake." && echo "This Makefile file is deprecated."
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

