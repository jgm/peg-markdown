CFLAGS = -g -Wall $(OFLAGS) $(XFLAGS) -Isrc
OFLAGS = -O3 -DNDEBUG
#OFLAGS = -pg

OBJS = tree.o compile.o

all : peg leg

peg : peg.o $(OBJS)
	$(CC) $(CFLAGS) -o $@-new peg.o $(OBJS)
	mv $@-new $@

leg : leg.o $(OBJS)
	$(CC) $(CFLAGS) -o $@-new leg.o $(OBJS)
	mv $@-new $@

ROOT	=
PREFIX	= /usr/local
BINDIR	= $(ROOT)$(PREFIX)/bin
MANDIR	= $(ROOT)$(PREFIX)/man/man1

install : $(BINDIR) $(BINDIR)/peg $(BINDIR)/leg $(MANDIR) $(MANDIR)/peg.1

$(BINDIR) :
	mkdir -p $(BINDIR)

$(BINDIR)/% : %
	cp -p $< $@
	strip $@

$(MANDIR) :
	mkdir -p $(MANDIR)

$(MANDIR)/% : src/%
	cp -p $< $@

uninstall : .FORCE
	rm -f $(BINDIR)/peg
	rm -f $(BINDIR)/leg
	rm -f $(MANDIR)/peg.1

%.o : src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

peg.o : src/peg.c src/peg.peg-c

leg.o : src/leg.c

check : check-peg check-leg

check-peg : peg.peg-c .FORCE
	diff src/peg.peg-c peg.peg-c

check-leg : leg.c .FORCE
	diff src/leg.c leg.c

peg.peg-c : src/peg.peg peg
	./peg -o $@ $<

leg.c : src/leg.leg leg
	./leg -o $@ $<

new : newpeg newleg

newpeg : peg.peg-c
	mv src/peg.peg-c src/peg.peg-c-
	mv peg.peg-c src/.

newleg : leg.c
	mv src/leg.c src/leg.c-
	mv leg.c src/.

test examples : peg leg .FORCE
	$(SHELL) -ec '(cd examples;  $(MAKE))'

clean : .FORCE
	rm -f src/*~ *~ *.o *.peg.[cd] *.leg.[cd] peg.peg-c leg.c
	$(SHELL) -ec '(cd examples;  $(MAKE) $@)'

spotless : clean .FORCE
	rm -f src/*-
	rm -rf build
	rm -f peg
	rm -f leg
	$(SHELL) -ec '(cd examples;  $(MAKE) $@)'

.FORCE :
