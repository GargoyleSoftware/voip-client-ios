;*****************************************************************************
;* mc-a.asm: x86 motion compensation
;*****************************************************************************
;* Copyright (C) 2003-2011 x264 project
;*
;* Authors: Loren Merritt <lorenm@u.washington.edu>
;*          Jason Garrett-Glaser <darkshikari@gmail.com>
;*          Laurent Aimar <fenrir@via.ecp.fr>
;*          Dylan Yudaken <dyudaken@gmail.com>
;*          Holger Lubitz <holger@lubitz.org>
;*          Min Chen <chenm001.163.com>
;*          Oskar Arvidsson <oskar@irock.se>
;*
;* This program is free software; you can redistribute it and/or modify
;* it under the terms of the GNU General Public License as published by
;* the Free Software Foundation; either version 2 of the License, or
;* (at your option) any later version.
;*
;* This program is distributed in the hope that it will be useful,
;* but WITHOUT ANY WARRANTY; without even the implied warranty of
;* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;* GNU General Public License for more details.
;*
;* You should have received a copy of the GNU General Public License
;* along with this program; if not, write to the Free Software
;* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
;*
;* This program is also available under a commercial proprietary license.
;* For more information, contact us at licensing@x264.com.
;*****************************************************************************

%include "x86inc.asm"
%include "x86util.asm"

SECTION_RODATA 32

ch_shuf: db 0,2,2,4,4,6,6,8,1,3,3,5,5,7,7,9
ch_shuf_adj: times 8 db 0
             times 8 db 2
             times 8 db 4
             times 8 db 6
sq_1: times 1 dq 1

SECTION .text

cextern pb_0
cextern pw_1
cextern pw_4
cextern pw_8
cextern pw_32
cextern pw_64
cextern pw_00ff
cextern pw_pixel_max
cextern sw_64
cextern pd_32

;=============================================================================
; implicit weighted biprediction
;=============================================================================
; assumes log2_denom = 5, offset = 0, weight1 + weight2 = 64
%ifdef ARCH_X86_64
    DECLARE_REG_TMP 0,1,2,3,4,5,10,11
    %macro AVG_START 0-1 0
        PROLOGUE 6,7,%1
%ifdef WIN64
        movsxd r5, r5d
%endif
    %endmacro
%else
    DECLARE_REG_TMP 1,2,3,4,5,6,1,2
    %macro AVG_START 0-1 0
        PROLOGUE 0,7,%1
        mov t0, r0m
        mov t1, r1m
        mov t2, r2m
        mov t3, r3m
        mov t4, r4m
        mov t5, r5m
    %endmacro
%endif

%macro AVG_END 0
    sub eax, 2
    lea  t4, [t4+t5*2*SIZEOF_PIXEL]
    lea  t2, [t2+t3*2*SIZEOF_PIXEL]
    lea  t0, [t0+t1*2*SIZEOF_PIXEL]
    jg .height_loop
    REP_RET
%endmacro

%ifdef HIGH_BIT_DEPTH

%macro BIWEIGHT_MMX 2
    movh      m0, %1
    movh      m1, %2
    punpcklwd m0, m1
    pmaddwd   m0, m3
    paddd     m0, m4
    psrad     m0, 6
%endmacro

%macro BIWEIGHT_START_MMX 0
    movzx  t6d, word r6m
    mov    t7d, 64
    sub    t7d, t6d
    shl    t7d, 16
    add    t6d, t7d
    movd    m3, t6d
    SPLATD  m3, m3
    mova    m4, [pd_32]
    pxor    m5, m5
%endmacro

%else ;!HIGH_BIT_DEPTH
%macro BIWEIGHT_MMX 2
    movh      m0, %1
    movh      m1, %2
    punpcklbw m0, m5
    punpcklbw m1, m5
    pmullw    m0, m2
    pmullw    m1, m3
    paddw     m0, m1
    paddw     m0, m4
    psraw     m0, 6
%endmacro

%macro BIWEIGHT_START_MMX 0
    movd    m2, r6m
    SPLATW  m2, m2   ; weight_dst
    mova    m3, [pw_64]
    psubw   m3, m2   ; weight_src
    mova    m4, [pw_32] ; rounding
    pxor    m5, m5
%endmacro
%endif ;HIGH_BIT_DEPTH

%macro BIWEIGHT_SSSE3 2
    movh      m0, %1
    movh      m1, %2
    punpcklbw m0, m1
    pmaddubsw m0, m3
    paddw     m0, m4
    psraw     m0, 6
%endmacro

%macro BIWEIGHT_START_SSSE3 0
    movzx  t6d, byte r6m ; FIXME x86_64
    mov    t7d, 64
    sub    t7d, t6d
    shl    t7d, 8
    add    t6d, t7d
    movd    m3, t6d
    mova    m4, [pw_32]
    SPLATW  m3, m3   ; weight_dst,src
%endmacro

%ifdef HIGH_BIT_DEPTH
%macro BIWEIGHT_ROW 4
    BIWEIGHT   [%2], [%3]
%if %4==mmsize/4
    packssdw     m0, m0
    CLIPW        m0, m5, m7
    movh       [%1], m0
%else
    SWAP 0, 6
    BIWEIGHT   [%2+mmsize/2], [%3+mmsize/2]
    packssdw     m6, m0
    CLIPW        m6, m5, m7
    mova       [%1], m6
%endif
%endmacro

%else ;!HIGH_BIT_DEPTH
%macro BIWEIGHT_ROW 4
    BIWEIGHT [%2], [%3]
%if %4==mmsize/2
    packuswb   m0, m0
    movh     [%1], m0
%else
    SWAP 0, 6
    BIWEIGHT [%2+mmsize/2], [%3+mmsize/2]
    packuswb   m6, m0
    mova     [%1], m6
%endif
%endmacro

%endif ;HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; int pixel_avg_weight_w16( pixel *dst, int, pixel *src1, int, pixel *src2, int, int i_weight )
;-----------------------------------------------------------------------------
%macro AVG_WEIGHT 1-2 0
cglobal pixel_avg_weight_w%1
    BIWEIGHT_START
    AVG_START %2
%ifdef HIGH_BIT_DEPTH
    mova    m7, [pw_pixel_max]
%endif
.height_loop:
%if mmsize==16 && %1==mmsize/(2*SIZEOF_PIXEL)
    BIWEIGHT [t2], [t4]
    SWAP 0, 6
    BIWEIGHT [t2+SIZEOF_PIXEL*t3], [t4+SIZEOF_PIXEL*t5]
%ifdef HIGH_BIT_DEPTH
    packssdw m6, m0
    CLIPW    m6, m5, m7
%else ;!HIGH_BIT_DEPTH
    packuswb m6, m0
%endif ;HIGH_BIT_DEPTH
    movlps   [t0], m6
    movhps   [t0+SIZEOF_PIXEL*t1], m6
%else
%assign x 0
%rep (%1*SIZEOF_PIXEL+mmsize-1)/mmsize
    BIWEIGHT_ROW   t0+x,                   t2+x,                   t4+x,                 %1
    BIWEIGHT_ROW   t0+x+SIZEOF_PIXEL*t1,   t2+x+SIZEOF_PIXEL*t3,   t4+x+SIZEOF_PIXEL*t5, %1
%assign x x+mmsize
%endrep
%endif
    AVG_END
%endmacro

%define BIWEIGHT BIWEIGHT_MMX
%define BIWEIGHT_START BIWEIGHT_START_MMX
INIT_MMX mmx2
AVG_WEIGHT 4
AVG_WEIGHT 8
AVG_WEIGHT 16
%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
AVG_WEIGHT 4,  8
AVG_WEIGHT 8,  8
AVG_WEIGHT 16, 8
%else ;!HIGH_BIT_DEPTH
INIT_XMM sse2
AVG_WEIGHT 8,  7
AVG_WEIGHT 16, 7
%define BIWEIGHT BIWEIGHT_SSSE3
%define BIWEIGHT_START BIWEIGHT_START_SSSE3
INIT_MMX ssse3
AVG_WEIGHT 4
INIT_XMM ssse3
AVG_WEIGHT 8,  7
AVG_WEIGHT 16, 7
%endif ;HIGH_BIT_DEPTH

;=============================================================================
; P frame explicit weighted prediction
;=============================================================================

%ifdef HIGH_BIT_DEPTH
%macro WEIGHT_START 1 ; (width)
    mova        m0, [r4+ 0]         ; 1<<denom
    mova        m3, [r4+16]
    movd        m2, [r4+32]         ; denom
    mova        m4, [pw_pixel_max]
    paddw       m2, [sq_1]          ; denom+1
%endmacro

%macro WEIGHT 2 ; (src1, src2)
    movh        m5, [%1]
    movh        m6, [%2]
    punpcklwd   m5, m0
    punpcklwd   m6, m0
    pmaddwd     m5, m3
    pmaddwd     m6, m3
    psrad       m5, m2
    psrad       m6, m2
    packssdw    m5, m6
%endmacro

%macro WEIGHT_TWO_ROW 3 ; (src, dst, width)
    %assign x 0
%rep (%3+mmsize/2-1)/(mmsize/2)
%if %3-x/2 <= 4 && mmsize == 16
    WEIGHT      %1+x, %1+r3+x
    CLIPW         m5, [pb_0], m4
    movh      [%2+x], m5
    movhps [%2+r1+x], m5
%else
    WEIGHT      %1+x, %1+x+mmsize/2
    SWAP           5,  7
    WEIGHT   %1+r3+x, %1+r3+x+mmsize/2
    CLIPW         m5, [pb_0], m4
    CLIPW         m7, [pb_0], m4
    mova      [%2+x], m7
    mova   [%2+r1+x], m5
%endif
    %assign x x+mmsize
%endrep
%endmacro

%else ; !HIGH_BIT_DEPTH

%macro WEIGHT_START 1
    mova     m3, [r4]
    mova     m6, [r4+16]
    movd     m5, [r4+32]
    pxor     m2, m2
%if (%1 == 20 || %1 == 12) && mmsize == 16
    movdq2q mm3, xmm3
    movdq2q mm4, xmm4
    movdq2q mm5, xmm5
    movdq2q mm6, xmm6
    pxor    mm2, mm2
%endif
%endmacro

%macro WEIGHT_START_SSSE3 1
    mova     m3, [r4]
    mova     m4, [r4+16]
    pxor     m2, m2
%if %1 == 20 || %1 == 12
    movdq2q mm3, xmm3
    movdq2q mm4, xmm4
    pxor    mm2, mm2
%endif
%endmacro

;; macro to weight mmsize bytes taking half from %1 and half from %2
%macro WEIGHT 2             ; (src1,src2)
    movh      m0, [%1]
    movh      m1, [%2]
    punpcklbw m0, m2        ;setup
    punpcklbw m1, m2        ;setup
    pmullw    m0, m3        ;scale
    pmullw    m1, m3        ;scale
    paddsw    m0, m6        ;1<<(denom-1)+(offset<<denom)
    paddsw    m1, m6        ;1<<(denom-1)+(offset<<denom)
    psraw     m0, m5        ;denom
    psraw     m1, m5        ;denom
%endmacro

%macro WEIGHT_SSSE3 2
    movh      m0, [%1]
    movh      m1, [%2]
    punpcklbw m0, m2
    punpcklbw m1, m2
    psllw     m0, 7
    psllw     m1, 7
    pmulhrsw  m0, m3
    pmulhrsw  m1, m3
    paddw     m0, m4
    paddw     m1, m4
%endmacro

%macro WEIGHT_SAVE_ROW 3        ;(src,dst,width)
%if %3 == 16
    mova     [%2], %1
%elif %3 == 8
    movq     [%2], %1
%else
    movd     [%2], %1       ; width 2 can write garbage for last 2 bytes
%endif
%endmacro

%macro WEIGHT_ROW 3         ; (src,dst,width)
    ;; load weights
    WEIGHT           %1, (%1+(mmsize/2))
    packuswb         m0, m1        ;put bytes into m0
    WEIGHT_SAVE_ROW  m0, %2, %3
%endmacro

%macro WEIGHT_SAVE_COL 2        ;(dst,size)
%if %2 == 8
    packuswb     m0, m1
    movq       [%1], m0
    movhps  [%1+r1], m0
%else
    packuswb     m0, m0
    packuswb     m1, m1
    movd       [%1], m0    ; width 2 can write garbage for last 2 bytes
    movd    [%1+r1], m1
%endif
%endmacro

%macro WEIGHT_COL 3     ; (src,dst,width)
%if %3 <= 4 && mmsize == 16
    INIT_MMX
    ;; load weights
    WEIGHT           %1, (%1+r3)
    WEIGHT_SAVE_COL  %2, %3
    INIT_XMM
%else
    WEIGHT           %1, (%1+r3)
    WEIGHT_SAVE_COL  %2, %3
%endif

%endmacro

%macro WEIGHT_TWO_ROW 3 ; (src,dst,width)
%assign x 0
%rep %3
%if (%3-x) >= mmsize
    WEIGHT_ROW    (%1+x),    (%2+x), mmsize     ; weight 1 mmsize
    WEIGHT_ROW (%1+r3+x), (%2+r1+x), mmsize     ; weight 1 mmsize
    %assign x (x+mmsize)
%else
    WEIGHT_COL (%1+x),(%2+x),(%3-x)
    %exitrep
%endif
%if x >= %3
    %exitrep
%endif
%endrep
%endmacro

%endif ; HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
;void mc_weight_wX( pixel *dst, int i_dst_stride, pixel *src, int i_src_stride, weight_t *weight, int h )
;-----------------------------------------------------------------------------

%ifdef ARCH_X86_64
%define NUMREGS 6
%define LOAD_HEIGHT
%define HEIGHT_REG r5d
%define TMP_REG r6d
%else
%define NUMREGS 5
%define TMP_REG r5d
%define LOAD_HEIGHT mov r4d, r5m
%define HEIGHT_REG r4d
%endif

%assign XMMREGS 7
%ifdef HIGH_BIT_DEPTH
%assign NUMREGS NUMREGS+1
%assign XMMREGS 8
%endif

%macro WEIGHTER 1
    cglobal mc_weight_w%1, NUMREGS, NUMREGS, XMMREGS
    FIX_STRIDES r1, r3
    WEIGHT_START %1
    LOAD_HEIGHT
.loop:
    WEIGHT_TWO_ROW r2, r0, %1
    lea  r0, [r0+r1*2]
    lea  r2, [r2+r3*2]
    sub HEIGHT_REG, 2
    jg .loop
    REP_RET
%endmacro

INIT_MMX mmx2
WEIGHTER  4
WEIGHTER  8
WEIGHTER 12
WEIGHTER 16
WEIGHTER 20
INIT_XMM sse2
WEIGHTER  8
WEIGHTER 16
WEIGHTER 20
%ifdef HIGH_BIT_DEPTH
WEIGHTER 12
INIT_XMM avx
WEIGHTER  8
WEIGHTER 12
WEIGHTER 16
WEIGHTER 20
%else
%define WEIGHT WEIGHT_SSSE3
%define WEIGHT_START WEIGHT_START_SSSE3
INIT_MMX ssse3
WEIGHTER  4
INIT_XMM ssse3
WEIGHTER  8
WEIGHTER 16
WEIGHTER 20
INIT_XMM avx
WEIGHTER  8
WEIGHTER 16
WEIGHTER 20
%endif

%macro OFFSET_OP 7
    mov%6        m0, [%1]
    mov%6        m1, [%2]
%ifdef HIGH_BIT_DEPTH
    p%5usw       m0, m2
    p%5usw       m1, m2
%ifidn %5,add
    pminsw       m0, m3
    pminsw       m1, m3
%endif
%else
    p%5usb       m0, m2
    p%5usb       m1, m2
%endif
    mov%7      [%3], m0
    mov%7      [%4], m1
%endmacro

%macro OFFSET_TWO_ROW 4
%assign x 0
%rep %3
%if (%3*SIZEOF_PIXEL-x) >= mmsize
    OFFSET_OP (%1+x), (%1+x+r3), (%2+x), (%2+x+r1), %4, u, a
    %assign x (x+mmsize)
%else
%ifdef HIGH_BIT_DEPTH
    OFFSET_OP (%1+x), (%1+x+r3), (%2+x), (%2+x+r1), %4, h, h
%else
    OFFSET_OP (%1+x), (%1+x+r3), (%2+x), (%2+x+r1), %4, d, d
%endif
    %exitrep
%endif
%if x >= %3*SIZEOF_PIXEL
    %exitrep
%endif
%endrep
%endmacro

;-----------------------------------------------------------------------------
;void mc_offset_wX( pixel *src, int i_src_stride, pixel *dst, int i_dst_stride, weight_t *w, int h )
;-----------------------------------------------------------------------------
%macro OFFSET 2
    cglobal mc_offset%2_w%1, NUMREGS, NUMREGS
    FIX_STRIDES r1, r3
    mova m2, [r4]
%ifdef HIGH_BIT_DEPTH
%ifidn %2,add
    mova m3, [pw_pixel_max]
%endif
%endif
    LOAD_HEIGHT
.loop:
    OFFSET_TWO_ROW r2, r0, %1, %2
    lea  r0, [r0+r1*2]
    lea  r2, [r2+r3*2]
    sub HEIGHT_REG, 2
    jg .loop
    REP_RET
%endmacro

%macro OFFSETPN 1
       OFFSET %1, add
       OFFSET %1, sub
%endmacro
INIT_MMX mmx2
OFFSETPN  4
OFFSETPN  8
OFFSETPN 12
OFFSETPN 16
OFFSETPN 20
INIT_XMM sse2
OFFSETPN 12
OFFSETPN 16
OFFSETPN 20
INIT_XMM avx
OFFSETPN 12
OFFSETPN 16
OFFSETPN 20
%ifdef HIGH_BIT_DEPTH
INIT_XMM sse2
OFFSETPN  8
INIT_XMM avx
OFFSETPN  8
%endif
%undef LOAD_HEIGHT
%undef HEIGHT_REG
%undef NUMREGS



;=============================================================================
; pixel avg
;=============================================================================

;-----------------------------------------------------------------------------
; void pixel_avg_4x4( pixel *dst, int dst_stride,
;                     pixel *src1, int src1_stride, pixel *src2, int src2_stride, int weight );
;-----------------------------------------------------------------------------
%macro AVGH 2
cglobal pixel_avg_%1x%2
    mov eax, %2
    cmp dword r6m, 32
    jne pixel_avg_weight_w%1 %+ SUFFIX
%if mmsize == 16 && %1 == 16
    test dword r4m, 15
    jz pixel_avg_w%1_sse2
%endif
    jmp pixel_avg_w%1_mmx2
%endmacro

;-----------------------------------------------------------------------------
; void pixel_avg_w4( pixel *dst, int dst_stride,
;                    pixel *src1, int src1_stride, pixel *src2, int src2_stride,
;                    int height, int weight );
;-----------------------------------------------------------------------------

%macro AVG_FUNC 3
cglobal pixel_avg_w%1
    AVG_START
.height_loop:
%assign x 0
%rep (%1*SIZEOF_PIXEL+mmsize-1)/mmsize
    %2     m0, [t2+x]
    %2     m1, [t2+x+SIZEOF_PIXEL*t3]
%ifdef HIGH_BIT_DEPTH
    pavgw  m0, [t4+x]
    pavgw  m1, [t4+x+SIZEOF_PIXEL*t5]
%else ;!HIGH_BIT_DEPTH
    pavgb  m0, [t4+x]
    pavgb  m1, [t4+x+SIZEOF_PIXEL*t5]
%endif
    %3     [t0+x], m0
    %3     [t0+x+SIZEOF_PIXEL*t1], m1
%assign x x+mmsize
%endrep
    AVG_END
%endmacro

%ifdef HIGH_BIT_DEPTH

INIT_MMX mmx2
AVG_FUNC 4, movq, movq
AVGH 4, 8
AVGH 4, 4
AVGH 4, 2

AVG_FUNC 8, movq, movq
AVGH 8, 16
AVGH 8,  8
AVGH 8,  4

AVG_FUNC 16, movq, movq
AVGH 16, 16
AVGH 16,  8

INIT_XMM sse2
AVG_FUNC 4, movq, movq
AVGH  4, 8
AVGH  4, 4
AVGH  4, 2

AVG_FUNC 8, movdqu, movdqa
AVGH  8, 16
AVGH  8,  8
AVGH  8,  4

AVG_FUNC 16, movdqu, movdqa
AVGH  16, 16
AVGH  16,  8

%else ;!HIGH_BIT_DEPTH

INIT_MMX mmx2
AVG_FUNC 4, movd, movd
AVGH 4, 8
AVGH 4, 4
AVGH 4, 2

AVG_FUNC 8, movq, movq
AVGH 8, 16
AVGH 8,  8
AVGH 8,  4

AVG_FUNC 16, movq, movq
AVGH 16, 16
AVGH 16, 8

INIT_XMM sse2
AVG_FUNC 16, movdqu, movdqa
AVGH 16, 16
AVGH 16,  8
AVGH  8, 16
AVGH  8,  8
AVGH  8,  4
INIT_XMM ssse3
AVGH 16, 16
AVGH 16,  8
AVGH  8, 16
AVGH  8,  8
AVGH  8,  4
INIT_MMX ssse3
AVGH  4,  8
AVGH  4,  4
AVGH  4,  2

%endif ;HIGH_BIT_DEPTH



;=============================================================================
; pixel avg2
;=============================================================================

%ifdef HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; void pixel_avg2_wN( uint16_t *dst,  int dst_stride,
;                     uint16_t *src1, int src_stride,
;                     uint16_t *src2, int height );
;-----------------------------------------------------------------------------
%macro AVG2_W_ONE 1
cglobal pixel_avg2_w%1, 6,7,4
    sub     r4, r2
    lea     r6, [r4+r3*2]
.height_loop:
    movu    m0, [r2]
    movu    m1, [r2+r3*2]
%if mmsize == 8
    pavgw   m0, [r2+r4]
    pavgw   m1, [r2+r6]
%else
    movu    m2, [r2+r4]
    movu    m3, [r2+r6]
    pavgw   m0, m2
    pavgw   m1, m3
%endif
    mova   [r0], m0
    mova   [r0+r1*2], m1
    sub    r5d, 2
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
    jg .height_loop
    REP_RET
%endmacro

%macro AVG2_W_TWO 3
cglobal pixel_avg2_w%1, 6,7,8
    sub     r4, r2
    lea     r6, [r4+r3*2]
.height_loop:
    movu    m0, [r2]
    %2      m1, [r2+mmsize]
    movu    m2, [r2+r3*2]
    %2      m3, [r2+r3*2+mmsize]
%if mmsize == 8
    pavgw   m0, [r2+r4]
    pavgw   m1, [r2+r4+mmsize]
    pavgw   m2, [r2+r6]
    pavgw   m3, [r2+r6+mmsize]
%else
    movu    m4, [r2+r4]
    %2      m5, [r2+r4+mmsize]
    movu    m6, [r2+r6]
    %2      m7, [r2+r6+mmsize]
    pavgw   m0, m4
    pavgw   m1, m5
    pavgw   m2, m6
    pavgw   m3, m7
%endif
    mova   [r0], m0
    %3     [r0+mmsize], m1
    mova   [r0+r1*2], m2
    %3     [r0+r1*2+mmsize], m3
    sub    r5d, 2
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
    jg .height_loop
    REP_RET
%endmacro

INIT_MMX mmx2
AVG2_W_ONE  4
AVG2_W_TWO  8, movu, mova
INIT_XMM sse2
AVG2_W_ONE  8
AVG2_W_TWO 10, movd, movd
AVG2_W_TWO 16, movu, mova

INIT_MMX
cglobal pixel_avg2_w10_mmx2, 6,7
    sub     r4, r2
    lea     r6, [r4+r3*2]
.height_loop:
    movu    m0, [r2+ 0]
    movu    m1, [r2+ 8]
    movh    m2, [r2+16]
    movu    m3, [r2+r3*2+ 0]
    movu    m4, [r2+r3*2+ 8]
    movh    m5, [r2+r3*2+16]
    pavgw   m0, [r2+r4+ 0]
    pavgw   m1, [r2+r4+ 8]
    pavgw   m2, [r2+r4+16]
    pavgw   m3, [r2+r6+ 0]
    pavgw   m4, [r2+r6+ 8]
    pavgw   m5, [r2+r6+16]
    mova   [r0+ 0], m0
    mova   [r0+ 8], m1
    movh   [r0+16], m2
    mova   [r0+r1*2+ 0], m3
    mova   [r0+r1*2+ 8], m4
    movh   [r0+r1*2+16], m5
    sub    r5d, 2
    lea     r2, [r2+r3*2*2]
    lea     r0, [r0+r1*2*2]
    jg .height_loop
    REP_RET

cglobal pixel_avg2_w16_mmx2, 6,7
    sub     r4, r2
    lea     r6, [r4+r3*2]
.height_loop:
    movu    m0, [r2+ 0]
    movu    m1, [r2+ 8]
    movu    m2, [r2+16]
    movu    m3, [r2+24]
    movu    m4, [r2+r3*2+ 0]
    movu    m5, [r2+r3*2+ 8]
    movu    m6, [r2+r3*2+16]
    movu    m7, [r2+r3*2+24]
    pavgw   m0, [r2+r4+ 0]
    pavgw   m1, [r2+r4+ 8]
    pavgw   m2, [r2+r4+16]
    pavgw   m3, [r2+r4+24]
    pavgw   m4, [r2+r6+ 0]
    pavgw   m5, [r2+r6+ 8]
    pavgw   m6, [r2+r6+16]
    pavgw   m7, [r2+r6+24]
    mova   [r0+ 0], m0
    mova   [r0+ 8], m1
    mova   [r0+16], m2
    mova   [r0+24], m3
    mova   [r0+r1*2+ 0], m4
    mova   [r0+r1*2+ 8], m5
    mova   [r0+r1*2+16], m6
    mova   [r0+r1*2+24], m7
    sub    r5d, 2
    lea     r2, [r2+r3*2*2]
    lea     r0, [r0+r1*2*2]
    jg .height_loop
    REP_RET

cglobal pixel_avg2_w18_mmx2, 6,7
    sub     r4, r2
.height_loop:
    movu    m0, [r2+ 0]
    movu    m1, [r2+ 8]
    movu    m2, [r2+16]
    movu    m3, [r2+24]
    movh    m4, [r2+32]
    pavgw   m0, [r2+r4+ 0]
    pavgw   m1, [r2+r4+ 8]
    pavgw   m2, [r2+r4+16]
    pavgw   m3, [r2+r4+24]
    pavgw   m4, [r2+r4+32]
    mova   [r0+ 0], m0
    mova   [r0+ 8], m1
    mova   [r0+16], m2
    mova   [r0+24], m3
    movh   [r0+32], m4
    sub    r5d, 1
    lea     r2, [r2+r3*2]
    lea     r0, [r0+r1*2]
    jg .height_loop
    REP_RET

INIT_XMM
cglobal pixel_avg2_w18_sse2, 6,7,6
    sub     r4, r2
.height_loop:
    movu    m0, [r2+ 0]
    movu    m1, [r2+16]
    movh    m2, [r2+32]
    movu    m3, [r2+r4+ 0]
    movu    m4, [r2+r4+16]
    movh    m5, [r2+r4+32]
    pavgw   m0, m3
    pavgw   m1, m4
    pavgw   m2, m5
    mova   [r0+ 0], m0
    mova   [r0+16], m1
    movh   [r0+32], m2
    sub    r5d, 1
    lea     r2, [r2+r3*2]
    lea     r0, [r0+r1*2]
    jg .height_loop
    REP_RET
%endif ; HIGH_BIT_DEPTH

%ifndef HIGH_BIT_DEPTH
;-----------------------------------------------------------------------------
; void pixel_avg2_w4( uint8_t *dst, int dst_stride,
;                     uint8_t *src1, int src_stride,
;                     uint8_t *src2, int height );
;-----------------------------------------------------------------------------
%macro AVG2_W8 2
cglobal pixel_avg2_w%1_mmx2, 6,7
    sub    r4, r2
    lea    r6, [r4+r3]
.height_loop:
    %2     mm0, [r2]
    %2     mm1, [r2+r3]
    pavgb  mm0, [r2+r4]
    pavgb  mm1, [r2+r6]
    lea    r2, [r2+r3*2]
    %2     [r0], mm0
    %2     [r0+r1], mm1
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    REP_RET
%endmacro

INIT_MMX
AVG2_W8 4, movd
AVG2_W8 8, movq

%macro AVG2_W16 2
cglobal pixel_avg2_w%1_mmx2, 6,7
    sub    r2, r4
    lea    r6, [r2+r3]
.height_loop:
    movq   mm0, [r4]
    %2     mm1, [r4+8]
    movq   mm2, [r4+r3]
    %2     mm3, [r4+r3+8]
    pavgb  mm0, [r4+r2]
    pavgb  mm1, [r4+r2+8]
    pavgb  mm2, [r4+r6]
    pavgb  mm3, [r4+r6+8]
    lea    r4, [r4+r3*2]
    movq   [r0], mm0
    %2     [r0+8], mm1
    movq   [r0+r1], mm2
    %2     [r0+r1+8], mm3
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    REP_RET
%endmacro

AVG2_W16 12, movd
AVG2_W16 16, movq

cglobal pixel_avg2_w20_mmx2, 6,7
    sub    r2, r4
    lea    r6, [r2+r3]
.height_loop:
    movq   mm0, [r4]
    movq   mm1, [r4+8]
    movd   mm2, [r4+16]
    movq   mm3, [r4+r3]
    movq   mm4, [r4+r3+8]
    movd   mm5, [r4+r3+16]
    pavgb  mm0, [r4+r2]
    pavgb  mm1, [r4+r2+8]
    pavgb  mm2, [r4+r2+16]
    pavgb  mm3, [r4+r6]
    pavgb  mm4, [r4+r6+8]
    pavgb  mm5, [r4+r6+16]
    lea    r4, [r4+r3*2]
    movq   [r0], mm0
    movq   [r0+8], mm1
    movd   [r0+16], mm2
    movq   [r0+r1], mm3
    movq   [r0+r1+8], mm4
    movd   [r0+r1+16], mm5
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    REP_RET

cglobal pixel_avg2_w16_sse2, 6,7
    sub    r4, r2
    lea    r6, [r4+r3]
.height_loop:
    movdqu xmm0, [r2]
    movdqu xmm2, [r2+r3]
    movdqu xmm1, [r2+r4]
    movdqu xmm3, [r2+r6]
    lea    r2, [r2+r3*2]
    pavgb  xmm0, xmm1
    pavgb  xmm2, xmm3
    movdqa [r0], xmm0
    movdqa [r0+r1], xmm2
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    REP_RET

%macro AVG2_W20 1
cglobal pixel_avg2_w20_%1, 6,7
    sub    r2, r4
    lea    r6, [r2+r3]
.height_loop:
    movdqu xmm0, [r4]
    movdqu xmm2, [r4+r3]
%ifidn %1, sse2_misalign
    movd   mm4,  [r4+16]
    movd   mm5,  [r4+r3+16]
    pavgb  xmm0, [r4+r2]
    pavgb  xmm2, [r4+r6]
%else
    movdqu xmm1, [r4+r2]
    movdqu xmm3, [r4+r6]
    movd   mm4,  [r4+16]
    movd   mm5,  [r4+r3+16]
    pavgb  xmm0, xmm1
    pavgb  xmm2, xmm3
%endif
    pavgb  mm4,  [r4+r2+16]
    pavgb  mm5,  [r4+r6+16]
    lea    r4, [r4+r3*2]
    movdqa [r0], xmm0
    movd   [r0+16], mm4
    movdqa [r0+r1], xmm2
    movd   [r0+r1+16], mm5
    lea    r0, [r0+r1*2]
    sub    r5d, 2
    jg     .height_loop
    REP_RET
%endmacro

AVG2_W20 sse2
AVG2_W20 sse2_misalign

; Cacheline split code for processors with high latencies for loads
; split over cache lines.  See sad-a.asm for a more detailed explanation.
; This particular instance is complicated by the fact that src1 and src2
; can have different alignments.  For simplicity and code size, only the
; MMX cacheline workaround is used.  As a result, in the case of SSE2
; pixel_avg, the cacheline check functions calls the SSE2 version if there
; is no cacheline split, and the MMX workaround if there is.

%macro INIT_SHIFT 2
    and    eax, 7
    shl    eax, 3
    movd   %1, [sw_64]
    movd   %2, eax
    psubw  %1, %2
%endmacro

%macro AVG_CACHELINE_START 0
    %assign stack_offset 0
    INIT_SHIFT mm6, mm7
    mov    eax, r4m
    INIT_SHIFT mm4, mm5
    PROLOGUE 6,6
    and    r2, ~7
    and    r4, ~7
    sub    r4, r2
.height_loop:
%endmacro

%macro AVG_CACHELINE_LOOP 2
    movq   mm1, [r2+%1]
    movq   mm0, [r2+8+%1]
    movq   mm3, [r2+r4+%1]
    movq   mm2, [r2+r4+8+%1]
    psrlq  mm1, mm7
    psllq  mm0, mm6
    psrlq  mm3, mm5
    psllq  mm2, mm4
    por    mm0, mm1
    por    mm2, mm3
    pavgb  mm2, mm0
    %2 [r0+%1], mm2
%endmacro

%macro AVG_CACHELINE_FUNC 2
pixel_avg2_w%1_cache_mmx2:
    AVG_CACHELINE_START
    AVG_CACHELINE_LOOP 0, movq
%if %1>8
    AVG_CACHELINE_LOOP 8, movq
%if %1>16
    AVG_CACHELINE_LOOP 16, movd
%endif
%endif
    add    r2, r3
    add    r0, r1
    dec    r5d
    jg .height_loop
    REP_RET
%endmacro

%macro AVG_CACHELINE_CHECK 3 ; width, cacheline, instruction set
%if %1 == 12
;w12 isn't needed because w16 is just as fast if there's no cacheline split
%define cachesplit pixel_avg2_w16_cache_mmx2
%else
%define cachesplit pixel_avg2_w%1_cache_mmx2
%endif
cglobal pixel_avg2_w%1_cache%2_%3
    mov    eax, r2m
    and    eax, %2-1
    cmp    eax, (%2-%1-(%1 % 8))
%if %1==12||%1==20
    jbe pixel_avg2_w%1_%3
%else
    jb pixel_avg2_w%1_%3
%endif
%if 0 ; or %1==8 - but the extra branch seems too expensive
    ja cachesplit
%ifdef ARCH_X86_64
    test      r4b, 1
%else
    test byte r4m, 1
%endif
    jz pixel_avg2_w%1_%3
%else
    or     eax, r4m
    and    eax, 7
    jz pixel_avg2_w%1_%3
    mov    eax, r2m
%endif
%if mmsize==16 || (%1==8 && %2==64)
    AVG_CACHELINE_FUNC %1, %2
%else
    jmp cachesplit
%endif
%endmacro

INIT_MMX
AVG_CACHELINE_CHECK  8, 64, mmx2
AVG_CACHELINE_CHECK 12, 64, mmx2
%ifndef ARCH_X86_64
AVG_CACHELINE_CHECK 16, 64, mmx2
AVG_CACHELINE_CHECK 20, 64, mmx2
AVG_CACHELINE_CHECK  8, 32, mmx2
AVG_CACHELINE_CHECK 12, 32, mmx2
AVG_CACHELINE_CHECK 16, 32, mmx2
AVG_CACHELINE_CHECK 20, 32, mmx2
%endif
INIT_XMM
AVG_CACHELINE_CHECK 16, 64, sse2
AVG_CACHELINE_CHECK 20, 64, sse2

; computed jump assumes this loop is exactly 48 bytes
%macro AVG16_CACHELINE_LOOP_SSSE3 2 ; alignment
ALIGN 16
avg_w16_align%1_%2_ssse3:
%if %1==0 && %2==0
    movdqa  xmm1, [r2]
    pavgb   xmm1, [r2+r4]
    add    r2, r3
%elif %1==0
    movdqa  xmm1, [r2+r4+16]
    palignr xmm1, [r2+r4], %2
    pavgb   xmm1, [r2]
    add    r2, r3
%elif %2&15==0
    movdqa  xmm1, [r2+16]
    palignr xmm1, [r2], %1
    pavgb   xmm1, [r2+r4]
    add    r2, r3
%else
    movdqa  xmm1, [r2+16]
    movdqa  xmm2, [r2+r4+16]
    palignr xmm1, [r2], %1
    palignr xmm2, [r2+r4], %2&15
    add    r2, r3
    pavgb   xmm1, xmm2
%endif
    movdqa  [r0], xmm1
    add    r0, r1
    dec    r5d
    jg     avg_w16_align%1_%2_ssse3
    ret
%if %1==0
    times 13 db 0x90 ; make sure the first ones don't end up short
%endif
%endmacro

cglobal pixel_avg2_w16_cache64_ssse3
%if 0 ; seems both tests aren't worth it if src1%16==0 is optimized
    mov   eax, r2m
    and   eax, 0x3f
    cmp   eax, 0x30
    jb x264_pixel_avg2_w16_sse2
    or    eax, r4m
    and   eax, 7
    jz x264_pixel_avg2_w16_sse2
%endif
    PROLOGUE 6, 7
    lea    r6, [r4+r2]
    and    r4, ~0xf
    and    r6, 0x1f
    and    r2, ~0xf
    lea    r6, [r6*3]    ;(offset + align*2)*3
    sub    r4, r2
    shl    r6, 4         ;jump = (offset + align*2)*48
%define avg_w16_addr avg_w16_align1_1_ssse3-(avg_w16_align2_2_ssse3-avg_w16_align1_1_ssse3)
%ifdef PIC
    lea   r11, [avg_w16_addr]
    add    r6, r11
%else
    lea    r6, [avg_w16_addr + r6]
%endif
%ifdef UNIX64
    jmp    r6
%else
    call   r6
    RET
%endif

%assign j 0
%assign k 1
%rep 16
AVG16_CACHELINE_LOOP_SSSE3 j, j
AVG16_CACHELINE_LOOP_SSSE3 j, k
%assign j j+1
%assign k k+1
%endrep
%endif ; !HIGH_BIT_DEPTH

;=============================================================================
; pixel copy
;=============================================================================

%macro COPY1 2
    movu  m0, [r2]
    movu  m1, [r2+r3]
    movu  m2, [r2+r3*2]
    movu  m3, [r2+%2]
    mova  [r0],      m0
    mova  [r0+r1],   m1
    mova  [r0+r1*2], m2
    mova  [r0+%1],   m3
%endmacro

%macro COPY2 2-4 0, 1
    movu  m0, [r2+%3*mmsize]
    movu  m1, [r2+%4*mmsize]
    movu  m2, [r2+r3+%3*mmsize]
    movu  m3, [r2+r3+%4*mmsize]
    movu  m4, [r2+r3*2+%3*mmsize]
    movu  m5, [r2+r3*2+%4*mmsize]
    movu  m6, [r2+%2+%3*mmsize]
    movu  m7, [r2+%2+%4*mmsize]
    mova  [r0+%3*mmsize],      m0
    mova  [r0+%4*mmsize],      m1
    mova  [r0+r1+%3*mmsize],   m2
    mova  [r0+r1+%4*mmsize],   m3
    mova  [r0+r1*2+%3*mmsize], m4
    mova  [r0+r1*2+%4*mmsize], m5
    mova  [r0+%1+%3*mmsize],   m6
    mova  [r0+%1+%4*mmsize],   m7
%endmacro

%macro COPY4 2
    COPY2 %1, %2, 0, 1
    COPY2 %1, %2, 2, 3
%endmacro

;-----------------------------------------------------------------------------
; void mc_copy_w4( uint8_t *dst, int i_dst_stride,
;                  uint8_t *src, int i_src_stride, int i_height )
;-----------------------------------------------------------------------------
INIT_MMX
cglobal mc_copy_w4_mmx, 4,6
    FIX_STRIDES r1, r3
    cmp dword r4m, 4
    lea     r5, [r3*3]
    lea     r4, [r1*3]
    je .end
%ifndef HIGH_BIT_DEPTH
    %define mova movd
    %define movu movd
%endif
    COPY1   r4, r5
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
.end:
    COPY1   r4, r5
    RET

%macro MC_COPY 1
%assign %%w %1*SIZEOF_PIXEL/mmsize
%if %%w > 0
cglobal mc_copy_w%1, 5,7,8*(%%w/2)
    FIX_STRIDES r1, r3
    lea     r6, [r3*3]
    lea     r5, [r1*3]
.height_loop:
    COPY %+ %%w r5, r6
    lea     r2, [r2+r3*4]
    lea     r0, [r0+r1*4]
    sub    r4d, 4
    jg .height_loop
    REP_RET
%endif
%endmacro

INIT_MMX mmx
MC_COPY  8
MC_COPY 16
INIT_XMM sse2
MC_COPY  8
MC_COPY 16
INIT_XMM aligned, sse2
MC_COPY 16



;=============================================================================
; prefetch
;=============================================================================
; assumes 64 byte cachelines
; FIXME doesn't cover all pixels in high depth and/or 4:4:4

;-----------------------------------------------------------------------------
; void prefetch_fenc( pixel *pix_y, int stride_y,
;                     pixel *pix_uv, int stride_uv, int mb_x )
;-----------------------------------------------------------------------------
INIT_MMX
%ifdef ARCH_X86_64
cglobal prefetch_fenc_mmx2, 5,5
    FIX_STRIDES r1d, r3d
    and    r4d, 3
    mov    eax, r4d
    imul   r4d, r1d
    lea    r0,  [r0+r4*4+64*SIZEOF_PIXEL]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]
    lea    r0,  [r0+r1*2]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]

    imul   eax, r3d
    lea    r2,  [r2+rax*2+64*SIZEOF_PIXEL]
    prefetcht0  [r2]
    prefetcht0  [r2+r3]
    RET

%else
cglobal prefetch_fenc_mmx2, 0,3
    mov    r2, r4m
    mov    r1, r1m
    mov    r0, r0m
    FIX_STRIDES r1
    and    r2, 3
    imul   r2, r1
    lea    r0, [r0+r2*4+64*SIZEOF_PIXEL]
    prefetcht0 [r0]
    prefetcht0 [r0+r1]
    lea    r0, [r0+r1*2]
    prefetcht0 [r0]
    prefetcht0 [r0+r1]

    mov    r2, r4m
    mov    r1, r3m
    mov    r0, r2m
    FIX_STRIDES r1
    and    r2, 3
    imul   r2, r1
    lea    r0, [r0+r2*2+64*SIZEOF_PIXEL]
    prefetcht0 [r0]
    prefetcht0 [r0+r1]
    ret
%endif ; ARCH_X86_64

;-----------------------------------------------------------------------------
; void prefetch_ref( pixel *pix, int stride, int parity )
;-----------------------------------------------------------------------------
cglobal prefetch_ref_mmx2, 3,3
    FIX_STRIDES r1d
    dec    r2d
    and    r2d, r1d
    lea    r0,  [r0+r2*8+64*SIZEOF_PIXEL]
    lea    r2,  [r1*3]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]
    prefetcht0  [r0+r1*2]
    prefetcht0  [r0+r2]
    lea    r0,  [r0+r1*4]
    prefetcht0  [r0]
    prefetcht0  [r0+r1]
    prefetcht0  [r0+r1*2]
    prefetcht0  [r0+r2]
    RET



;=============================================================================
; chroma MC
;=============================================================================

%ifdef ARCH_X86_64
    DECLARE_REG_TMP 10,11,6
%else
    DECLARE_REG_TMP 0,1,2
%endif

%macro MC_CHROMA_START 0
    movifnidn r3,  r3mp
    movifnidn r4d, r4m
    movifnidn r5d, r5m
    movifnidn t2d, r6m
    mov       t0d, t2d
    mov       t1d, r5d
    sar       t0d, 3
    sar       t1d, 3
    imul      t0d, r4d
    lea       t0d, [t0+t1*2]
    FIX_STRIDES t0d
    movsxdifnidn t0, t0d
    add       r3,  t0            ; src += (dx>>3) + (dy>>3) * src_stride
%endmacro

%ifdef HIGH_BIT_DEPTH
%macro UNPACK_UNALIGNED 4
    movu       %1, [%4+0]
    movu       %2, [%4+4]
    punpckhwd  %3, %1, %2
    punpcklwd  %1, %2
%if mmsize == 8
    mova       %2, %1
    punpcklwd  %1, %3
    punpckhwd  %2, %3
%else
    shufps     %2, %1, %3, q3131
    shufps     %1, %3, q2020
%endif
%endmacro
%else ; !HIGH_BIT_DEPTH
%macro UNPACK_UNALIGNED 3
%if mmsize == 8 || cpuflag(misalign)
    punpcklwd  %1, %3
%else
    movh       %2, %3
    punpcklwd  %1, %2
%endif
%endmacro
%endif ; HIGH_BIT_DEPTH

;-----------------------------------------------------------------------------
; void mc_chroma( uint8_t *dstu, uint8_t *dstv, int dst_stride,
;                 uint8_t *src, int src_stride,
;                 int dx, int dy,
;                 int width, int height )
;-----------------------------------------------------------------------------
%macro MC_CHROMA 0
cglobal mc_chroma, 0,6
    MC_CHROMA_START
    FIX_STRIDES r4
    and       r5d, 7
%ifdef ARCH_X86_64
    jz .mc1dy
%endif
    and       t2d, 7
%ifdef ARCH_X86_64
    jz .mc1dx
%endif
    shl       r5d, 16
    add       t2d, r5d
    mov       t0d, t2d
    shl       t2d, 8
    sub       t2d, t0d
    add       t2d, 0x80008 ; (x<<24) + ((8-x)<<16) + (y<<8) + (8-y)
    cmp dword r7m, 4
%if mmsize==8
.skip_prologue:
%else
    jl mc_chroma_mmx2 %+ .skip_prologue
    WIN64_SPILL_XMM 9
%endif
    movd       m5, t2d
    movifnidn  r0, r0mp
    movifnidn  r1, r1mp
    movifnidn r2d, r2m
    movifnidn r5d, r8m
    pxor       m6, m6
    punpcklbw  m5, m6
%if mmsize==8
    pshufw     m7, m5, q3232
    pshufw     m6, m5, q0000
    pshufw     m5, m5, q1111
    jge .width4
%else
%ifdef WIN64
    cmp dword r7m, 4 ; flags were clobbered by WIN64_SPILL_XMM
%endif
    pshufd     m7, m5, q1111
    punpcklwd  m5, m5
    pshufd     m6, m5, q0000
    pshufd     m5, m5, q1111
    jg .width8
%endif
%ifdef HIGH_BIT_DEPTH
    add        r2, r2
    UNPACK_UNALIGNED m0, m1, m2, r3
%else
    movu       m0, [r3]
    UNPACK_UNALIGNED m0, m1, [r3+2]
    mova       m1, m0
    pand       m0, [pw_00ff]
    psrlw      m1, 8
%endif ; HIGH_BIT_DEPTH
    pmaddwd    m0, m7
    pmaddwd    m1, m7
    packssdw   m0, m1
    SWAP        3, 0
ALIGN 4
.loop2:
%ifdef HIGH_BIT_DEPTH
    UNPACK_UNALIGNED m0, m1, m2, r3+r4
    pmullw     m3, m6
%else ; !HIGH_BIT_DEPTH
    movu       m0, [r3+r4]
    UNPACK_UNALIGNED m0, m1, [r3+r4+2]
    pmullw     m3, m6
    mova       m1, m0
    pand       m0, [pw_00ff]
    psrlw      m1, 8
%endif ; HIGH_BIT_DEPTH
    pmaddwd    m0, m7
    pmaddwd    m1, m7
    mova       m2, [pw_32]
    packssdw   m0, m1
    paddw      m2, m3
    mova       m3, m0
    pmullw     m0, m5
    paddw      m0, m2
    psrlw      m0, 6
%ifdef HIGH_BIT_DEPTH
    movh     [r0], m0
%if mmsize == 8
    psrlq      m0, 32
    movh     [r1], m0
%else
    movhps   [r1], m0
%endif
%else ; !HIGH_BIT_DEPTH
    packuswb   m0, m0
    movd     [r0], m0
%if mmsize==8
    psrlq      m0, 16
%else
    psrldq     m0, 4
%endif
    movd     [r1], m0
%endif ; HIGH_BIT_DEPTH
    add        r3, r4
    add        r0, r2
    add        r1, r2
    dec       r5d
    jg .loop2
    REP_RET

%if mmsize==8
.width4:
%ifdef ARCH_X86_64
    mov        t0, r0
    mov        t1, r1
    mov        t2, r3
    %define multy0 [rsp-8]
    mova    multy0, m5
%else
    mov       r3m, r3
    %define multy0 r4m
    mova    multy0, m5
%endif
%else
.width8:
%ifdef ARCH_X86_64
    %define multy0 m8
    SWAP        8, 5
%else
    %define multy0 r0m
    mova    multy0, m5
%endif
%endif
    FIX_STRIDES r2
.loopx:
%ifdef HIGH_BIT_DEPTH
    UNPACK_UNALIGNED m0, m2, m4, r3
    UNPACK_UNALIGNED m1, m3, m5, r3+mmsize
%else
    movu       m0, [r3]
    movu       m1, [r3+mmsize/2]
    UNPACK_UNALIGNED m0, m2, [r3+2]
    UNPACK_UNALIGNED m1, m3, [r3+2+mmsize/2]
    psrlw      m2, m0, 8
    psrlw      m3, m1, 8
    pand       m0, [pw_00ff]
    pand       m1, [pw_00ff]
%endif
    pmaddwd    m0, m7
    pmaddwd    m2, m7
    pmaddwd    m1, m7
    pmaddwd    m3, m7
    packssdw   m0, m2
    packssdw   m1, m3
    SWAP        4, 0
    SWAP        5, 1
    add        r3, r4
ALIGN 4
.loop4:
%ifdef HIGH_BIT_DEPTH
    UNPACK_UNALIGNED m0, m1, m2, r3
    pmaddwd    m0, m7
    pmaddwd    m1, m7
    packssdw   m0, m1
    UNPACK_UNALIGNED m1, m2, m3, r3+mmsize
    pmaddwd    m1, m7
    pmaddwd    m2, m7
    packssdw   m1, m2
%else ; !HIGH_BIT_DEPTH
    movu       m0, [r3]
    movu       m1, [r3+mmsize/2]
    UNPACK_UNALIGNED m0, m2, [r3+2]
    UNPACK_UNALIGNED m1, m3, [r3+2+mmsize/2]
    psrlw      m2, m0, 8
    psrlw      m3, m1, 8
    pand       m0, [pw_00ff]
    pand       m1, [pw_00ff]
    pmaddwd    m0, m7
    pmaddwd    m2, m7
    pmaddwd    m1, m7
    pmaddwd    m3, m7
    packssdw   m0, m2
    packssdw   m1, m3
%endif ; HIGH_BIT_DEPTH
    pmullw     m4, m6
    pmullw     m5, m6
    mova       m2, [pw_32]
    paddw      m3, m2, m5
    paddw      m2, m4
    mova       m4, m0
    mova       m5, m1
    pmullw     m0, multy0
    pmullw     m1, multy0
    paddw      m0, m2
    paddw      m1, m3
    psrlw      m0, 6
    psrlw      m1, 6
%ifdef HIGH_BIT_DEPTH
    movh     [r0], m0
    movh     [r0+mmsize/2], m1
%if mmsize==8
    psrlq      m0, 32
    psrlq      m1, 32
    movh     [r1], m0
    movh     [r1+mmsize/2], m1
%else
    movhps   [r1], m0
    movhps   [r1+mmsize/2], m1
%endif
%else ; !HIGH_BIT_DEPTH
    packuswb   m0, m1
%if mmsize==8
    pshufw     m1, m0, q0020
    pshufw     m0, m0, q0031
    movd     [r0], m1
    movd     [r1], m0
%else
    pshufd     m0, m0, q3120
    movq     [r0], m0
    movhps   [r1], m0
%endif
%endif ; HIGH_BIT_DEPTH
    add        r3, r4
    add        r0, r2
    add        r1, r2
    dec       r5d
    jg .loop4
%if mmsize!=8
    REP_RET
%else
    sub dword r7m, 4
    jg .width8
    REP_RET
.width8:
%ifdef ARCH_X86_64
    lea        r3, [t2+8*SIZEOF_PIXEL]
    lea        r0, [t0+4*SIZEOF_PIXEL]
    lea        r1, [t1+4*SIZEOF_PIXEL]
%else
    mov        r3, r3m
    mov        r0, r0m
    mov        r1, r1m
    add        r3, 8*SIZEOF_PIXEL
    add        r0, 4*SIZEOF_PIXEL
    add        r1, 4*SIZEOF_PIXEL
%endif
    mov       r5d, r8m
    jmp .loopx
%endif

%ifdef ARCH_X86_64 ; too many regs for x86_32
    RESET_MM_PERMUTATION
%ifdef WIN64
%if xmm_regs_used > 6
    %assign stack_offset stack_offset-(xmm_regs_used-6)*16-16
    %assign xmm_regs_used 6
%endif
%endif
.mc1dy:
    and       t2d, 7
    movd       m5, t2d
    mov       r6d, r4d ; pel_offset = dx ? 2 : src_stride
    jmp .mc1d
.mc1dx:
    movd       m5, r5d
    mov       r6d, 2*SIZEOF_PIXEL
.mc1d:
%ifdef HIGH_BIT_DEPTH
%if mmsize == 16
    WIN64_SPILL_XMM 8
%endif
%endif
    mova       m4, [pw_8]
    SPLATW     m5, m5
    psubw      m4, m5
    movifnidn  r0, r0mp
    movifnidn  r1, r1mp
    movifnidn r2d, r2m
    FIX_STRIDES r2
    movifnidn r5d, r8m
    cmp dword r7m, 4
    jg .mc1d_w8
    mov       r10, r2
    mov       r11, r4
%if mmsize!=8
    shr       r5d, 1
%endif
.loop1d_w4:
%ifdef HIGH_BIT_DEPTH
%if mmsize == 8
    movq       m0, [r3+0]
    movq       m2, [r3+8]
    movq       m1, [r3+r6+0]
    movq       m3, [r3+r6+8]
%else
    movu       m0, [r3]
    movu       m1, [r3+r6]
    add        r3, r11
    movu       m2, [r3]
    movu       m3, [r3+r6]
%endif
    SBUTTERFLY wd, 0, 2, 6
    SBUTTERFLY wd, 1, 3, 7
    SBUTTERFLY wd, 0, 2, 6
    SBUTTERFLY wd, 1, 3, 7
%if mmsize == 16
    SBUTTERFLY wd, 0, 2, 6
    SBUTTERFLY wd, 1, 3, 7
%endif
%else ; !HIGH_BIT_DEPTH
    movq       m0, [r3]
    movq       m1, [r3+r6]
%if mmsize!=8
    add        r3, r11
    movhps     m0, [r3]
    movhps     m1, [r3+r6]
%endif
    psrlw      m2, m0, 8
    psrlw      m3, m1, 8
    pand       m0, [pw_00ff]
    pand       m1, [pw_00ff]
%endif ; HIGH_BIT_DEPTH
    pmullw     m0, m4
    pmullw     m1, m5
    pmullw     m2, m4
    pmullw     m3, m5
    paddw      m0, [pw_4]
    paddw      m2, [pw_4]
    paddw      m0, m1
    paddw      m2, m3
    psrlw      m0, 3
    psrlw      m2, 3
%ifdef HIGH_BIT_DEPTH
%if mmsize == 8
    xchg       r4, r11
    xchg       r2, r10
%endif
    movq     [r0], m0
    movq     [r1], m2
%if mmsize == 16
    add        r0, r10
    add        r1, r10
    movhps   [r0], m0
    movhps   [r1], m2
%endif
%else ; !HIGH_BIT_DEPTH
    packuswb   m0, m2
%if mmsize==8
    xchg       r4, r11
    xchg       r2, r10
    movd     [r0], m0
    psrlq      m0, 32
    movd     [r1], m0
%else
    movhlps    m1, m0
    movd     [r0], m0
    movd     [r1], m1
    add        r0, r10
    add        r1, r10
    psrldq     m0, 4
    psrldq     m1, 4
    movd     [r0], m0
    movd     [r1], m1
%endif
%endif ; HIGH_BIT_DEPTH
    add        r3, r4
    add        r0, r2
    add        r1, r2
    dec       r5d
    jg .loop1d_w4
    REP_RET
.mc1d_w8:
    sub       r2, 4*SIZEOF_PIXEL
    sub       r4, 8*SIZEOF_PIXEL
    mov      r10, 4*SIZEOF_PIXEL
    mov      r11, 8*SIZEOF_PIXEL
%if mmsize==8
    shl       r5d, 1
%endif
    jmp .loop1d_w4
%endif ; ARCH_X86_64
%endmacro ; MC_CHROMA


%macro MC_CHROMA_SSSE3 0
cglobal mc_chroma, 0,6,9
    MC_CHROMA_START
    and       r5d, 7
    and       t2d, 7
    mov       t0d, r5d
    shl       t0d, 8
    sub       t0d, r5d
    mov       r5d, 8
    add       t0d, 8
    sub       r5d, t2d
    imul      t2d, t0d ; (x*255+8)*y
    imul      r5d, t0d ; (x*255+8)*(8-y)
    movd       m6, t2d
    movd       m7, r5d
%if cpuflag(cache64)
    mov       t0d, r3d
    and       t0d, 7
%ifdef PIC
    lea        t1, [ch_shuf_adj]
    movddup    m5, [t1 + t0*4]
%else
    movddup    m5, [ch_shuf_adj + t0*4]
%endif
    paddb      m5, [ch_shuf]
    and        r3, ~7
%else
    mova       m5, [ch_shuf]
%endif
    movifnidn  r0, r0mp
    movifnidn  r1, r1mp
    movifnidn r2d, r2m
    movifnidn r5d, r8m
    SPLATW     m6, m6
    SPLATW     m7, m7
    cmp dword r7m, 4
    jg .width8
    movu       m0, [r3]
    pshufb     m0, m5
.loop4:
    movu       m1, [r3+r4]
    pshufb     m1, m5
    movu       m3, [r3+r4*2]
    pshufb     m3, m5
    mova       m4, m3
    pmaddubsw  m0, m7
    pmaddubsw  m2, m1, m7
    pmaddubsw  m1, m6
    pmaddubsw  m3, m6
    paddw      m0, [pw_32]
    paddw      m2, [pw_32]
    paddw      m1, m0
    paddw      m3, m2
    mova       m0, m4
    psrlw      m1, 6
    psrlw      m3, 6
    packuswb   m1, m3
    movhlps    m3, m1
    movd     [r0], m1
    movd  [r0+r2], m3
    psrldq     m1, 4
    psrldq     m3, 4
    movd     [r1], m1
    movd  [r1+r2], m3
    lea        r3, [r3+r4*2]
    lea        r0, [r0+r2*2]
    lea        r1, [r1+r2*2]
    sub       r5d, 2
    jg .loop4
    REP_RET

.width8:
    movu       m0, [r3]
    pshufb     m0, m5
    movu       m1, [r3+8]
    pshufb     m1, m5
%ifdef ARCH_X86_64
    SWAP        8, 6
    %define  mult1 m8
%else
    mova      r0m, m6
    %define  mult1 r0m
%endif
.loop8:
    movu       m2, [r3+r4]
    pshufb     m2, m5
    movu       m3, [r3+r4+8]
    pshufb     m3, m5
    mova       m4, m2
    mova       m6, m3
    pmaddubsw  m0, m7
    pmaddubsw  m1, m7
    pmaddubsw  m2, mult1
    pmaddubsw  m3, mult1
    paddw      m0, [pw_32]
    paddw      m1, [pw_32]
    paddw      m0, m2
    paddw      m1, m3
    psrlw      m0, 6
    psrlw      m1, 6
    packuswb   m0, m1
    pshufd     m0, m0, q3120
    movq     [r0], m0
    movhps   [r1], m0

    movu       m2, [r3+r4*2]
    pshufb     m2, m5
    movu       m3, [r3+r4*2+8]
    pshufb     m3, m5
    mova       m0, m2
    mova       m1, m3
    pmaddubsw  m4, m7
    pmaddubsw  m6, m7
    pmaddubsw  m2, mult1
    pmaddubsw  m3, mult1
    paddw      m4, [pw_32]
    paddw      m6, [pw_32]
    paddw      m2, m4
    paddw      m3, m6
    psrlw      m2, 6
    psrlw      m3, 6
    packuswb   m2, m3
    pshufd     m2, m2, q3120
    movq   [r0+r2], m2
    movhps [r1+r2], m2
    lea        r3, [r3+r4*2]
    lea        r0, [r0+r2*2]
    lea        r1, [r1+r2*2]
    sub       r5d, 2
    jg .loop8
    REP_RET
%endmacro

%ifdef HIGH_BIT_DEPTH
INIT_MMX mmx2
MC_CHROMA
INIT_XMM sse2
MC_CHROMA
INIT_XMM avx
MC_CHROMA
%else ; !HIGH_BIT_DEPTH
INIT_MMX mmx2
MC_CHROMA
INIT_XMM sse2, misalign
MC_CHROMA
INIT_XMM sse2
MC_CHROMA
INIT_XMM ssse3
MC_CHROMA_SSSE3
INIT_XMM ssse3, cache64
MC_CHROMA_SSSE3
INIT_XMM avx
MC_CHROMA_SSSE3 ; No known AVX CPU will trigger CPU_CACHELINE_64
%endif ; HIGH_BIT_DEPTH
