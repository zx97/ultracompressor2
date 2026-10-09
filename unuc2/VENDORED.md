# Third-party code — `unuc2` by Jan Bobrowski

Except for the *local additions* listed below, everything under `unuc2/` is the
**unmodified upstream** project **`unuc2`** by **Jan Bobrowski**
(<https://torinak.com/~jb/unuc2/> · <https://github.com/jan-bobrowski/unuc2>).

It is vendored **verbatim for convenience** — this is **not a fork** and these
files are **not patched** here. Any fix or improvement to them belongs upstream.

* Upstream version: **0.8**
* Upstream commit: **`6f1fc3c85591569532f7a206e6c7dba3b5366eb9`** (2022-06-20)

## Vendored verbatim (unmodified — do not edit)

The sha256 of each file is identical to the upstream 0.8 release.

| File | sha256 | License |
|---|---|---|
| `libunuc2.c` | `4b25386d26d545d8edefa5fbaa19bb8be9ac473cc807d6b5d46e6fcd84f53e61` | GNU LGPL v3 |
| `libunuc2.h` | `d1b540318e5d97f4e8ca59ef676ca39de90dee7ce07ef4044a8437eb4e9fb928` | GNU LGPL v3 |
| `unuc2.c`    | `cfacd6c6b2eaa468ebf5408d706b11287efe8f343cd18ba551cf0adb0fb63c7f` | GNU GPL v3 |
| `list.h`     | `e9cc50cf49b028ccbaf67dfcb427e168db38ab88d5fba0dfa871057e10952bd9` | GNU GPL v3 |
| `super.bin`  | `228b57ec73e600d46f77e787494f170dac8ce5c737912824cc66e7e08dbf1f49` | data (part of unuc2) |
| `misc/`      | `mc.ext`, `uuc2` (verbatim) | shipped upstream |

## Local additions (part of this port — © Manuel FLURY, LGPL v3)

| File | Notes |
|---|---|
| `unuc2sea.c` | extracts self-extracting `.EXE` archives; reuses `libunuc2.c` |
| `Makefile`   | upstream build **plus** a local `unuc2sea` target |
| `README.md`  | upstream README **plus** a short `unuc2sea` section |

## Refreshing

Download unuc2 0.8 from the upstream URL above and replace the *vendored
verbatim* files; the local additions do not need to change.
