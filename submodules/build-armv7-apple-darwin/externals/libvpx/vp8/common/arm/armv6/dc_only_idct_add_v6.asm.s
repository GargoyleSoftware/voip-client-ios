@ This file was created from a .asm file
@  using the ads2gas_apple.pl script.

	.set WIDE_REFERENCE, 0
	.set ARCHITECTURE, 5
	.set DO1STROUNDING, 0
@
@  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
@
@  Use of this source code is governed by a BSD-style license and patent
@  grant that can be found in the LICENSE file in the root of the source
@  tree. All contributing project authors may be found in the AUTHORS
@  file in the root of the source tree.
@

    .globl _vp8_dc_only_idct_add_v6
	.globl vp8_dc_only_idct_add_v6

.text
.p2align 2

@void vp8_dc_only_idct_add_c(short input_dc, unsigned char *pred_ptr,
@                            int pred_stride, unsigned char *dst_ptr,
@                            int dst_stride)
@ r0  input_dc
@ r1  pred_ptr
@ r2  pred_stride
@ r3  dst_ptr
@ sp  dst_stride

_vp8_dc_only_idct_add_v6:
	vp8_dc_only_idct_add_v6: @
    stmdb       sp!, {r4 - r7}

    add         r0, r0, #4                @ input_dc += 4
    ldr         r12, c0x0000FFFF
    ldr         r4, [r1], r2
    and         r0, r12, r0, asr #3       @ input_dc >> 3 + mask
    ldr         r6, [r1], r2
    orr         r0, r0, r0, lsl #16       @ a1 | a1

    ldr         r12, [sp, #16]            @ dst stride

    uxtab16     r5, r0, r4                @ a1+2 | a1+0
    uxtab16     r4, r0, r4, ror #8        @ a1+3 | a1+1
    uxtab16     r7, r0, r6
    uxtab16     r6, r0, r6, ror #8
    usat16      r5, #8, r5
    usat16      r4, #8, r4
    usat16      r7, #8, r7
    usat16      r6, #8, r6
    orr         r5, r5, r4, lsl #8
    orr         r7, r7, r6, lsl #8
    ldr         r4, [r1], r2
    str         r5, [r3], r12
    ldr         r6, [r1]
    str         r7, [r3], r12

    uxtab16     r5, r0, r4
    uxtab16     r4, r0, r4, ror #8
    uxtab16     r7, r0, r6
    uxtab16     r6, r0, r6, ror #8
    usat16      r5, #8, r5
    usat16      r4, #8, r4
    usat16      r7, #8, r7
    usat16      r6, #8, r6
    orr         r5, r5, r4, lsl #8
    orr         r7, r7, r6, lsl #8
    str         r5, [r3], r12
    str         r7, [r3]

    ldmia       sp!, {r4 - r7}
    bx          lr

    @  @ |vp8_dc_only_idct_add_v6|

@ Constant Pool
c0x0000FFFF: .long  0x0000FFFF
