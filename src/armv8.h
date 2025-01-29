#ifndef ARMV8_H
#define ARMV8_H

#include <common/int.h>

typedef struct {
    u64 sp;
    u64 pc;

    // General purpose registers
    u64 r0;
    u64 r1;
    u64 r2;
    u64 r3;
    u64 r4;
    u64 r5;
    u64 r6;
    u64 r7;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;
    u64 r16;
    u64 r17;
    u64 r18;
    u64 r19;
    u64 r20;
    u64 r21;
    u64 r22;
    u64 r23;
    u64 r24;
    u64 r25;
    u64 r26;
    u64 r27;
    u64 r28;
    u64 r29;
    u64 r30;

    // 128-bit SIMD / floating-point registers
    u64 v0[2];
    u64 v1[2];
    u64 v2[2];
    u64 v3[2];
    u64 v4[2];
    u64 v5[2];
    u64 v6[2];
    u64 v7[2];
    u64 v8[2];
    u64 v9[2];
    u64 v10[2];
    u64 v11[2];
    u64 v12[2];
    u64 v13[2];
    u64 v14[2];
    u64 v15[2];
    u64 v16[2];
    u64 v17[2];
    u64 v18[2];
    u64 v19[2];
    u64 v20[2];
    u64 v21[2];
    u64 v22[2];
    u64 v23[2];
    u64 v24[2];
    u64 v25[2];
    u64 v26[2];
    u64 v27[2];
    u64 v28[2];
    u64 v29[2];
    u64 v30[2];


    u64 fpcr[2];
    u64 fpsr[2];
}cpu_ctx;

#endif // #ifndef ARMV8_H

