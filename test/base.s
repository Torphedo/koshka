// This isn't a coherent program, it just has every base instruction to test
// the decoder.

main:
    adc w1, w2, w3
    adcs w1, w2, w3

    add w1, w2, w3 // TODO: Figure out syntax for ADD (extended register)
    add w1, w2, #42
    add w1, w2, w3, lsl 4 // TODO: Figure out syntax for ADD (shifted register)

    adds w1, w2, w3 // TODO: Figure out syntax for ADDS (extended register)
    adds w1, w2, #42
    adds w1, w2, w3, lsl 4 // TODO: Figure out syntax for ADDS (shifted register)

    adr x1, main
    adrp x1, main

    and x1, x2, #64

