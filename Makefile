#note if WITH_ICLASP is defined also WITH_CLASP has to be defined
export WITH_ICLASP = 0
export WITH_CLASP = 0

ifeq ($(WITH_ICLASP), 1)
export WITH_CLASP = 1
endif

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

