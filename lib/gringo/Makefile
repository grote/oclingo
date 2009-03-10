all:
	test -d generated && cp generated/* src/ || true
	${MAKE} -C src all

%:
	${MAKE} -C src $@
