uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifneq (,$(findstring MINGW,$(uname_S)))
	X = .exe
endif

export X

PROGRAM=markdown$(X)
CFLAGS ?=-Wall -O2 -pipe -g -Wno-unused-label -Wno-unused-function
OBJS=markdown_parser.o markdown_output.o markdown_lib.o
PKG_CONFIG=pkg-config
LEG=leg
CC=gcc
LIBRARY=libpegmarkdown
VERSION=1.0.0
SHORT_VERSION=1

ALL : $(LIBRARY) $(PROGRAM)

%.o : %.c markdown_peg.h
	$(CC) -fPIC -c `$(PKG_CONFIG) --cflags glib-2.0` $(CFLAGS) -o $@ $<

$(PROGRAM) : markdown.c $(OBJS)
	$(CC) `$(PKG_CONFIG) --cflags glib-2.0` $(CFLAGS) -o $@ $< `$(PKG_CONFIG) --libs glib-2.0` -L. -lpegmarkdown

$(LIBRARY) : markdown_parser.o $(OBJS)
	$(CC) `$(PKG_CONFIG) --cflags glib-2.0` $(CFLAGS) -shared -Wl,-soname,$(LIBRARY).so.$(SHORT_VERSION) -o $(LIBRARY).so.$(VERSION) $(OBJS) `$(PKG_CONFIG) --libs glib-2.0`
	ln -s $(LIBRARY).so.$(VERSION) $(LIBRARY).so.$(SHORT_VERSION)
	ln -s $(LIBRARY).so.$(SHORT_VERSION) $(LIBRARY).so

markdown_parser.o : markdown_parser.c
	$(CC) -fPIC -c `$(PKG_CONFIG) --cflags glib-2.0` $(CFLAGS) -o $@ $<

markdown_parser.c : markdown_parser.leg markdown_peg.h parsing_functions.c utility_functions.c
	$(LEG) -o $@ $<

.PHONY: clean test

clean:
	rm -f markdown_parser.c $(PROGRAM) $(OBJS) *.so.*

test: $(PROGRAM)
	cd MarkdownTest_1.0.3; \
	./MarkdownTest.pl --script=../$(PROGRAM) --tidy

leak-check: $(PROGRAM)
	valgrind --leak-check=full ./markdown README
