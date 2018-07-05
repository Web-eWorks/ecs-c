export CFLAGS += -std=c99
MAKE += --no-print-directory

MEMCHECK = valgrind --tool=memcheck --leak-check=full --track-origins=yes
MASSIF = valgrind --tool=massif --massif-out-file=massif.out

GR = $(shell tput setaf 2)
BD = $(shell tput bold)
RS = $(shell tput sgr0)

debug: export CFLAGS += -g -Wall -Wno-unused-function
release: export CFLAGS += -O3

.PHONY: debug release test clean

debug release:
	@ mkdir -p obj/
	@ $(MAKE) -f src/Makefile $@
	@ $(MAKE) -f test/Makefile $@
	@ echo "> $(GR)Finished $@ compilation.$(RS)"

test-all: test
	@ echo "Running memcheck."
	@ $(MEMCHECK) ./main 2> memcheck.out.txt > /dev/null
	@ echo "> $(GR)Memcheck finished successfully.$(RS)"
	@ echo "Running massif."
	@ $(MASSIF) ./main &> /dev/null
	@ ms_print massif.out > massif.out.txt
	@ echo "> $(GR)Massif finished successfully.$(RS)"
	@ echo
	@ echo "$(GR)$(BD)All test data logged successfully!$(RS)"

test: debug
	@ ./main
	@ echo "> $(GR)Tests finished successfully.$(RS)"

clean:
	@ $(MAKE) -f src/Makefile clean
	@ $(MAKE) -f test/Makefile clean
	@ rm -rf obj/
	@ rm -f memcheck.out.txt massif.out massif.out.txt
