# UltraCompressor II — DOS → Linux port

A Linux port of the **UltraCompressor II** (UC2) DOS sources, plus small
native extractors for UC2 archives.

UC2 is a 1990s MS-DOS archiver by AIP-NL (Ad Infinitum Programs) that combined
LZ-style compression with *shared dictionaries* (masters) to beat ZIP on related
files. Its original source was later released under the LGPL by its author, Nico
de Vries. This tree ports that source to a modern 32/64-bit C++ toolchain and adds
native decoders for UC2 archives, including self-extracting `.EXE` files.

## Repository layout

```
.
├── Makefile          top-level convenience make (builds both sub-projects)
├── LICENSE           GNU LGPL v3
├── src/              the DOS → Linux port (binary: src/uc2)
│   ├── *.cpp *.h     ported sources (Borland RTL replaced by linux_inc/)
│   ├── linux_inc/    POSIX shims for the Borland run-time (conio, dos, dir, ...)
│   └── super.inc     SUPERMASTER dictionary (needed by uc2_decomp.cpp)
└── unuc2/            native (portable C) UC2 decoders
    ├── libunuc2.c/.h Ultra/UltraComp decoder library
    ├── unuc2.c       CLI: extract standard .UC2 archives
    ├── unuc2sea.c    CLI + API: extract self-extracting .EXE archives
    ├── super.bin     SUPERMASTER data used by the decoder
    ├── VENDORED.md   provenance of the upstream unuc2 code (0.8, unmodified)
    └── super.inc     generated at build time from super.bin
```

## Building

Requirements: `gcc`, `g++`, `hexdump` (util-linux) and `make`.

```bash
make            # build src/uc2 and the unuc2 / unuc2sea tools
make uc2        # only the DOS-port binary (includes the native extractor)
make tools      # only unuc2 + unuc2sea
make test       # build everything, then run an extraction smoke test
make install    # install the binaries under $(PREFIX)/bin (default /usr/local)
make dist       # build source + binary release tarballs in dist/
make clean      # clean all sub-projects
```

## Usage

### `src/uc2` — the ported archiver

```
uc2 a <files...>          add files to an archive
uc2 e <archive> [#dir]    extract an archive (type auto-detected)
uc2 x <archive> [#dir]    same as 'e'
uc2 l <archive>           list an archive
uc2 v <archive>           verbose list
uc2 t <archive>           test/repair an archive
uc2 c <archive>           convert an archive to UC2 format
uc2 split <archive> <KiB> split an archive into <archive>.P01, .P02, ...
uc2 join  <archive>       rebuild an archive from its pieces
```

The extract command auto-detects the input type: a standard `.UC2` archive, or a
self-extracting `.EXE` (its DOS stub is skipped and the embedded archive is decoded
like any other):

```bash
uc2 e archive.uc2                 # extracts into the current directory
uc2 e selfextract.exe '#out'      # extracts a self-extracting .EXE into out/
```

### `unuc2` / `unuc2sea` — standalone extractors

```bash
make -C unuc2
./unuc2/unuc2sea archive.exe output-dir/   # self-extracting .EXE
./unuc2/unuc2     archive.uc2 out/ ./out   # standard .UC2 archive
```

## Multi-volume archives

`uc2 split` / `uc2 join` implement the original **SAS** ("simple archive
splitter") format: pieces are named `archive.P01`, `.P02`, … each with a 14-byte
header, and are **byte-interchangeable** with the original `SAS.EXE` (both
directions verified against the shipped binary).

The piece format was reverse-engineered for interoperability from the original
`SAS.EXE`, which ships in the **UC PRO v2.3** package (`uc2pro.exe`, free for
individual use): <http://www.bttr-software.de/freesoft/arc1.htm> — see also
*Credits & references* below.

```bash
uc2 split archive.uc2 1440     # 1440 KiB pieces (one floppy)
uc2 join  archive.uc2          # rebuild the archive from the pieces
```

## Banners

If an archive carries an ANSI banner (`U$~BAN.TXT`), it is displayed on
extraction

## Status

* Full archiver engine: create / list / test / extract through `src/uc2`,
  verified by round-trip (compressible, incompressible and large inputs).
* Native extraction of standard `.UC2` archives and self-extracting `.EXE`
  files, verified byte-for-byte against a reference.
* Multi-volume archives (SAS-compatible `split` / `join`) and native ANSI banners.
* Free software: **no licence or registration required** (GNU LGPL v3).
  management, volume labels, video BIOS). See `src/NOTES.md`.

## Credits & references

This port would not exist without the following people and projects.

* **Original UltraCompressor II** © 1992–1996 **AIP-NL (Ad Infinitum Programs)**,
  by **Nico de Vries** together with **Danny Bezemer** and **Jan-Pieter Cornet**
  and many other contributors. The author re-released the source under the
  **GNU LGPL** (December 2015): <https://www.nicodevries.com/professional/>
* The full list of everyone who contributed to UC2 (ideas, work, support,
  testing, …) is in the `CREDITS` section of [`src/U_MANUAL.TXT`](src/U_MANUAL.TXT).
* **Native `unuc2` decoders** © **Jan Bobrowski** 2020–2022 (LGPL v3):
  <https://torinak.com/~jb/unuc2/> · <https://github.com/jan-bobrowski/unuc2>
  — vendored unmodified (v0.8), see [`unuc2/VENDORED.md`](unuc2/VENDORED.md).
* **`mem.h`** additionally carries grants to **Jean-loup Gailly** and
  **Robert Jung** (the gzip authors), with parts © 1993 **Jan-Pieter Cornet**.
* **SAS** ("simple archive splitter") ships with **UC PRO v2.3** (`uc2pro.exe`,
  free for individual use) and was the reference for the byte-compatible
  `split` / `join`. Its own source is not part of the LGPL release:
  <http://www.bttr-software.de/freesoft/arc1.htm>
* **Linux port** by **Manuel FLURY** (2026).

**License:** GNU Lesser General Public License v3 — see [`LICENSE`](LICENSE)
(<https://www.gnu.org/licenses/lgpl-3.0.html>). The port and the native
extractors are derivative works distributed under the same terms.
