export CFLAGS += -std=c99
MAKE += --no-print-directory

MEMCHECK = valgrind --tool=memcheck --leak-check=full --track-origins=yes
MASSIF = valgrind --tool=massif --massif-out-file=massif.out
CALLGRIND = valgrind --tool=callgrind --callgrind-out-file=callgrind.out
CACHEGRIND = valgrind --tool=cachegrind --cachegrind-out-file=cachegrind.out

GR = $(shell tput setaf 2)
BD = $(shell tput bold)
RS = $(shell tput sgr0)

debug: export CFLAGS += -g -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Werror=incompatible-pointer-types
release: export CFLAGS += -O3

.PHONY: debug release test test-callgrind test-cachegrind test-memcheck test-massif clean

debug release:
	@ mkdir -p obj/
	@ $(MAKE) -f src/Makefile $@
	@ $(MAKE) -f test/Makefile $@
	@ echo "> $(GR)Finished $@ compilation.$(RS)"

test-run: debug
	@ ./main
	@ echo "$(GR)$(BD)Test runner finished successfully.$(RS)"

test-callgrind:
	@ echo "Running callgrind."
	@ $(CALLGRIND) ./main &> /dev/null
	@ echo "> $(GR)Callgrind finished successfully.$(RS)"

test-cachegrind:
	@ echo "Running cachegrind."
	@ $(CACHEGRIND) ./main > /dev/null
	@ echo "> $(GR)cachegrind finished successfully.$(RS)"

test-memcheck:
	@ echo "Running memcheck."
	@ $(MEMCHECK) ./main 2> memcheck.out.txt > /dev/null
	@ echo "> $(GR)Memcheck finished successfully.$(RS)"

test-massif:
	@ echo "Running massif."
	@ $(MASSIF) ./main &> /dev/null
	@ ms_print massif.out > massif.out.txt
	@ echo "> $(GR)Massif finished successfully.$(RS)"

test-check: test-callgrind test-cachegrind test-memcheck test-massif
	@ echo "$(GR)$(BD)All test data logged successfully!$(RS)"

test: debug test-check test-run

clean:
	@ $(MAKE) -f src/Makefile clean
	@ $(MAKE) -f test/Makefile clean
	@ rm -rf obj/
	@ rm -f {memcheck,cachegrind,callgrind,massif}.out*
