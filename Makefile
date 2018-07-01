SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=obj/%.o)
HEADER = include/ecs.h
OUT = main

CFLAGS += -O1 -Iinclude/ -Isrc/
LDFLAGS += -lz

.PHONY: default test clean

default: $(OUT)

# Rules to trigger recompilation when a source's header has changed.
%.h: ;

# Generate object files from each source file
obj/%.o: src/%.c include/%.h src/%.h $(HEADER)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	@ echo "Compilation successful."

test: $(OUT)
	@ ./main
	@ echo "Tests finished successfully."

clean:
	@ rm -rf $(OBJS) $(OUT)
	@ echo "Cleaned."
