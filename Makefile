export CFLAGS += -std=c99
MAKE += --no-print-directory

debug: export CFLAGS += -g -Wall -Wno-unused-function
release: export CFLAGS += -O3

.PHONY: debug release test clean

debug release:
	@ mkdir -p obj/
	@ $(MAKE) -f src/Makefile $@
	@ $(MAKE) -f test/Makefile $@
	@ echo "Finished $@ compilation."

test: debug
	@ ./main
	@ echo "Tests finished successfully."

clean:
	@ $(MAKE) -f src/Makefile clean
	@ $(MAKE) -f test/Makefile clean
	@ rm -r obj/
