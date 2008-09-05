#note if WITH_ICLASP is defined also WITH_CLASP has to be defined
export WITH_ICLASP = 1
export WITH_CLASP = 1

ifeq ($(WITH_ICLASP), 1)
export WITH_CLASP = 1
endif

note:
	echo "U should use cmake." && echo "This Makefile file is deprecated."
all: note
	make -C lib all
	make -C app all

doc: note
	make -C doc

clean: note
	make -C lib clean
	make -C app clean

depend: note
	make -C lib depend
	make -C app depend

