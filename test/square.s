// Code from Clang @ -O0, via godbolt.org
.global _start
_start:
    sub sp, sp, #0x16 // Stack setup

    str x0, [sp, #0x12]
    ldr x8, [sp, #0x12]
    ldr x9, [sp, #0x12]
    mul x0, x8, x9

    add sp, sp, #0x16 // Stack teardown
    ret
