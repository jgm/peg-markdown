uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifneq (,$(findstring MINGW,$(uname_S)))
	X = .exe
endif

export X

PROGRAM=markdown$(X)
CFLAGS ?= -Wall -O3 -ansi
OBJS=markdown_parser.o markdown_output.o markdown_lib.o
PKG_CONFIG=pkg-config
LEG=leg
CC=gcc

ALL : $(PROGRAM)

%.o : %.c markdown_peg.h
	$(CC) -c `$(PKG_CONFIG) --cflags glib-2.0` $(CFLAGS) -o $@ $<

$(PROGRAM) : markdown.c $(OBJS)
	$(CC) `$(PKG_CONFIG) --cflags glib-2.0` $(CFLAGS) -o $@ $< $(OBJS) `$(PKG_CONFIG) --libs glib-2.0`

markdown_parser.c : markdown_parser.leg markdown_peg.h parsing_functions.c utility_functions.c
	$(LEG) -o $@ $<

.PHONY: clean test

clean:
	rm -f markdown_parser.c $(PROGRAM) $(OBJS)

test: $(PROGRAM)
	cd MarkdownTest_1.0.3; \
	./MarkdownTest.pl --script=../$(PROGRAM) --tidy

leak-check: $(PROGRAM)
	valgrind --leak-check=full ./markdown README
