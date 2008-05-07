ALL : markdown

PROGRAM=markdown
MYGETOPTDIR=my_getopt-1.5
OBJS=$(MYGETOPTDIR)/my_getopt.o markdown_parser.o markdown_output.o
PEGDIR=peg-0.1.4
LEG=$(PEGDIR)/leg

$(LEG):
	make -C $(PEGDIR)

%.o : %.c
	$(CC) -c -o $@ $<

markdown : markdown.c $(OBJS)
	$(CC) -Wall -O3 -ansi -o $@ $(OBJS) $<

markdown_parser.c : markdown_parser.leg $(LEG)
	$(LEG) -o $@ $<

.PHONY: clean test

clean:
	rm markdown_peg.c $(PROGRAM) $(OBJS); \
	make -C $(PEGDIR) clean

test: $(PROGRAM)
	cd MarkdownTest_1.0.3; \
	./MarkdownTest.pl --script=../$(PROGRAM) --tidy

