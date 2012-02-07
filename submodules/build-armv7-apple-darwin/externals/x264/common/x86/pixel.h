/*****************************************************************************
 * pixel.h: x86 pixel metrics
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

#ifndef X264_I386_PIXEL_H
#define X264_I386_PIXEL_H

#define DECL_PIXELS( ret, name, suffix, args ) \
    ret x264_pixel_##name##_16x16_##suffix args;\
    ret x264_pixel_##name##_16x8_##suffix args;\
    ret x264_pixel_##name##_8x16_##suffix args;\
    ret x264_pixel_##name##_8x8_##suffix args;\
    ret x264_pixel_##name##_8x4_##suffix args;\
    ret x264_pixel_##name##_4x8_##suffix args;\
    ret x264_pixel_##name##_4x4_##suffix args;\

#define DECL_X1( name, suffix ) \
    DECL_PIXELS( int, name, suffix, ( pixel *, int, pixel *, int ) )

#define DECL_X4( name, suffix ) \
    DECL_PIXELS( void, name##_x3, suffix, ( pixel *, pixel *, pixel *, pixel *, int, int * ) )\
    DECL_PIXELS( void, name##_x4, suffix, ( pixel *, pixel *, pixel *, pixel *, pixel *, int, int * ) )

DECL_X1( sad, mmx2 )
DECL_X1( sad, sse2 )
DECL_X4( sad, sse2_misalign )
DECL_X1( sad, sse3 )
DECL_X1( sad, sse2_aligned )
DECL_X1( sad, ssse3 )
DECL_X1( sad, ssse3_aligned )
DECL_X4( sad, mmx2 )
DECL_X4( sad, sse2 )
DECL_X4( sad, sse3 )
DECL_X4( sad, ssse3 )
DECL_X1( ssd, mmx )
DECL_X1( ssd, mmx2 )
DECL_X1( ssd, sse2slow )
DECL_X1( ssd, sse2 )
DECL_X1( ssd, ssse3 )
DECL_X1( ssd, avx )
DECL_X1( satd, mmx2 )
DECL_X1( satd, sse2 )
DECL_X1( satd, ssse3 )
DECL_X1( satd, sse4 )
DECL_X1( satd, avx )
DECL_X1( sa8d, mmx2 )
DECL_X1( sa8d, sse2 )
DECL_X1( sa8d, ssse3 )
DECL_X1( sa8d, sse4 )
DECL_X1( sa8d, avx )
DECL_X1( sad, cache32_mmx2 );
DECL_X1( sad, cache64_mmx2 );
DECL_X1( sad, cache64_sse2 );
DECL_X1( sad, cache64_ssse3 );
DECL_X4( sad, cache32_mmx2 );
DECL_X4( sad, cache64_mmx2 );
DECL_X4( sad, cache64_sse2 );
DECL_X4( sad, cache64_ssse3 );

DECL_PIXELS( uint64_t, var, mmx2, ( pixel *pix, int i_stride ))
DECL_PIXELS( uint64_t, var, sse2, ( pixel *pix, int i_stride ))
DECL_PIXELS( uint64_t, var, avx,  ( pixel *pix, int i_stride ))
DECL_PIXELS( uint64_t, hadamard_ac, mmx2,  ( pixel *pix, int i_stride ))
DECL_PIXELS( uint64_t, hadamard_ac, sse2,  ( pixel *pix, int i_stride ))
DECL_PIXELS( uint64_t, hadamard_ac, ssse3, ( pixel *pix, int i_stride ))
DECL_PIXELS( uint64_t, hadamard_ac, sse4,  ( pixel *pix, int i_stride ))
DECL_PIXELS( uint64_t, hadamard_ac, avx,   ( pixel *pix, int i_stride ))


void x264_intra_satd_x3_4x4_mmx2   ( pixel   *, pixel   *, int * );
void x264_intra_satd_x3_4x4_ssse3  ( uint8_t *, uint8_t *, int * );
void x264_intra_sad_x3_4x4_mmx2    ( pixel   *, pixel   *, int * );
void x264_intra_sad_x3_4x4_sse4    ( uint8_t *, uint8_t *, int * );
void x264_intra_sad_x3_4x4_avx     ( uint8_t *, uint8_t *, int * );
void x264_intra_satd_x3_8x8c_mmx2  ( pixel   *, pixel   *, int * );
void x264_intra_satd_x3_8x8c_ssse3 ( uint8_t *, uint8_t *, int * );
void x264_intra_sad_x3_8x8c_mmx2   ( pixel   *, pixel   *, int * );
void x264_intra_sad_x3_8x8c_sse2   ( pixel   *, pixel   *, int * );
void x264_intra_sad_x3_8x8c_ssse3  ( pixel   *, pixel   *, int * );
void x264_intra_satd_x3_16x16_mmx2 ( pixel   *, pixel   *, int * );
void x264_intra_satd_x3_16x16_ssse3( uint8_t *, uint8_t *, int * );
void x264_intra_sad_x3_16x16_mmx2  ( pixel   *, pixel   *, int * );
void x264_intra_sad_x3_16x16_sse2  ( pixel   *, pixel   *, int * );
void x264_intra_sad_x3_16x16_ssse3 ( pixel   *, pixel   *, int * );
void x264_intra_sa8d_x3_8x8_mmx2   ( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_sse2   ( pixel   *, pixel   *, int * );
void x264_intra_sa8d_x3_8x8_ssse3  ( uint8_t *, uint8_t *, int * );
void x264_intra_sa8d_x3_8x8_avx    ( uint8_t *, uint8_t *, int * );
void x264_intra_sad_x3_8x8_mmx2    ( pixel   *, pixel   *, int * );
void x264_intra_sad_x3_8x8_sse2    ( pixel   *, pixel   *, int * );
void x264_intra_sad_x3_8x8_ssse3   ( pixel   *, pixel   *, int * );
void x264_intra_sad_x3_8x8_avx     ( pixel   *, pixel   *, int * );

void x264_pixel_ssd_nv12_core_mmx2( pixel *pixuv1, int stride1,
                                    pixel *pixuv2, int stride2, int width,
                                    int height, uint64_t *ssd_u, uint64_t *ssd_v );
void x264_pixel_ssd_nv12_core_sse2( pixel *pixuv1, int stride1,
                                    pixel *pixuv2, int stride2, int width,
                                    int height, uint64_t *ssd_u, uint64_t *ssd_v );
void x264_pixel_ssd_nv12_core_avx ( pixel *pixuv1, int stride1,
                                    pixel *pixuv2, int stride2, int width,
                                    int height, uint64_t *ssd_u, uint64_t *ssd_v );
void x264_pixel_ssim_4x4x2_core_mmx2( const uint8_t *pix1, int stride1,
                                      const uint8_t *pix2, int stride2, int sums[2][4] );
void x264_pixel_ssim_4x4x2_core_sse2( const pixel *pix1, int stride1,
                                      const pixel *pix2, int stride2, int sums[2][4] );
void x264_pixel_ssim_4x4x2_core_avx ( const pixel *pix1, int stride1,
                                      const pixel *pix2, int stride2, int sums[2][4] );
float x264_pixel_ssim_end4_sse2( int sum0[5][4], int sum1[5][4], int width );
float x264_pixel_ssim_end4_avx( int sum0[5][4], int sum1[5][4], int width );
int  x264_pixel_var2_8x8_mmx2( pixel *, int, pixel *, int, int * );
int  x264_pixel_var2_8x8_sse2( pixel *, int, pixel *, int, int * );
int  x264_pixel_var2_8x8_ssse3( uint8_t *, int, uint8_t *, int, int * );
int  x264_pixel_vsad_mmx2( pixel *src, int stride, int height );
int  x264_pixel_vsad_sse2( pixel *src, int stride, int height );

#define DECL_ADS( size, suffix ) \
int x264_pixel_ads##size##_##suffix( int enc_dc[size], uint16_t *sums, int delta,\
                                     uint16_t *cost_mvx, int16_t *mvs, int width, int thresh );
DECL_ADS( 4, mmx2 )
DECL_ADS( 2, mmx2 )
DECL_ADS( 1, mmx2 )
DECL_ADS( 4, sse2 )
DECL_ADS( 2, sse2 )
DECL_ADS( 1, sse2 )
DECL_ADS( 4, ssse3 )
DECL_ADS( 2, ssse3 )
DECL_ADS( 1, ssse3 )
DECL_ADS( 4, avx )
DECL_ADS( 2, avx )
DECL_ADS( 1, avx )

#undef DECL_PIXELS
#undef DECL_X1
#undef DECL_X4
#undef DECL_ADS

#endif
