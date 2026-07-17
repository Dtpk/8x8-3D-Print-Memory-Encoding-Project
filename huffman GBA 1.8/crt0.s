@ Minimal GBA crt0
.section .text.start
.arm
.global _start
_start:
    b reset
    .space 156 @ Logo space
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .byte 0x00 @ Fixed
    .space 16 @ Other header fields

reset:
    @ Simple reset: disable interrupts and jump to main
    mov r0, #0x04000000
    add r1, r0, #0x200
    mov r2, #0
    strh r2, [r1, #8] @ REG_IME = 0 (0x04000208)
    
    ldr sp, =0x03007F00 @ Set stack pointer to end of IWRAM
    
    ldr r0, =main
    bx r0
