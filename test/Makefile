SRCS = $(wildcard test/*.c)
OBJS = $(SRCS:test/%.c=obj/%.o)
HEADERS = include/ecs.h $(wildcard test/*.h)
OUT = main

CFLAGS += -Iinclude -Itest
LDFLAGS += -L. -lecs -lz -lpthread -lm

.PHONY: release debug clean
debug release: $(OUT)

# Rules to trigger recompilation when a source's header has changed.
%.h: ;

# Generate object files from each source file
obj/%.o: test/%.c $(HEADERS)
	@ echo "CC $< > $@"
	@ $(CC) -c -o $@ $< $(CFLAGS)

$(OUT): $(OBJS)
	@ echo "LD [$^] > $@"
	@ $(CC) -o $@ $^ $(LDFLAGS)
	@ echo "> Done."

clean:
	@ rm -rf $(OBJS) $(OUT)
	@ echo "Cleaned test build products."
