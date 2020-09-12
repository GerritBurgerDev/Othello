COMPILER ?= mpicc

CFLAGS ?= -O2 -Wall -Wno-variadic-macros -pedantic $(GCC_SUPPFLAGS)
LDFLAGS ?= -g 
LDLIBS =
DEBUG = -DDEBUG

SOURCE_P1 = src/final.c
SOURCE_P2 = src/LOL.c
SRCS=$(wildcard src/*.c)
OBJS=$(SRCS:src/%.c=player/%.o)

all:
	$(COMPILER) $(CFLAGS) $(SOURCE_P1) -o player/random
	$(COMPILER) $(CFLAGS) $(SOURCE_P2) -o player/lol

release: $(OBJS)
	$(COMPILER) $(LDFLAGS) -o $(EXECUTABLE) $(OBJS) $(LDLIBS) 

player/%.o: src/%.c
	$(COMPILER) $(CFLAGS) -o $@ -c $<

clean:
	rm -f player/*.o
	rm ${EXECUTABLE}

debug:
	$(COMPILER) $(DEBUG) $(CFLAGS) $(SOURCE_P1) -o player/random
	$(COMPILER) $(DEBUG) $(CFLAGS) $(SOURCE_P2) -o player/lol
