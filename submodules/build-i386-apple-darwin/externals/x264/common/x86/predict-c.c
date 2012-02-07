/*****************************************************************************
 * predict-c.c: intra prediction
 *****************************************************************************
 * Copyright (C) 2003-2011 x264 project
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Loren Merritt <lorenm@u.washington.edu>
 *          Jason Garrett-Glaser <darkshikari@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at licensing@x264.com.
 *****************************************************************************/

#include "common/common.h"
#include "predict.h"
#include "pixel.h"

 void x264_predict_16x16_v_mmx( pixel *src );
 void x264_predict_16x16_v_sse2( pixel *src );
 void x264_predict_16x16_h_mmx2( pixel *src );
 void x264_predict_16x16_h_sse2( uint16_t *src );
 void x264_predict_16x16_h_ssse3( uint8_t *src );
 void x264_predict_16x16_dc_core_mmx2( pixel *src, int i_dc_left );
 void x264_predict_16x16_dc_core_sse2( pixel *src, int i_dc_left );
 void x264_predict_16x16_dc_left_core_mmx2( pixel *src, int i_dc_left );
 void x264_predict_16x16_dc_left_core_sse2( pixel *src, int i_dc_left );
 void x264_predict_16x16_dc_top_mmx2( pixel *src );
 void x264_predict_16x16_dc_top_sse2( pixel *src );
 void x264_predict_16x16_dc_top_ssse3( uint16_t *src );
 void x264_predict_16x16_p_core_mmx2( uint8_t *src, int i00, int b, int c );
 void x264_predict_16x16_p_core_sse2( pixel *src, int i00, int b, int c );
 void x264_predict_16x16_p_core_avx( pixel *src, int i00, int b, int c );
 void x264_predict_8x8c_p_core_mmx2( uint8_t *src, int i00, int b, int c );
 void x264_predict_8x8c_p_core_sse2( pixel *src, int i00, int b, int c );
 void x264_predict_8x8c_dc_mmx2( pixel *src );
 void x264_predict_8x8c_dc_sse2( uint16_t *src );
 void x264_predict_8x8c_dc_top_mmx2( uint8_t *src );
 void x264_predict_8x8c_dc_top_sse2( uint16_t *src );
 void x264_predict_8x8c_v_mmx( pixel *src );
 void x264_predict_8x8c_v_sse2( uint16_t *src );
 void x264_predict_8x8c_h_mmx2( uint8_t *src );
 void x264_predict_8x8c_h_sse2( pixel *src );
 void x264_predict_8x8c_h_ssse3( uint8_t *src );
 void x264_predict_8x8_v_mmx2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_v_sse2( uint16_t *src, uint16_t edge[36] );
 void x264_predict_8x8_h_mmx2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_h_sse2( uint16_t *src, uint16_t edge[36] );
 void x264_predict_8x8_hd_mmx2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_hu_mmx2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_dc_mmx2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_dc_sse2( uint16_t *src, uint16_t edge[36] );
 void x264_predict_8x8_dc_top_mmx2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_dc_top_sse2( uint16_t *src, uint16_t edge[36] );
 void x264_predict_8x8_dc_left_mmx2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_dc_left_sse2( uint16_t *src, uint16_t edge[36] );
 void x264_predict_8x8_ddl_mmx2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_ddl_sse2( pixel *src, pixel edge[36] );
 void x264_predict_8x8_ddl_avx( pixel *src, pixel edge[36] );
 void x264_predict_8x8_ddr_mmx2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_ddr_sse2( pixel *src, pixel edge[36] );
 void x264_predict_8x8_ddr_avx( pixel *src, pixel edge[36] );
 void x264_predict_8x8_vl_sse2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_vl_avx( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_vr_mmx2( uint8_t *src, uint8_t edge[36] );
 void x264_predict_8x8_vr_sse2( pixel *src, pixel edge[36] );
 void x264_predict_8x8_vr_ssse3( uint16_t *src, uint16_t edge[36] );
 void x264_predict_8x8_vr_avx( pixel *src, pixel edge[36] );
 void x264_predict_8x8_hu_sse2( pixel *src, pixel edge[36] );
 void x264_predict_8x8_hu_ssse3( pixel *src, pixel edge[36] );
 void x264_predict_8x8_hu_avx( pixel *src, pixel edge[36] );
 void x264_predict_8x8_hd_sse2( pixel *src, pixel edge[36] );
 void x264_predict_8x8_hd_ssse3( pixel *src, pixel edge[36] );
 void x264_predict_8x8_hd_avx( pixel *src, pixel edge[36] );
 void x264_predict_8x8_filter_mmx2( uint8_t *src, uint8_t edge[36], int i_neighbor, int i_filters );
 void x264_predict_8x8_filter_sse2( uint16_t *src, uint16_t edge[36], int i_neighbor, int i_filters );
 void x264_predict_8x8_filter_ssse3( pixel *src, pixel edge[36], int i_neighbor, int i_filters );
 void x264_predict_8x8_filter_avx( uint16_t *src, uint16_t edge[36], int i_neighbor, int i_filters );
 void x264_predict_4x4_ddl_mmx2( pixel *src );
 void x264_predict_4x4_ddl_sse2( uint16_t *src );
 void x264_predict_4x4_ddl_avx( uint16_t *src );
 void x264_predict_4x4_ddr_mmx2( pixel *src );
 void x264_predict_4x4_vl_mmx2( pixel *src );
 void x264_predict_4x4_vl_sse2( uint16_t *src );
 void x264_predict_4x4_vl_avx( uint16_t *src );
 void x264_predict_4x4_vr_mmx2( uint8_t *src );
 void x264_predict_4x4_vr_sse2( uint16_t *src );
 void x264_predict_4x4_vr_ssse3( pixel *src );
 void x264_predict_4x4_vr_avx( uint16_t *src );
 void x264_predict_4x4_hd_mmx2( pixel *src );
 void x264_predict_4x4_hd_sse2( uint16_t *src );
 void x264_predict_4x4_hd_ssse3( pixel *src );
 void x264_predict_4x4_hd_avx( uint16_t *src );
 void x264_predict_4x4_dc_mmx2( pixel *src );
 void x264_predict_4x4_ddr_sse2( uint16_t *src );
 void x264_predict_4x4_ddr_ssse3( pixel *src );
 void x264_predict_4x4_ddr_avx( uint16_t *src );
 void x264_predict_4x4_hu_mmx2( pixel *src );

#define PREDICT_16x16_DC(name)\
static void x264_predict_16x16_dc_##name( pixel *src )\
{\
    uint32_t dc = 16;\
    for( int i = 0; i < 16; i += 2 )\
    {\
        dc += src[-1 + i * FDEC_STRIDE];\
        dc += src[-1 + (i+1) * FDEC_STRIDE];\
    }\
    x264_predict_16x16_dc_core_##name( src, dc );\
}

PREDICT_16x16_DC( mmx2 )
PREDICT_16x16_DC( sse2 )

#define PREDICT_16x16_DC_LEFT(name)\
static void x264_predict_16x16_dc_left_##name( pixel *src )\
{\
    uint32_t dc = 8;\
    for( int i = 0; i < 16; i += 2 )\
    {\
        dc += src[-1 + i * FDEC_STRIDE];\
        dc += src[-1 + (i+1) * FDEC_STRIDE];\
    }\
    x264_predict_16x16_dc_left_core_##name( src, dc>>4 );\
}

PREDICT_16x16_DC_LEFT( mmx2 )
PREDICT_16x16_DC_LEFT( sse2 )

#define PREDICT_P_SUM(j,i)\
    H += i * ( src[j+i - FDEC_STRIDE ]  - src[j-i - FDEC_STRIDE ] );\
    V += i * ( src[(j+i)*FDEC_STRIDE -1] - src[(j-i)*FDEC_STRIDE -1] );\

ALIGNED_16( static const int16_t pw_12345678[8] ) = {1,2,3,4,5,6,7,8};
ALIGNED_16( static const int16_t pw_m87654321[8] ) = {-8,-7,-6,-5,-4,-3,-2,-1};
ALIGNED_16( static const int16_t pw_m32101234[8] ) = {-3,-2,-1,0,1,2,3,4};
ALIGNED_8( static const int8_t pb_12345678[8] ) = {1,2,3,4,5,6,7,8};
ALIGNED_8( static const int8_t pb_m87654321[8] ) = {-8,-7,-6,-5,-4,-3,-2,-1};
ALIGNED_8( static const int8_t pb_m32101234[8] ) = {-3,-2,-1,0,1,2,3,4};

#if !HIGH_BIT_DEPTH
#define PREDICT_16x16_P(name)\
static void x264_predict_16x16_p_##name( pixel *src )\
{\
    int a, b, c;\
    int H = 0;\
    int V = 0;\
    int i00;\
    PREDICT_P_SUM(7,1) \
    PREDICT_P_SUM(7,2) \
    PREDICT_P_SUM(7,3) \
    PREDICT_P_SUM(7,4) \
    PREDICT_P_SUM(7,5) \
    PREDICT_P_SUM(7,6) \
    PREDICT_P_SUM(7,7) \
    PREDICT_P_SUM(7,8) \
    a = 16 * ( src[15*FDEC_STRIDE -1] + src[15 - FDEC_STRIDE] );\
    b = ( 5 * H + 32 ) >> 6;\
    c = ( 5 * V + 32 ) >> 6;\
    i00 = a - b * 7 - c * 7 + 16;\
    x264_predict_16x16_p_core_##name( src, i00, b, c );\
}
#ifndef ARCH_X86_64
PREDICT_16x16_P( mmx2 )
#endif
PREDICT_16x16_P( sse2   )
PREDICT_16x16_P( avx    )
#endif //!HIGH_BIT_DEPTH

#if HAVE_X86_INLINE_ASM
#if HIGH_BIT_DEPTH
static void x264_predict_16x16_p_sse2( uint16_t *src )
#else
static void x264_predict_16x16_p_ssse3( uint8_t *src )
#endif
{
    int a, b, c, i00;
    int H, V;
#if HIGH_BIT_DEPTH
    asm (
        "movdqu           %1, %%xmm1 \n"
        "movdqa           %2, %%xmm0 \n"
        "pmaddwd          %3, %%xmm0 \n"
        "pmaddwd          %4, %%xmm1 \n"
        "paddd        %%xmm1, %%xmm0 \n"
        "movhlps      %%xmm0, %%xmm1 \n"
        "paddd        %%xmm1, %%xmm0 \n"
        "pshuflw $14, %%xmm0, %%xmm1 \n"
        "paddd        %%xmm1, %%xmm0 \n"
        "movd         %%xmm0, %0     \n"
        :"=r"(H)
        :"m"(src[-FDEC_STRIDE-1]), "m"(src[-FDEC_STRIDE+8]),
         "m"(*pw_12345678), "m"(*pw_m87654321)
    );
#else
    asm (
        "movq           %1, %%mm1 \n"
        "movq           %2, %%mm0 \n"
        "palignr $7,    %3, %%mm1 \n"
        "pmaddubsw      %4, %%mm0 \n"
        "pmaddubsw      %5, %%mm1 \n"
        "paddw       %%mm1, %%mm0 \n"
        "pshufw $14, %%mm0, %%mm1 \n"
        "paddw       %%mm1, %%mm0 \n"
        "pshufw  $1, %%mm0, %%mm1 \n"
        "paddw       %%mm1, %%mm0 \n"
        "movd        %%mm0, %0    \n"
        "movswl        %w0, %0    \n"
        :"=r"(H)
        :"m"(src[-FDEC_STRIDE]), "m"(src[-FDEC_STRIDE+8]),
         "m"(src[-FDEC_STRIDE-8]), "m"(*pb_12345678), "m"(*pb_m87654321)
    );
#endif
    V = 8 * ( src[15*FDEC_STRIDE-1] - src[-1*FDEC_STRIDE-1] )
      + 7 * ( src[14*FDEC_STRIDE-1] - src[ 0*FDEC_STRIDE-1] )
      + 6 * ( src[13*FDEC_STRIDE-1] - src[ 1*FDEC_STRIDE-1] )
      + 5 * ( src[12*FDEC_STRIDE-1] - src[ 2*FDEC_STRIDE-1] )
      + 4 * ( src[11*FDEC_STRIDE-1] - src[ 3*FDEC_STRIDE-1] )
      + 3 * ( src[10*FDEC_STRIDE-1] - src[ 4*FDEC_STRIDE-1] )
      + 2 * ( src[ 9*FDEC_STRIDE-1] - src[ 5*FDEC_STRIDE-1] )
      + 1 * ( src[ 8*FDEC_STRIDE-1] - src[ 6*FDEC_STRIDE-1] );
    a = 16 * ( src[15*FDEC_STRIDE -1] + src[15 - FDEC_STRIDE] );
    b = ( 5 * H + 32 ) >> 6;
    c = ( 5 * V + 32 ) >> 6;
    i00 = a - b * 7 - c * 7 + 16;
    /* b*15 + c*15 can overflow: it's easier to just branch away in this rare case
     * than to try to consider it in the asm. */
    if( BIT_DEPTH > 8 && (i00 > 0x7fff || abs(b) > 1092 || abs(c) > 1092) )
        x264_predict_16x16_p_c( src );
    else
        x264_predict_16x16_p_core_sse2( src, i00, b, c );
}
#endif

#if !HIGH_BIT_DEPTH

#define PREDICT_8x8_P(name)\
static void x264_predict_8x8c_p_##name( uint8_t *src )\
{\
    int a, b, c;\
    int H = 0;\
    int V = 0;\
    int i00;\
    PREDICT_P_SUM(3,1)\
    PREDICT_P_SUM(3,2)\
    PREDICT_P_SUM(3,3)\
    PREDICT_P_SUM(3,4)\
    a = 16 * ( src[7*FDEC_STRIDE -1] + src[7 - FDEC_STRIDE] );\
    b = ( 17 * H + 16 ) >> 5;\
    c = ( 17 * V + 16 ) >> 5;\
    i00 = a -3*b -3*c + 16;\
    x264_predict_8x8c_p_core_##name( src, i00, b, c );\
}
#ifndef ARCH_X86_64
PREDICT_8x8_P( mmx2 )
#endif
PREDICT_8x8_P( sse2   )

#endif //!HIGH_BIT_DEPTH

#if HAVE_X86_INLINE_ASM
#if HIGH_BIT_DEPTH
static void x264_predict_8x8c_p_sse2( uint16_t *src )
#else
static void x264_predict_8x8c_p_ssse3( uint8_t *src )
#endif
{
    int a, b, c, i00;
    int H, V;
#if HIGH_BIT_DEPTH
    asm (
        "movdqa           %1, %%xmm0 \n"
        "pmaddwd          %2, %%xmm0 \n"
        "movhlps      %%xmm0, %%xmm1 \n"
        "paddd        %%xmm1, %%xmm0 \n"
        "pshuflw $14, %%xmm0, %%xmm1 \n"
        "paddd        %%xmm1, %%xmm0 \n"
        "movd         %%xmm0, %0     \n"
        :"=r"(H)
        :"m"(src[-FDEC_STRIDE]), "m"(*pw_m32101234)
    );
#else
    asm (
        "movq           %1, %%mm0 \n"
        "pmaddubsw      %2, %%mm0 \n"
        "pshufw $14, %%mm0, %%mm1 \n"
        "paddw       %%mm1, %%mm0 \n"
        "pshufw  $1, %%mm0, %%mm1 \n"
        "paddw       %%mm1, %%mm0 \n"
        "movd        %%mm0, %0    \n"
        "movswl        %w0, %0    \n"
        :"=r"(H)
        :"m"(src[-FDEC_STRIDE]), "m"(*pb_m32101234)
    );
#endif
    V = 1 * ( src[4*FDEC_STRIDE -1] - src[ 2*FDEC_STRIDE -1] )
      + 2 * ( src[5*FDEC_STRIDE -1] - src[ 1*FDEC_STRIDE -1] )
      + 3 * ( src[6*FDEC_STRIDE -1] - src[ 0*FDEC_STRIDE -1] )
      + 4 * ( src[7*FDEC_STRIDE -1] - src[-1*FDEC_STRIDE -1] );
    H += -4 * src[-1*FDEC_STRIDE -1];
    a = 16 * ( src[7*FDEC_STRIDE -1] + src[7 - FDEC_STRIDE] );
    b = ( 17 * H + 16 ) >> 5;
    c = ( 17 * V + 16 ) >> 5;
    i00 = a -3*b -3*c + 16;
#if HIGH_BIT_DEPTH
    x264_predict_8x8c_p_core_sse2( src, a, b, c );
#else
    x264_predict_8x8c_p_core_sse2( src, i00, b, c );
#endif
}
#endif

#if ARCH_X86_64 && !HIGH_BIT_DEPTH
static void x264_predict_8x8c_dc_left( uint8_t *src )
{
    int y;
    uint32_t s0 = 0, s1 = 0;
    uint64_t dc0, dc1;

    for( y = 0; y < 4; y++ )
    {
        s0 += src[y * FDEC_STRIDE     - 1];
        s1 += src[(y+4) * FDEC_STRIDE - 1];
    }
    dc0 = (( s0 + 2 ) >> 2) * 0x0101010101010101ULL;
    dc1 = (( s1 + 2 ) >> 2) * 0x0101010101010101ULL;

    for( y = 0; y < 4; y++ )
    {
        M64( src ) = dc0;
        src += FDEC_STRIDE;
    }
    for( y = 0; y < 4; y++ )
    {
        M64( src ) = dc1;
        src += FDEC_STRIDE;
    }

}
#endif // ARCH_X86_64 && !HIGH_BIT_DEPTH

/****************************************************************************
 * Exported functions:
 ****************************************************************************/
void x264_predict_16x16_init_mmx( int cpu, x264_predict_t pf[7] )
{
    if( !(cpu&X264_CPU_MMX) )
        return;
    pf[I_PRED_16x16_V]       = x264_predict_16x16_v_mmx;
    if( cpu&X264_CPU_MMX2 )
    {
        pf[I_PRED_16x16_DC]      = x264_predict_16x16_dc_mmx2;
        pf[I_PRED_16x16_DC_TOP]  = x264_predict_16x16_dc_top_mmx2;
        pf[I_PRED_16x16_DC_LEFT] = x264_predict_16x16_dc_left_mmx2;
        pf[I_PRED_16x16_H]       = x264_predict_16x16_h_mmx2;
    }
#if HIGH_BIT_DEPTH
    if( !(cpu&X264_CPU_SSE2) )
        return;
    pf[I_PRED_16x16_DC]      = x264_predict_16x16_dc_sse2;
    pf[I_PRED_16x16_DC_TOP]  = x264_predict_16x16_dc_top_sse2;
    pf[I_PRED_16x16_DC_LEFT] = x264_predict_16x16_dc_left_sse2;
    pf[I_PRED_16x16_V]       = x264_predict_16x16_v_sse2;
    pf[I_PRED_16x16_H]       = x264_predict_16x16_h_sse2;
#if HAVE_X86_INLINE_ASM
    pf[I_PRED_16x16_P]       = x264_predict_16x16_p_sse2;
#endif
#else
#if !ARCH_X86_64
    pf[I_PRED_16x16_P]       = x264_predict_16x16_p_mmx2;
#endif
    if( !(cpu&X264_CPU_SSE2) )
        return;
    pf[I_PRED_16x16_DC]      = x264_predict_16x16_dc_sse2;
    pf[I_PRED_16x16_V]       = x264_predict_16x16_v_sse2;
    if( cpu&X264_CPU_SSE2_IS_SLOW )
        return;
    pf[I_PRED_16x16_DC_TOP]  = x264_predict_16x16_dc_top_sse2;
    pf[I_PRED_16x16_DC_LEFT] = x264_predict_16x16_dc_left_sse2;
    pf[I_PRED_16x16_P]       = x264_predict_16x16_p_sse2;
    if( !(cpu&X264_CPU_SSSE3) )
        return;
    pf[I_PRED_16x16_H]       = x264_predict_16x16_h_ssse3;
#if HAVE_X86_INLINE_ASM
    pf[I_PRED_16x16_P]       = x264_predict_16x16_p_ssse3;
#endif
    if( !(cpu&X264_CPU_AVX) )
        return;
    pf[I_PRED_16x16_P]       = x264_predict_16x16_p_avx;
#endif // HIGH_BIT_DEPTH
}

void x264_predict_8x8c_init_mmx( int cpu, x264_predict_t pf[7] )
{
    if( !(cpu&X264_CPU_MMX) )
        return;
#if HIGH_BIT_DEPTH
    pf[I_PRED_CHROMA_V]       = x264_predict_8x8c_v_mmx;
    if( !(cpu&X264_CPU_MMX2) )
        return;
    pf[I_PRED_CHROMA_DC]      = x264_predict_8x8c_dc_mmx2;
    if( !(cpu&X264_CPU_SSE2) )
        return;
    pf[I_PRED_CHROMA_V]       = x264_predict_8x8c_v_sse2;
    pf[I_PRED_CHROMA_DC]      = x264_predict_8x8c_dc_sse2;
    pf[I_PRED_CHROMA_DC_TOP]  = x264_predict_8x8c_dc_top_sse2;
    pf[I_PRED_CHROMA_H]       = x264_predict_8x8c_h_sse2;
#if HAVE_X86_INLINE_ASM
    pf[I_PRED_CHROMA_P]       = x264_predict_8x8c_p_sse2;
#endif
#else
#if ARCH_X86_64
    pf[I_PRED_CHROMA_DC_LEFT] = x264_predict_8x8c_dc_left;
#endif
    pf[I_PRED_CHROMA_V]       = x264_predict_8x8c_v_mmx;
    if( !(cpu&X264_CPU_MMX2) )
        return;
    pf[I_PRED_CHROMA_DC_TOP]  = x264_predict_8x8c_dc_top_mmx2;
    pf[I_PRED_CHROMA_H]       = x264_predict_8x8c_h_mmx2;
#if !ARCH_X86_64
    pf[I_PRED_CHROMA_P]       = x264_predict_8x8c_p_mmx2;
#endif
    pf[I_PRED_CHROMA_DC]      = x264_predict_8x8c_dc_mmx2;
    if( !(cpu&X264_CPU_SSE2) )
        return;
    pf[I_PRED_CHROMA_P]       = x264_predict_8x8c_p_sse2;
    if( !(cpu&X264_CPU_SSSE3) )
        return;
    pf[I_PRED_CHROMA_H]       = x264_predict_8x8c_h_ssse3;
#if HAVE_X86_INLINE_ASM
    pf[I_PRED_CHROMA_P]       = x264_predict_8x8c_p_ssse3;
#endif
#endif // HIGH_BIT_DEPTH
}

void x264_predict_8x8_init_mmx( int cpu, x264_predict8x8_t pf[12], x264_predict_8x8_filter_t *predict_8x8_filter )
{
    if( !(cpu&X264_CPU_MMX2) )
        return;
#if HIGH_BIT_DEPTH
    if( !(cpu&X264_CPU_SSE2) )
        return;
    pf[I_PRED_8x8_V]      = x264_predict_8x8_v_sse2;
    pf[I_PRED_8x8_H]      = x264_predict_8x8_h_sse2;
    pf[I_PRED_8x8_DC]     = x264_predict_8x8_dc_sse2;
    pf[I_PRED_8x8_DC_TOP] = x264_predict_8x8_dc_top_sse2;
    pf[I_PRED_8x8_DC_LEFT]= x264_predict_8x8_dc_left_sse2;
    pf[I_PRED_8x8_DDL]    = x264_predict_8x8_ddl_sse2;
    pf[I_PRED_8x8_DDR]    = x264_predict_8x8_ddr_sse2;
    pf[I_PRED_8x8_VR]     = x264_predict_8x8_vr_sse2;
    pf[I_PRED_8x8_HD]     = x264_predict_8x8_hd_sse2;
    pf[I_PRED_8x8_HU]     = x264_predict_8x8_hu_sse2;
    *predict_8x8_filter   = x264_predict_8x8_filter_sse2;
    if( !(cpu&X264_CPU_SSSE3) )
        return;
    pf[I_PRED_8x8_HD]     = x264_predict_8x8_hd_ssse3;
    pf[I_PRED_8x8_HU]     = x264_predict_8x8_hu_ssse3;
    pf[I_PRED_8x8_VR]     = x264_predict_8x8_vr_ssse3;
    *predict_8x8_filter   = x264_predict_8x8_filter_ssse3;
    if( !(cpu&X264_CPU_AVX) )
        return;
    pf[I_PRED_8x8_DDL]    = x264_predict_8x8_ddl_avx;
    pf[I_PRED_8x8_HD]     = x264_predict_8x8_hd_avx;
    pf[I_PRED_8x8_HU]     = x264_predict_8x8_hu_avx;
    pf[I_PRED_8x8_VR]     = x264_predict_8x8_vr_avx;
    *predict_8x8_filter   = x264_predict_8x8_filter_avx;
#else
    pf[I_PRED_8x8_V]      = x264_predict_8x8_v_mmx2;
    pf[I_PRED_8x8_H]      = x264_predict_8x8_h_mmx2;
    pf[I_PRED_8x8_DC]     = x264_predict_8x8_dc_mmx2;
    pf[I_PRED_8x8_DC_TOP] = x264_predict_8x8_dc_top_mmx2;
    pf[I_PRED_8x8_DC_LEFT]= x264_predict_8x8_dc_left_mmx2;
    pf[I_PRED_8x8_HD]     = x264_predict_8x8_hd_mmx2;
    *predict_8x8_filter   = x264_predict_8x8_filter_mmx2;
#if ARCH_X86
    pf[I_PRED_8x8_DDL]  = x264_predict_8x8_ddl_mmx2;
    pf[I_PRED_8x8_DDR]  = x264_predict_8x8_ddr_mmx2;
    pf[I_PRED_8x8_VR]   = x264_predict_8x8_vr_mmx2;
    pf[I_PRED_8x8_HU]   = x264_predict_8x8_hu_mmx2;
#endif
    if( !(cpu&X264_CPU_SSE2) )
        return;
    pf[I_PRED_8x8_DDL]  = x264_predict_8x8_ddl_sse2;
    pf[I_PRED_8x8_VL]   = x264_predict_8x8_vl_sse2;
    pf[I_PRED_8x8_VR]   = x264_predict_8x8_vr_sse2;
    pf[I_PRED_8x8_DDR]  = x264_predict_8x8_ddr_sse2;
    pf[I_PRED_8x8_HD]   = x264_predict_8x8_hd_sse2;
    pf[I_PRED_8x8_HU]   = x264_predict_8x8_hu_sse2;
    if( !(cpu&X264_CPU_SSSE3) )
        return;
    pf[I_PRED_8x8_HD]   = x264_predict_8x8_hd_ssse3;
    pf[I_PRED_8x8_HU]   = x264_predict_8x8_hu_ssse3;
    *predict_8x8_filter = x264_predict_8x8_filter_ssse3;
    if( !(cpu&X264_CPU_AVX) )
        return;
    pf[I_PRED_8x8_DDL]  = x264_predict_8x8_ddl_avx;
    pf[I_PRED_8x8_DDR]  = x264_predict_8x8_ddr_avx;
    pf[I_PRED_8x8_VL]   = x264_predict_8x8_vl_avx;
    pf[I_PRED_8x8_VR]   = x264_predict_8x8_vr_avx;
    pf[I_PRED_8x8_HD]   = x264_predict_8x8_hd_avx;
#endif // HIGH_BIT_DEPTH
}

void x264_predict_4x4_init_mmx( int cpu, x264_predict_t pf[12] )
{
    if( !(cpu&X264_CPU_MMX2) )
        return;
    pf[I_PRED_4x4_DC]  = x264_predict_4x4_dc_mmx2;
    pf[I_PRED_4x4_DDL] = x264_predict_4x4_ddl_mmx2;
    pf[I_PRED_4x4_DDR] = x264_predict_4x4_ddr_mmx2;
    pf[I_PRED_4x4_VL]  = x264_predict_4x4_vl_mmx2;
    pf[I_PRED_4x4_HD]  = x264_predict_4x4_hd_mmx2;
    pf[I_PRED_4x4_HU]  = x264_predict_4x4_hu_mmx2;
#if HIGH_BIT_DEPTH
    if( !(cpu&X264_CPU_SSE2) )
        return;
    pf[I_PRED_4x4_DDL] = x264_predict_4x4_ddl_sse2;
    pf[I_PRED_4x4_DDR] = x264_predict_4x4_ddr_sse2;
    pf[I_PRED_4x4_HD]  = x264_predict_4x4_hd_sse2;
    pf[I_PRED_4x4_VL]  = x264_predict_4x4_vl_sse2;
    pf[I_PRED_4x4_VR]  = x264_predict_4x4_vr_sse2;
    if( !(cpu&X264_CPU_AVX) )
        return;
    pf[I_PRED_4x4_DDL] = x264_predict_4x4_ddl_avx;
    pf[I_PRED_4x4_DDR] = x264_predict_4x4_ddr_avx;
    pf[I_PRED_4x4_HD]  = x264_predict_4x4_hd_avx;
#if ARCH_X86_64
    pf[I_PRED_4x4_VL]  = x264_predict_4x4_vl_avx;
#endif
    pf[I_PRED_4x4_VR]  = x264_predict_4x4_vr_avx;
#else
    pf[I_PRED_4x4_VR]  = x264_predict_4x4_vr_mmx2;
#endif // HIGH_BIT_DEPTH
    if( !(cpu&X264_CPU_SSSE3) )
        return;
    pf[I_PRED_4x4_DDR] = x264_predict_4x4_ddr_ssse3;
    pf[I_PRED_4x4_VR]  = x264_predict_4x4_vr_ssse3;
    pf[I_PRED_4x4_HD]  = x264_predict_4x4_hd_ssse3;
}
