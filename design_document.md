# Design Document — SimpleLoader (OS Assignment 1, Part A)

**GitHub repository (private):** `https://github.com/<your-username>/<repo-name>`

---

## 1. Objective

Implement a loader that takes a statically linked, non-PIE, 32-bit ELF
executable, places its code in memory by hand, and transfers control to its
entrypoint — using only `open`, `read`, `lseek`, `malloc`, `mmap` and
`munmap`, with no ELF-manipulating library API such as libelf.

Two versions are built from the same logic:

| | without-bonus | with-bonus |
|---|---|---|
| Form | one executable, `loader.c` contains `main()` | `lib_simpleloader.so` + separate `launch` driver |
| Entry | `./loader ./fib` | `./bin/launch test/factorial` |

## 2. Background: the two views of an ELF file

An ELF file has a *linkable view* (section headers, used by the linker) and an
*execution view* (program headers, used by the loader). A loader only needs
the execution view, which is why this implementation never touches the section
header table.

- The **ELF header** (`Elf32_Ehdr`) sits at offset 0 and is the roadmap to the
  rest of the file. The fields that matter here are `e_ident` (magic + class +
  endianness), `e_type`, `e_machine`, `e_entry` (virtual address of `_start`),
  `e_phoff` (file offset of the program header table), `e_phentsize` and
  `e_phnum`.
- Each **program header** (`Elf32_Phdr`) describes one segment. For a `PT_LOAD`
  segment the loader needs `p_offset` (where the bytes are in the file),
  `p_vaddr` (where they belong in memory), `p_filesz` (how many bytes exist in
  the file) and `p_memsz` (how much memory the segment occupies — the extra
  `p_memsz - p_filesz` bytes are `.bss` and must read as zero).

## 3. Control flow

```
launch.c (or main() in loader.c)
      │  argv[1] = path of the ELF executable
      │  sanity-check the file (magic, 32-bit, ET_EXEC, i386, has PHDRs)
      ▼
load_and_run_elf(&argv[1])
      │  1. open()  + lseek(SEEK_END) to size the file
      │  2. malloc(file_size) and read() the whole binary into the heap
      │  3. validate the ELF header
      │  4. walk the PHDR table -> the PT_LOAD segment containing e_entry
      │  5. mmap(p_memsz, RWX, PRIVATE|ANONYMOUS) and copy p_filesz bytes
      │  6. entry = segment + (e_entry - p_vaddr); cast to int(*)(void)
      │  7. call it and print the return value
      ▼
loader_cleanup()      munmap the segment, free the buffer, close the fd
```

## 4. Design decisions

**Reading the whole binary into the heap.** The specification asks for the
binary to be copied into `malloc`'d memory. This also simplifies everything
downstream: once the file is in one contiguous buffer, `ehdr` is just a cast of
the buffer's base and `phdr` is `buffer + e_phoff`, so the program header table
is walked with ordinary pointer arithmetic instead of repeated `lseek`/`read`
pairs. It also makes bounds checking easy, since the file size is known.

**Finding the right segment.** There can be several `PT_LOAD` segments (text,
rodata, data). The correct one is the segment whose virtual address range
contains the entrypoint:

```c
if (p_type == PT_LOAD && e_entry >= p_vaddr && e_entry < p_vaddr + p_memsz)
```

Using `p_memsz` rather than `p_filesz` for the upper bound is deliberate — the
range a segment *occupies in memory* is the one the entrypoint must fall in.

**Mapping and copying.** The segment is mapped anonymously with
`PROT_READ | PROT_WRITE | PROT_EXEC` and `MAP_PRIVATE | MAP_ANONYMOUS`:
execute permission because we are about to jump into it, write permission
because we must copy the bytes in first, and anonymous because the mapping is
backed by nothing on disk — we fill it ourselves.

Exactly `p_filesz` bytes are copied. The remaining `p_memsz - p_filesz` bytes
are the `.bss` and are explicitly zeroed. Copying `p_memsz` bytes straight
from the file instead would be a bug: it reads past the end of the segment's
file image, pulling in whatever follows it (the next segment, section headers,
symbol table) and placing that garbage where zero-initialised globals should
be. It also silently short-reads near the end of the file. An anonymous
mapping is already zero filled by the kernel, so the explicit `memset` is
defensive, but it documents the intent and survives a change of mapping type.

**Reaching the entrypoint.** `e_entry` is a virtual address expressed in the
executable's own address space, while `mmap` returned an arbitrary address in
ours. The offset of the entrypoint within the segment is invariant, so:

```c
void *entry_ptr = (char *)segment + (e_entry - target->p_vaddr);
int (*_start)(void) = (int (*)(void))entry_ptr;
int result = _start();
```

The `_start` in `fib.c` / `factorial.c` takes no arguments and returns `int`,
so the function pointer type matches and the call uses the ordinary cdecl
convention on our existing stack.

**Why only statically linked, non-PIE executables work.** The loaded code is
executed at a different address than the one it was linked for, so any
absolute address baked into it would be wrong — this is why `-no-pie` matters
(a fixed load address and no relocation entries to process) and why
`-nostdlib` matters (no dynamic linker, no PLT/GOT to resolve, no libc
initialisation). A real loader would additionally map *every* `PT_LOAD`
segment at its own `p_vaddr` with `MAP_FIXED`, honour each segment's
`p_flags` for permissions, process relocations, and set up a fresh stack with
`argc`/`argv`/`envp`.

## 5. Error handling

Every failure is reported and then routed through `loader_cleanup()` before
`exit()`, so no descriptor or mapping is leaked on any path.

| Check | Failure detected |
|---|---|
| `open` returns < 0 | file missing or unreadable |
| file size < `sizeof(Elf32_Ehdr)` | truncated / not an ELF |
| `read` loop total != file size | short read or I/O error |
| `e_ident[0..3]` vs `\x7fELF` | not an ELF file |
| `EI_CLASS != ELFCLASS32` | 64-bit binary (missing `-m32`) |
| `EI_DATA != ELFDATA2LSB` | wrong endianness |
| `e_type != ET_EXEC` | PIE or shared object (missing `-no-pie`) |
| `e_machine != EM_386` | wrong architecture |
| `e_phoff == 0` or `e_phnum == 0` | no program header table |
| `e_phentsize != sizeof(Elf32_Phdr)` | malformed header |
| PHDR table / segment extends past EOF | corrupt or truncated file |
| `p_filesz > p_memsz` | corrupt program header |
| no `PT_LOAD` contains `e_entry` | not a loadable executable |
| `mmap` returns `MAP_FAILED` | out of memory / bad size |

`launch.c` repeats the cheap header checks before calling the library, so a
bad input is rejected by the launcher rather than inside the loader.

## 6. Build system (bonus)

```
with-bonus/
├── Makefile          mkdir bin, then make -C loader, launcher, test
├── loader/Makefile   gcc -m32 -fPIC -shared -o ../bin/lib_simpleloader.so loader.c
├── launcher/Makefile gcc -m32 -I../loader -o ../bin/launch launch.c \
│                         -L../bin -l_simpleloader -Wl,-rpath,'$ORIGIN'
└── test/Makefile     gcc -m32 -no-pie -nostdlib -o factorial factorial.c
```

- `-fPIC -shared` produces the position-independent shared object.
- `-l_simpleloader` resolves to `lib_simpleloader.so` (gcc adds the `lib`
  prefix and `.so` suffix), found via `-L../bin`.
- `-Wl,-rpath,'$ORIGIN'` records a load path relative to the `launch` binary
  itself, so `./bin/launch` runs without setting `LD_LIBRARY_PATH`.
- `test/Makefile` uses a real prerequisite (`factorial: factorial.c`), so the
  test case is rebuilt only when the executable is missing or the source has
  changed — while `launch` and the library are never rebuilt just to run a
  different executable.
- Each directory has its own `clean` target; the top-level `clean` invokes
  them all and removes `bin/`.

## 7. Testing

| Input | Result |
|---|---|
| `./bin/launch test/factorial` | `User _start return value = 120` (5! = 120) |
| `./loader ./fib` | `User _start return value = 102334155` (fib(40)) |
| `./bin/launch` (no argument) | usage message, exit 1 |
| `./bin/launch /bin/ls` | rejected: not a 32-bit ELF file |
| `./bin/launch nosuchfile` | rejected: `open: No such file or directory` |
| `valgrind ./loader ./fib` | no leaks from the loader's own allocations |
