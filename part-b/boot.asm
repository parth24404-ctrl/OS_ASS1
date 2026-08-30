; ===========================================================================
;  boot.asm - a minimal 16-bit x86 boot sector
;
;  Assemble : nasm -f bin boot.asm -o boot.bin
;  Run      : qemu-system-i386 -drive format=raw,file=boot.bin
; ===========================================================================

bits 16                     ; the CPU starts execution in 16-bit real mode
org 0x7c00                  ; the BIOS loads the boot sector at 0x0000:0x7C00,
                            ; so every label must be relative to that address

start:
    ; The BIOS loads this boot sector at 0x7C00.
    ; Make SI point to the first character of `message`.
    ; `lodsb` will use SI.
    mov si, message


print:
    ; Read the next character from the string.
    ; AL = [DS:SI] and SI is incremented automatically.
    lodsb

    ; Check whether we reached the end of the string.
    cmp al, 0
    je hang

    ; BIOS video service:
    ; AH = 0x0E means "display the character in AL".
    mov ah, 0x0e

    ; Call the BIOS video service (teletype output).
    int 0x10

    ; Go back and process the next character.
    jmp print


hang:
    ; We are finished printing.
    ; Disable interrupts.
    cli

    ; Halt the CPU (with interrupts disabled it never wakes up again).
    hlt

    ; Stay here forever, in case an NMI ever resumes execution.
    jmp hang


message:
    db "Hello from my bootloader!", 0


; A boot sector must be exactly 512 bytes.
; Fill the unused space with zeroes.
;   $  = address of this line, $$ = address of the start of the section,
;   so ($ - $$) is the number of bytes emitted so far.
times 510 - ($ - $$) db 0


; Add the boot-sector signature.
; x86 is little endian, so this writes the bytes 55 AA at offsets 510-511.
dw 0xaa55
