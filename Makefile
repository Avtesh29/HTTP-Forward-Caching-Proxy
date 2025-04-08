EXECBIN  = httpproxy
SOURCES  = $(wildcard *.c)
HEADERS  = $(wildcard *.h)
OBJECTS  = $(SOURCES:%.c=build/%.o)
FORMATS  = $(SOURCES:%.c=.format/%.c.fmt) $(HEADERS:%.h=.format/%.h.fmt)

CC       = clang
FORMAT   = clang-format
CFLAGS   = -Wall -Wpedantic -Werror -Wextra -O3 -g #-DDEBUG

.PHONY: all clean format

all: $(EXECBIN)

$(EXECBIN): $(OBJECTS)
	$(CC) -o $@ $^ -pthread

# $(EXECBIN): httpproxy.o
# 	$(CC) -o $@ $^ -pthread

build/%.o : %.c $(HEADERS)
	mkdir -p build
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(EXECBIN) $(OBJECTS)
	rm -rf build
	rm -f cache cache.o

# ---------------
# cache testing
cache: cache.o
	clang -o cache cache.c
# ---------------

nuke: clean
	rm -rf .format

format: $(FORMATS)

.format/%.c.fmt: %.c
	mkdir -p .format
	$(FORMAT) -i $<
	touch $@

.format/%.h.fmt: %.h
	mkdir -p .format
	$(FORMAT) -i $<
	touch $@
