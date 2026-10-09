
# unuc2

> Vendored third-party code (unmodified upstream) — see [`VENDORED.md`](VENDORED.md)
> for provenance, version and license.

Command-line utility and a library for unpacking UC2 archives.

Ultra Compressor II is a packer from DOS era that achieved much tighter
compression than ZIP by combining similiar files.
Original author was so kind to [publish source code](http://www.nicodevries.com/professional/)
that made the library possible.

## API

* uc2_identify – check UC2 magic
* uc2_open – initialize
* uc2_read_cdir – read dir entry
* uc2_get_tag – read tag
* uc2_finish_cdir – get archive label
* uc2_extract – decompress a file
* uc2_message – get error message
* uc2_close – free resources

See [libunuc2.h](libunuc2.h) for details.

## unuc2sea — self-extracting (.EXE) archives

`unuc2sea` unpacks `.EXE` self-extracting UC2 archives: it skips the DOS stub and
reuses the Ultra decoder from `libunuc2.c`.

```
make unuc2sea
./unuc2sea archive.exe output-dir/
```

