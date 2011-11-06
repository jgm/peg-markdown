ALL : markdown

PROGRAM=markdown
CFLAGS ?= -Wall -O3 -ansi
OBJS=markdown_parser.o markdown_output.o markdown_lib.o
PEGDIR_ORIG=peg-0.1.4
PEGDIR=peg
LEG=$(PEGDIR)/leg

$(PEGDIR):
	cp -r $(PEGDIR_ORIG) $(PEGDIR) ; \
	patch -p1 < peg-memory-fix.patch

$(LEG): $(PEGDIR)
	CC=gcc make -C $(PEGDIR)

%.o : %.c markdown_peg.h
	$(CC) -c `pkg-config --cflags glib-2.0` $(CFLAGS) -o $@ $<

$(PROGRAM) : markdown.c $(OBJS)
	$(CC) `pkg-config --cflags glib-2.0` `pkg-config --libs glib-2.0` $(CFLAGS) -o $@ $(OBJS) $<

markdown_parser.c : markdown_parser.leg $(LEG) markdown_peg.h parsing_functions.c utility_functions.c
	$(LEG) -o $@ $<

.PHONY: clean test

clean:
	rm -f markdown_parser.c $(PROGRAM) $(OBJS)

distclean: clean
	rm -rf $(PEGDIR)

test: $(PROGRAM)
	cd MarkdownTest_1.0.3; \
	./MarkdownTest.pl --script=../$(PROGRAM) --tidy

leak-check: $(PROGRAM)
	valgrind --leak-check=full ./markdown README

