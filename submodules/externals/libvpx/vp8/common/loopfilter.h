/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#ifndef loopfilter_h
#define loopfilter_h

#include "vpx_ports/mem.h"
#include "vpx_config.h"

#define MAX_LOOP_FILTER             63
/* fraction of total macroblock rows to be used in fast filter level picking */
/* has to be > 2 */
#define PARTIAL_FRAME_FRACTION      8

typedef enum
{
    NORMAL_LOOPFILTER = 0,
    SIMPLE_LOOPFILTER = 1
} LOOPFILTERTYPE;

#if ARCH_ARM
#define SIMD_WIDTH 1
#else
#define SIMD_WIDTH 16
#endif

/* Need to align this structure so when it is declared and
 * passed it can be loaded into vector registers.
 */
typedef struct
{
    DECLARE_ALIGNED(SIMD_WIDTH, unsigned char, mblim[MAX_LOOP_FILTER + 1][SIMD_WIDTH]);
    DECLARE_ALIGNED(SIMD_WIDTH, unsigned char, blim[MAX_LOOP_FILTER + 1][SIMD_WIDTH]);
    DECLARE_ALIGNED(SIMD_WIDTH, unsigned char, lim[MAX_LOOP_FILTER + 1][SIMD_WIDTH]);
    DECLARE_ALIGNED(SIMD_WIDTH, unsigned char, hev_thr[4][SIMD_WIDTH]);
    unsigned char lvl[4][4][4];
    unsigned char hev_thr_lut[2][MAX_LOOP_FILTER + 1];
    unsigned char mode_lf_lut[10];
} loop_filter_info_n;

typedef struct
{
    const unsigned char * mblim;
    const unsigned char * blim;
    const unsigned char * lim;
    const unsigned char * hev_thr;
} loop_filter_info;


#define prototype_loopfilter(sym) \
    void sym(unsigned char *src, int pitch, const unsigned char *blimit,\
             const unsigned char *limit, const unsigned char *thresh, int count)

#define prototype_loopfilter_block(sym) \
    void sym(unsigned char *y, unsigned char *u, unsigned char *v, \
             int ystride, int uv_stride, loop_filter_info *lfi)

#define prototype_simple_loopfilter(sym) \
    void sym(unsigned char *y, int ystride, const unsigned char *blimit)

#if ARCH_X86 || ARCH_X86_64
#include "x86/loopfilter_x86.h"
#endif

#if ARCH_ARM
#include "arm/loopfilter_arm.h"
#endif

#ifndef vp8_lf_normal_mb_v
#define vp8_lf_normal_mb_v vp8_loop_filter_mbv_c
#endif
extern prototype_loopfilter_block(vp8_lf_normal_mb_v);

#ifndef vp8_lf_normal_b_v
#define vp8_lf_normal_b_v vp8_loop_filter_bv_c
#endif
extern prototype_loopfilter_block(vp8_lf_normal_b_v);

#ifndef vp8_lf_normal_mb_h
#define vp8_lf_normal_mb_h vp8_loop_filter_mbh_c
#endif
extern prototype_loopfilter_block(vp8_lf_normal_mb_h);

#ifndef vp8_lf_normal_b_h
#define vp8_lf_normal_b_h vp8_loop_filter_bh_c
#endif
extern prototype_loopfilter_block(vp8_lf_normal_b_h);

#ifndef vp8_lf_simple_mb_v
#define vp8_lf_simple_mb_v vp8_loop_filter_simple_vertical_edge_c
#endif
extern prototype_simple_loopfilter(vp8_lf_simple_mb_v);

#ifndef vp8_lf_simple_b_v
#define vp8_lf_simple_b_v vp8_loop_filter_bvs_c
#endif
extern prototype_simple_loopfilter(vp8_lf_simple_b_v);

#ifndef vp8_lf_simple_mb_h
#define vp8_lf_simple_mb_h vp8_loop_filter_simple_horizontal_edge_c
#endif
extern prototype_simple_loopfilter(vp8_lf_simple_mb_h);

#ifndef vp8_lf_simple_b_h
#define vp8_lf_simple_b_h vp8_loop_filter_bhs_c
#endif
extern prototype_simple_loopfilter(vp8_lf_simple_b_h);

typedef prototype_loopfilter_block((*vp8_lf_block_fn_t));
typedef prototype_simple_loopfilter((*vp8_slf_block_fn_t));

typedef struct
{
    vp8_lf_block_fn_t  normal_mb_v;
    vp8_lf_block_fn_t  normal_b_v;
    vp8_lf_block_fn_t  normal_mb_h;
    vp8_lf_block_fn_t  normal_b_h;
    vp8_slf_block_fn_t  simple_mb_v;
    vp8_slf_block_fn_t  simple_b_v;
    vp8_slf_block_fn_t  simple_mb_h;
    vp8_slf_block_fn_t  simple_b_h;
} vp8_loopfilter_rtcd_vtable_t;

#if CONFIG_RUNTIME_CPU_DETECT
#define LF_INVOKE(ctx,fn) (ctx)->fn
#else
#define LF_INVOKE(ctx,fn) vp8_lf_##fn
#endif

typedef void loop_filter_uvfunction
(
    unsigned char *u,   /* source pointer */
    int p,              /* pitch */
    const unsigned char *blimit,
    const unsigned char *limit,
    const unsigned char *thresh,
    unsigned char *v
);

/* assorted loopfilter functions which get used elsewhere */
struct VP8Common;
struct MacroBlockD;

void vp8_loop_filter_init(struct VP8Common *cm);

void vp8_loop_filter_frame_init(struct VP8Common *cm,
                                struct MacroBlockD *mbd,
                                int default_filt_lvl);

void vp8_loop_filter_frame(struct VP8Common *cm, struct MacroBlockD *mbd);

void vp8_loop_filter_partial_frame(struct VP8Common *cm,
                                   struct MacroBlockD *mbd,
                                   int default_filt_lvl);

void vp8_loop_filter_frame_yonly(struct VP8Common *cm,
                                 struct MacroBlockD *mbd,
                                 int default_filt_lvl);

void vp8_loop_filter_update_sharpness(loop_filter_info_n *lfi,
                                      int sharpness_lvl);

#endif
