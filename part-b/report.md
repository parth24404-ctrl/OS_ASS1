# Part B — A Minimal x86 Bootloader

**Files:** `boot.asm` (source), `boot.bin` (512-byte raw image),
`boot_qemu_screenshot.png` (execution screenshot),
`starter/complete_bootloader.asm` (the original fill-in-the-blanks file).

## 1. Build and run

```bash
sudo apt install nasm qemu-system-x86      # one-time setup
nasm -f bin boot.asm -o boot.bin           # assemble to a raw 512-byte image
qemu-system-i386 -drive format=raw,file=boot.bin
```

`-f bin` makes NASM emit a *flat* binary — no ELF header, no relocations, no
symbol table, just the raw bytes the BIOS expects in a boot sector. QEMU is
told to treat the file as a raw disk so that its first sector *is* our code.

## 2. Implementation

The program runs in 16-bit real mode with no operating system underneath it,
so the only services available are the BIOS interrupts. The blanks in the
starter file were filled as follows.

| Blank | Filled with | Why |
|---|---|---|
| point SI at the string | `mov si, message` | `lodsb` reads from `DS:SI` |
| read the next character | `lodsb` | loads `[DS:SI]` into `AL` and increments `SI` |
| call the BIOS video service | `int 0x10` | with `AH=0x0E`, teletype output of `AL` |
| loop | `jmp print` | process the next character |
| disable interrupts | `cli` | nothing left to service |
| halt the CPU | `hlt` | park the core instead of spinning |
| stay here forever | `jmp hang` | safety net if an NMI resumes execution |
| pad the sector | `times 510 - ($ - $$) db 0` | fill up to offset 510 |
| boot signature | `dw 0xaa55` | writes the bytes `55 AA` at 510–511 |

Two directives make the rest work:

- `bits 16` — the CPU begins execution in 16-bit real mode, so NASM must emit
  16-bit instruction encodings.
- `org 0x7c00` — the BIOS loads the sector at `0x0000:0x7C00`, so every label
  must be assembled relative to that base. Without it, `mov si, message`
  would load an offset of ~`0x2A` instead of ~`0x7C2A` and the CPU would print
  whatever bytes happen to live in the interrupt vector table.

The padding expression is worth spelling out: `$` is the address of the
current line and `$$` the address of the start of the section, so `$ - $$` is
the number of bytes emitted so far. `times 510 - ($ - $$) db 0` therefore
emits exactly enough zero bytes to reach offset 510. Because x86 is
little-endian, `dw 0xaa55` stores `55` at offset 510 and `AA` at 511 — which
is what the BIOS actually looks for — giving a file of exactly 512 bytes.

Verifying the image:

```
$ ls -l boot.bin
-rw-r--r-- 1 mukul mukul 512 boot.bin
$ xxd boot.bin | tail -1
000001f0: 0000 0000 0000 0000 0000 0000 0000 55aa
```

## 3. Questions

### Why is the bootloader limited to 512 bytes?

Because 512 bytes is the size of one disk sector, and the BIOS reads exactly
**one sector** — the first one, the Master Boot Record — before handing over
control. This dates back to the original IBM PC: the firmware ROM was tiny and
had to make the simplest possible assumption about what a bootable disk looks
like. Two of those bytes are consumed by the signature, so at most **510
bytes** are available for code and data (on a partitioned disk the 64-byte
partition table takes another chunk). This is why real bootloaders such as
GRUB are *chained*: the 512-byte stage 1 does nothing but use `INT 13h` to
load a much larger stage 2 from disk and jump into it.

### Why is 0x7C00 significant?

It is the fixed physical address at which the BIOS places the boot sector
before jumping to it, so it is the address our code must be assembled for.
The value is an artefact of the IBM PC 5150, whose smallest configuration had
32 KiB of RAM (`0x0000`–`0x8000`). The designers wanted the boot sector as
high in memory as possible so the loaded OS would get a contiguous run of low
memory, but they had to leave room at the top for the sector itself plus its
stack and data: `0x8000 − 512 (sector) − 512 (stack/data) = 0x7C00`. It also
sits safely above the interrupt vector table (`0x0000`–`0x03FF`) and the BIOS
data area (`0x0400`–`0x04FF`). Every BIOS since has kept the convention.

### What does the 0x55AA boot signature represent?

It is the magic number that marks a sector as bootable. After loading the
first sector, the BIOS checks its last two bytes: offset 510 must be `0x55`
and offset 511 must be `0xAA` (written as the little-endian word `0xAA55`). If
they do not match, the BIOS treats the disk as non-bootable and moves to the
next boot device, or reports something like "No bootable device". It is a
sanity check, not a checksum — it only prevents the BIOS from jumping into a
blank or non-boot disk, and says nothing about whether the code is valid.

### How is QEMU involved in the boot process?

QEMU is a full-system emulator: it emulates an x86 CPU, RAM, a disk
controller and a display, and runs a real firmware image (SeaBIOS) inside that
virtual machine. `-drive format=raw,file=boot.bin` presents our file to the
guest as a raw disk whose sector 0 is our code. SeaBIOS then performs exactly
the sequence a physical PC would: POST, enumerate boot devices, read sector 0
into memory at `0x7C00`, verify the `0x55AA` signature, and jump to it with
the CPU in 16-bit real mode. Our `INT 10h` calls are serviced by the emulated
BIOS and the characters appear in QEMU's virtual VGA window — which is why the
screenshot shows the SeaBIOS banner and "Booting from Hard Disk..." above our
own output.

The practical value is the edit–assemble–test loop: the bootloader can be
tested in seconds without writing a USB stick or rebooting real hardware, and
a crash kills only the emulator. QEMU also offers debugging a real machine
cannot — `-monitor stdio` for register/memory inspection and screen dumps, and
`-s -S` to attach GDB and single-step the boot sector.

```bash
qemu-system-i386 -drive format=raw,file=boot.bin -monitor stdio   # monitor console
qemu-system-i386 -drive format=raw,file=boot.bin -s -S            # wait for gdb on :1234
```

## 4. Result

`boot_qemu_screenshot.png` shows the QEMU window after boot:

```
SeaBIOS (version 1.16.3-debian-1.16.3-2)
iPXE (https://ipxe.org) ...

Booting from Hard Disk...
Hello from my bootloader!
```
