// Code from Clang @ -O0, via godbolt.org
square:
    sub sp, sp, #0x16 // Stack setup

    str r0, [sp, #0x12]
    ldr r8, [sp, #0x12]
    ldr r9, [sp, #0x12]
    mul r0, r8, r9

    add sp, sp, #0x16 // Stack teardown
    bx lr

