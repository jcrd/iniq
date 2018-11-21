VERSION := $(shell git describe --dirty --tags --always || cat VERSION)

PREFIX ?= /usr/local
BINPREFIX ?= $(PREFIX)/bin
MANPREFIX ?= $(PREFIX)/share/man

CPPFLAGS += -D_DEFAULT_SOURCE -DVERSION=\"$(VERSION)\"
CFLAGS += -std=c99 -pedantic -Wall -Wextra

OBJ = iniq.o inih/ini.o
MANPAGE = man/iniq.1

all: iniq $(MANPAGE)

iniq: $(OBJ)

$(MANPAGE): $(MANPAGE).pod
	pod2man -n=iniq -c=iniq -s=1 -r=$(VERSION) $< $(MANPAGE)

install:
	mkdir -p $(DESTDIR)$(BINPREFIX)
	cp -p iniq $(DESTDIR)$(BINPREFIX)
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	cp -p $(MANPAGE) $(DESTDIR)$(MANPREFIX)/man1

uninstall:
	rm -f $(DESTDIR)$(BINPREFIX)/iniq
	rm -f $(DESTDIR)$(MANPREFIX)/man1/iniq.1

clean:
	rm -f iniq $(OBJ) $(MANPAGE)

test: iniq
	$(MAKE) -C test

.PHONY: all install uninstall clean test
