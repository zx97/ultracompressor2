# Top-level convenience Makefile for the UltraCompressor II project.
#
#   make            build the uc2 port and the unuc2 / unuc2sea tools
#   make uc2        build only the src/ port (includes the native extractor)
#   make tools      build unuc2 + unuc2sea
#   make test       build everything, then run a UC2 round-trip smoke test
#   make install    install the binaries under $(DESTDIR)$(PREFIX)/bin
#   make dist       build source + binary release tarballs in dist/
#   make clean      clean all sub-projects
#
# Prerequisites: gcc, g++, hexdump, make.

NAME    = ultracompressor2
VERSION = 1.0

PREFIX  ?= /usr/local
BINDIR  ?= $(PREFIX)/bin
DESTDIR ?=

DISTDIR     = dist
FULLVERSION = $(NAME)-$(VERSION)
BINTARBALL  = $(NAME)-$(VERSION)-linux-x86_64

.PHONY: all uc2 tools test install dist clean

all: uc2 tools

uc2:
	$(MAKE) -C src

tools:
	$(MAKE) -C unuc2 all

test: all
	@echo "== UC2 round-trip smoke test =="
	@tmp=$$(mktemp -d) ; \
	 ( cd "$$tmp" && printf 'hello ultracompressor\n' > in.txt && \
	   "$(CURDIR)/src/uc2" a t.uc2 in.txt >/dev/null && \
	   mkdir out && cd out && "$(CURDIR)/src/uc2" e ../t.uc2 >/dev/null && \
	   cmp ../in.txt in.txt && echo "OK: UC2 round-trip matches" ) ; \
	 rm -rf "$$tmp"

install: all
	install -d "$(DESTDIR)$(BINDIR)"
	install -s -m 0755 src/uc2        "$(DESTDIR)$(BINDIR)/uc2"
	install -s -m 0755 unuc2/unuc2    "$(DESTDIR)$(BINDIR)/unuc2"
	install -s -m 0755 unuc2/unuc2sea "$(DESTDIR)$(BINDIR)/unuc2sea"

# Release tarballs written to dist/: a source tarball (tracked files only) and
# a prebuilt binary tarball (stripped binaries + README + LICENSE).
dist: all
	@mkdir -p "$(DISTDIR)"
	git archive --format=tar --prefix="$(FULLVERSION)/" HEAD | xz -9 > "$(DISTDIR)/$(FULLVERSION).tar.xz"
	rm -rf "$(DISTDIR)/$(BINTARBALL)"
	install -d "$(DISTDIR)/$(BINTARBALL)/bin"
	install -s -m 0755 src/uc2        "$(DISTDIR)/$(BINTARBALL)/bin/uc2"
	install -s -m 0755 unuc2/unuc2    "$(DISTDIR)/$(BINTARBALL)/bin/unuc2"
	install -s -m 0755 unuc2/unuc2sea "$(DISTDIR)/$(BINTARBALL)/bin/unuc2sea"
	cp README.md LICENSE "$(DISTDIR)/$(BINTARBALL)/"
	printf 'UltraCompressor II (uc2) - prebuilt Linux x86-64 binaries\n\nBuilt for x86-64 Linux (glibc); runtime: libc6, libstdc++6.\nBinaries in bin/: uc2, unuc2, unuc2sea. Usage in README.md; terms in LICENSE (GNU LGPL v3).\n' > "$(DISTDIR)/$(BINTARBALL)/README.release"
	tar -C "$(DISTDIR)" -cJf "$(DISTDIR)/$(BINTARBALL).tar.xz" "$(BINTARBALL)"
	rm -rf "$(DISTDIR)/$(BINTARBALL)"
	@echo "== artifacts ==" && ls -l "$(DISTDIR)"

clean:
	$(MAKE) -C src clean
	$(MAKE) -C unuc2 clean
