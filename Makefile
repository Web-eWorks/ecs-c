SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=obj/%.o)
HEADER = include/ecs.h
OUT = main

CFLAGS += -Iinclude/ -Isrc/
LDFLAGS += -lz

debug: CFLAGS += -g
release: CFLAGS += -O3

.PHONY: default test release debug clean

default: debug

# Rules to trigger recompilation when a source's header has changed.
%.h: ;

# Generate object files from each source file
obj/%.o: src/%.c include/%.h src/%.h $(HEADER)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	@ echo "Compilation successful."

release: $(OUT)
debug: $(OUT)

test: debug
	@ ./main
	@ echo "Tests finished successfully."

clean:
	@ rm -rf $(OBJS) $(OUT)
	@ echo "Cleaned."
