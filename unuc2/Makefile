# gmake

NAME = unuc2
VERSION = 0.8
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DESTDIR ?=

CFLAGS += -DNDEBUG -Os
CFLAGS += -g -Wall
$(O)libunuc2.o $(O)libunuc2.so: CFLAGS += -std=c99
$(O)unuc2.o: override CFLAGS += -std=gnu99 -DVERSION=$(VERSION)

.PHONY: all
all: $(O)unuc2$(EXE) $(O)unuc2sea$(EXE)

$(O)libunuc2.a: $(O)libunuc2.o
	$(AR) rcs $@ $^

$(O)libunuc2.so: $(I)libunuc2.c
	$(CC) -fpic -shared $(CFLAGS) $^ $(LDFLAGS) -o $@

$(O)libunuc2.o: $(I)libunuc2.c $(I)libunuc2.h $(O)super.inc $(I)Makefile $(DEPS)
	$(CC) -c $(CFLAGS) -DSUPER_INC=\"$(O)super.inc\" $< -o $@

.INTERMEDIATE: $(O)super.inc
$(O)super.inc: $(I)super.bin
	hexdump -v -e '"0x" 1/1 "%02X" ","' <$< >$@

$(O)unuc2.o: $(I)unuc2.c $(I)list.h $(I)Makefile
	$(CC) -c $(CFLAGS) $< -o $@

$(O)unuc2$(EXE): $(O)unuc2.o $(O)libunuc2.a
	$(CC) $^ -o $@ $(LDFLAGS)

# Native self-extracting (SEA) archive extractor.
# Reuses the Ultra decoder from libunuc2.c (included directly).
$(O)unuc2sea$(EXE): $(I)unuc2sea.c $(I)libunuc2.c $(O)super.inc $(I)libunuc2.h
	$(CC) -std=gnu99 -DNDEBUG -Os -g -Wall -w -DUNUC2SEA_MAIN -DSUPER_INC=\"$(O)super.inc\" $< -o $@

.PHONY: strip
strip: $(O)unuc2$(EXE)
	$(X)strip $(O)unuc2$(EXE)

.PHONY: install
install: all
	install -d "$(DESTDIR)$(BINDIR)"
	install -s -m 0755 $(O)unuc2$(EXE)    "$(DESTDIR)$(BINDIR)/unuc2"
	install -s -m 0755 $(O)unuc2sea$(EXE) "$(DESTDIR)$(BINDIR)/unuc2sea"

.PHONY: clean
clean:
	$(RM) $(O)unuc2$(EXE) $(O)unuc2.o $(O)libunuc2.a $(O)libunuc2.so $(O)libunuc2.o $(O)super.inc
	$(RM) $(O)unuc2sea$(EXE)
	@$(CLEAN)
