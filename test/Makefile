SRCS = $(wildcard test/*.c)
OBJS = $(SRCS:test/%.c=obj/%.o)
HEADERS = include/ecs.h $(wildcard test/*.h)
OUT = main

CFLAGS += -Iinclude -Itest
LDFLAGS += -L. -lecs -lz -lpthread

.PHONY: release debug clean
debug release: $(OUT)

# Rules to trigger recompilation when a source's header has changed.
%.h: ;

# Generate object files from each source file
obj/%.o: test/%.c $(HEADERS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	@ rm -rf $(OBJS) $(OUT)
	@ echo "Cleaned test build products."
