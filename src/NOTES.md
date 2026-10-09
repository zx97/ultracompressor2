# UC2 DOS → Linux Port — NOTES

UltraCompressor II (UC2) was a 1994 Borland C++ / DOS application. This
directory contains a **port to Linux (g++)**. The Borland / real-mode run-time
has been replaced, the performance-critical 80386 assembly routines have been
rewritten in C, and the compression/decompression engine works end-to-end
(create, list, test, extract — verified by round-trip).

## Build
```
make            # builds ./uc2 (ncurses required: -lncurses)
```
The build produces `./uc2`; `make test` (from the repository root) runs a
round-trip smoke test.

## Multi-volume (SAS) format

`src/uc2sas.cpp` reproduces the piece format of the original **SAS.EXE**
("simple archive splitter") shipped with **UC PRO v2.3** (`uc2pro.exe`, free for
individual use): <http://www.bttr-software.de/freesoft/arc1.htm>. The 14-byte
piece header was reverse-engineered from that binary for interoperability;
`uc2 split` / `uc2 join` are byte-interchangeable with SAS.EXE in both
directions.
