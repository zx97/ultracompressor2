# UC2 (UltraCompressor II) — DOS -> Linux port
#
# Build with:  make
#
# This is a port of the 1994 Borland C++ / DOS sources to
# Linux (g++).  The Borland run-time has been replaced by POSIX equivalents
# in linux_inc/ (see NOTES.md).  The flat 32/64-bit memory model means the
# old far/huge/near/interrupt keywords and 80386 inline assembly had to be
# translated to C (see NOTES.md for status of each module).

CXX      = g++
CC       = gcc
CXXFLAGS = -include linux_inc/portdefs.h -Ilinux_inc -I. -fpermissive -O2 -w -DNDEBUG
LIBS     =

# --- Sources that already compile cleanly / nearly -------------------------
# (Drop the Borland RTL sources, the external-spawn lib, duplicate memory
#  manager, the code-fragment, and the two big inline-asm engine modules
#  which are being translated separately: nuke.cpp, ultracmp.cpp)
SRCS = \
	archio.cpp bitio.cpp comoterp.cpp compint.cpp comp_tt.cpp contains.cpp \
	convert.cpp dampro.cpp debuglog.cpp delta.cpp dirman.cpp diverse.cpp exit.cpp fletch.cpp \
	handle.cpp list.cpp llio.cpp main.cpp mem.cpp menu.cpp neuroman.cpp \
	superman.cpp tagman.cpp test.cpp treeenc.cpp \
	treegen.cpp ultracmp.cpp uc2_decomp.cpp uc2sas.cpp version.cpp video.cpp views.cpp vol.cpp \
	vmem.cpp xorblock.cpp \
	linux_inc/port_rt.cpp linux_inc/conio_linux.cpp \
	linux_inc/dir_linux.cpp linux_inc/borland.cpp

OBJS = $(SRCS:.cpp=.o)

# Native self-extracting (SEA) extractor (C), reuses unuc2/libunuc2.c.
# Compiled as C (gcc) and linked into the C++ binary; main() is suppressed.
SEAOBJ = unuc2sea_c.o
SEASRC = ../unuc2/unuc2sea.c

TARGET = uc2

all: $(TARGET)

$(TARGET): $(OBJS) $(SEAOBJ)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(SEAOBJ)

$(SEAOBJ): $(SEASRC) ../unuc2/libunuc2.c ../unuc2/super.bin
	hexdump -v -e '"0x" 1/1 "%02X" ","' < ../unuc2/super.bin > sea_super.inc
	$(CC) -std=gnu99 -DNDEBUG -Os -w -I. -I../unuc2 -DSUPER_INC='"sea_super.inc"' -c $(SEASRC) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(SEAOBJ) $(TARGET) sea_super.inc
