VERSIONCMD = git describe --dirty --tags --always 2> /dev/null
VERSION := $(shell $(VERSIONCMD) || cat VERSION)

PREFIX ?= /usr/local
BINPREFIX ?= $(PREFIX)/bin
MANPREFIX ?= $(PREFIX)/share/man

CPPFLAGS += -D_DEFAULT_SOURCE -DVERSION=\"$(VERSION)\"
CFLAGS += -std=c99 -pedantic -Wall -Wextra

OBJ = iniq.o inih/ini.o
MANPAGE = iniq.1

all: iniq $(MANPAGE)

iniq: $(OBJ)

$(MANPAGE): man/$(MANPAGE).pod
	pod2man -n=iniq -c=iniq -s=1 -r=$(VERSION) $< $(MANPAGE)

install-iniq:
	mkdir -p $(DESTDIR)$(BINPREFIX)
	cp -p iniq $(DESTDIR)$(BINPREFIX)

install: install-iniq
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	cp -p $(MANPAGE) $(DESTDIR)$(MANPREFIX)/man1

uninstall:
	rm -f $(DESTDIR)$(BINPREFIX)/iniq
	rm -f $(DESTDIR)$(MANPREFIX)/man1/iniq.1

clean:
	rm -f iniq $(OBJ) $(MANPAGE)

test: iniq
	$(MAKE) -C test

.PHONY: all install-iniq install uninstall clean test
