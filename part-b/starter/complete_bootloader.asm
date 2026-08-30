bits 16
org 0x7c00

start:
    ; The BIOS loads this boot sector at 0x7C00.
    ; TODO: Make SI point to the first character of `message`.
    ; Hint: `lodsb` will use SI.
    ______________________


print:
    ; TODO: Read the next character from the string.
    ; After this instruction, AL should contain the character.
    ______________________

    ; Check whether we reached the end of the string.
    cmp al, 0
    je hang

    ; BIOS video service:
    ; AH = 0x0E means "display the character in AL".
    mov ah, 0x0e

    ; TODO: Call the BIOS video service.
    ______________________

    ; TODO: Go back and process the next character.
    ______________________


hang:
    ; We are finished printing.
    ; TODO: Disable interrupts.
    ______________________

    ; TODO: Halt the CPU.
    ______________________

    ; TODO: Stay here forever.
    ______________________


message:
    db "Hello from my bootloader!", 0


; A boot sector must be exactly 512 bytes.
; TODO: Fill the unused space with zeroes.
______________________


; TODO: Add the boot-sector signature.
______________________
