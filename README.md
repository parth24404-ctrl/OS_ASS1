# OS Assignment 1 — SimpleLoader & Bootloader

A 32-bit ELF loader written in plain C (no ELF library APIs), and a 16-bit x86
boot sector run under QEMU.

```
.
├── part-a/
│   ├── without-bonus/        standalone loader (loader.c has main())
│   │   ├── loader.c  loader.h  fib.c  Makefile
│   └── with-bonus/           shared-library loader + separate launcher
│       ├── Makefile          top-level: builds all sub-directories
│       ├── loader/           loader.c, loader.h  -> bin/lib_simpleloader.so
│       ├── launcher/         launch.c            -> bin/launch
│       ├── test/             factorial.c         -> test/factorial
│       └── bin/              created by make (lib_simpleloader.so, launch)
├── part-b/
│   ├── boot.asm              the completed boot sector
│   ├── boot.bin              assembled 512-byte raw image
│   ├── boot_qemu_screenshot.png
│   ├── report.md             Part B write-up
│   └── starter/              the original fill-in-the-blanks file
├── design_document.md        Part A design document
└── README.md
```

## Prerequisites

```bash
sudo apt install build-essential gcc-multilib   # 32-bit compilation
sudo apt install nasm qemu-system-x86           # Part B
```

`gcc-multilib` provides the 32-bit libc headers and start files that `-m32`
needs. If you cannot install it (no root), build a local i386 sysroot and
point the Makefiles at it — every Makefile here auto-detects
`$HOME/local/i386` and falls back to plain `-m32` when it is absent. You can
also override it explicitly:

```bash
make SYSROOT=/path/to/i386 GCCVER=13
```

## Part A — Running the loader

### without-bonus

```bash
cd part-a/without-bonus
make
./loader ./fib
# User _start return value = 102334155
```

### with-bonus (shared library + launcher)

```bash
cd part-a/with-bonus
make                       # builds bin/lib_simpleloader.so, bin/launch, test/factorial
./bin/launch test/factorial
# User _start return value = 120
make clean
```

The top-level Makefile runs `make` in `loader/`, `launcher/` and `test/` in
that order. `lib_simpleloader.so` and `launch` land in `bin/`, while
`factorial` stays inside `test/`, as the specification requires. `launch` is
linked with `-Wl,-rpath,'$ORIGIN'`, so it finds the shared library next to
itself and no `LD_LIBRARY_PATH` is needed. The test case is rebuilt only when
`factorial.c` is newer than its executable.

The loader and launcher never need recompiling to run a different executable:

```bash
./bin/launch /path/to/any/32-bit/static/executable
```

## Part B — Running the bootloader

```bash
cd part-b
nasm -f bin boot.asm -o boot.bin
qemu-system-i386 -drive format=raw,file=boot.bin
```

QEMU opens a window showing:

```
Booting from Hard Disk...
Hello from my bootloader!
```

Useful variants:

```bash
qemu-system-i386 -drive format=raw,file=boot.bin -monitor stdio  # QEMU monitor
qemu-system-i386 -drive format=raw,file=boot.bin -s -S           # gdb on :1234
```

See `part-b/report.md` for the write-up (512-byte limit, 0x7C00, the 0x55AA
signature, and QEMU's role in the boot process).

## Notes

- All C files for Part A are compiled with `-m32`; `factorial.c` and `fib.c`
  are compiled with `-m32 -no-pie -nostdlib` and are **not** modified.
- `loader.h` is unmodified, as required.
- Error checking covers: file open/read failures, short reads, ELF magic,
  32-bit class, endianness, `ET_EXEC` type, i386 machine, presence and size of
  the program header table, PHDR table and segment bounds inside the file,
  `p_filesz <= p_memsz`, `mmap` failure, and a missing entrypoint segment.
  Every failure path runs the cleanup routine before exiting.
