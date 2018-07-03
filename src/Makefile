SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:src/%.c=obj/%.o)
HEADER = include/ecs.h
OUT = libecs.so

CFLAGS += -Iinclude/ -Isrc/ -fPIC
LDFLAGS += -shared

.PHONY: release debug clean
debug: $(OUT)

# Rules to trigger recompilation when a source's header has changed.
%.h: ;

# Generate object files from each source file
obj/%.o: src/%.c include/%.h src/%.h $(HEADER)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	@ echo "Compilation successful."

release: $(OUT)
	strip $^

clean:
	@ rm -rf $(OBJS) $(OUT)
	@ echo "Cleaned library files."
