export CFLAGS += -std=c99
export LDFLAGS +=
MAKE += --no-print-directory

debug: export CFLAGS += -g -Wall -Wno-unused-function
release: export CFLAGS += -O3

.PHONY: debug release test clean

debug release:
	@ $(MAKE) -f src/Makefile $@
	@ $(MAKE) -f test/Makefile $@
	@ echo "Finished $@ compilation."

test: debug
	@ ./main
	@ echo "Tests finished successfully."

clean:
	@ $(MAKE) -f src/Makefile clean
	@ $(MAKE) -f test/Makefile clean
