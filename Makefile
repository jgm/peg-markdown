ALL : markdown

PROGRAM=markdown
MYGETOPTDIR=my_getopt-1.5
OBJS=$(MYGETOPTDIR)/my_getopt.o bufopen.o markdown_parser.o markdown_output.o markdown_lib.o
PEGDIR=peg-0.1.4
LEG=$(PEGDIR)/leg

$(LEG):
	make -C $(PEGDIR)

%.o : %.c markdown_peg.h bufopen.h
	$(CC) -c -o $@ $<

markdown : markdown.c $(OBJS)
	$(CC) -Wall -O3 -ansi -o $@ $(OBJS) $<

markdown_parser.c : markdown_parser.leg $(LEG) markdown_peg.h
	$(LEG) -o $@ $<

.PHONY: clean test

clean:
	rm -f markdown_parser.c $(PROGRAM) $(OBJS); \
	make -C $(PEGDIR) clean

test: $(PROGRAM)
	cd MarkdownTest_1.0.3; \
	./MarkdownTest.pl --script=../$(PROGRAM) --tidy

