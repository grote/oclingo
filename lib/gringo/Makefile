GEN=$(wildcard generated/*.h) $(wildcard generated/*.cpp)
SRC=$(patsubst generated/%,src/%,$(GEN))

all: $(SRC)
	@test -e .copied && rm .copied && touch $(SRC) || true
	${MAKE} -C src all

$(SRC): src/% : generated/%
	@touch .copied
	cp $< $@

%:
	${MAKE} -C src $@
