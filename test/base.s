// This isn't a coherent program, it uses all the base ARM instructions to test
// the decoder.
.global _start
_start:
    adc x1, x2, x3
    adcs x1, x2, x3

    add x1, x2, x3 // TODO: Figure out syntax for ADD (extended register)
    add x1, x2, #42
    add x1, x2, x3, lsl 4 // TODO: Figure out syntax for ADD (shifted register)

    adds x1, x2, x3 // TODO: Figure out syntax for ADDS (extended register)
    adds x1, x2, #42
    adds x1, x2, x3, lsl 4 // TODO: Figure out syntax for ADDS (shifted register)

    adr x1, _start
    adrp x1, _start

    and x1, x2, #128
    and x1, x2, x1, lsl 4
    ands x1, x2, #64
    ands x1, x2, x1, lsl 4

    // ASR w/ register is an alias of ASRV
    asr x1, x2, x3
    // ASR w/ immediate is an alias of SBFM
    asr x1, x2, #4
    asrv x1, x2, x3

