/*
 *  Copyright (c) 2010 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */


#include "vpx_config.h"
#include "loopfilter.h"
#include "onyxc_int.h"
#include "vpx_mem/vpx_mem.h"

typedef unsigned char uc;

prototype_loopfilter(vp8_loop_filter_horizontal_edge_c);
prototype_loopfilter(vp8_loop_filter_vertical_edge_c);
prototype_loopfilter(vp8_mbloop_filter_horizontal_edge_c);
prototype_loopfilter(vp8_mbloop_filter_vertical_edge_c);

prototype_simple_loopfilter(vp8_loop_filter_simple_horizontal_edge_c);
prototype_simple_loopfilter(vp8_loop_filter_simple_vertical_edge_c);

/* Horizontal MB filtering */
void vp8_loop_filter_mbh_c(unsigned char *y_ptr, unsigned char *u_ptr,
                           unsigned char *v_ptr, int y_stride, int uv_stride,
                           loop_filter_info *lfi)
{
    vp8_mbloop_filter_horizontal_edge_c(y_ptr, y_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_mbloop_filter_horizontal_edge_c(u_ptr, uv_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 1);

    if (v_ptr)
        vp8_mbloop_filter_horizontal_edge_c(v_ptr, uv_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 1);
}

/* Vertical MB Filtering */
void vp8_loop_filter_mbv_c(unsigned char *y_ptr, unsigned char *u_ptr,
                           unsigned char *v_ptr, int y_stride, int uv_stride,
                           loop_filter_info *lfi)
{
    vp8_mbloop_filter_vertical_edge_c(y_ptr, y_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_mbloop_filter_vertical_edge_c(u_ptr, uv_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 1);

    if (v_ptr)
        vp8_mbloop_filter_vertical_edge_c(v_ptr, uv_stride, lfi->mblim, lfi->lim, lfi->hev_thr, 1);
}

/* Horizontal B Filtering */
void vp8_loop_filter_bh_c(unsigned char *y_ptr, unsigned char *u_ptr,
                          unsigned char *v_ptr, int y_stride, int uv_stride,
                          loop_filter_info *lfi)
{
    vp8_loop_filter_horizontal_edge_c(y_ptr + 4 * y_stride, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_horizontal_edge_c(y_ptr + 8 * y_stride, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_horizontal_edge_c(y_ptr + 12 * y_stride, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_loop_filter_horizontal_edge_c(u_ptr + 4 * uv_stride, uv_stride, lfi->blim, lfi->lim, lfi->hev_thr, 1);

    if (v_ptr)
        vp8_loop_filter_horizontal_edge_c(v_ptr + 4 * uv_stride, uv_stride, lfi->blim, lfi->lim, lfi->hev_thr, 1);
}

void vp8_loop_filter_bhs_c(unsigned char *y_ptr, int y_stride,
                           const unsigned char *blimit)
{
    vp8_loop_filter_simple_horizontal_edge_c(y_ptr + 4 * y_stride, y_stride, blimit);
    vp8_loop_filter_simple_horizontal_edge_c(y_ptr + 8 * y_stride, y_stride, blimit);
    vp8_loop_filter_simple_horizontal_edge_c(y_ptr + 12 * y_stride, y_stride, blimit);
}

/* Vertical B Filtering */
void vp8_loop_filter_bv_c(unsigned char *y_ptr, unsigned char *u_ptr,
                          unsigned char *v_ptr, int y_stride, int uv_stride,
                          loop_filter_info *lfi)
{
    vp8_loop_filter_vertical_edge_c(y_ptr + 4, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_vertical_edge_c(y_ptr + 8, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);
    vp8_loop_filter_vertical_edge_c(y_ptr + 12, y_stride, lfi->blim, lfi->lim, lfi->hev_thr, 2);

    if (u_ptr)
        vp8_loop_filter_vertical_edge_c(u_ptr + 4, uv_stride, lfi->blim, lfi->lim, lfi->hev_thr, 1);

    if (v_ptr)
        vp8_loop_filter_vertical_edge_c(v_ptr + 4, uv_stride, lfi->blim, lfi->lim, lfi->hev_thr, 1);
}

void vp8_loop_filter_bvs_c(unsigned char *y_ptr, int y_stride,
                           const unsigned char *blimit)
{
    vp8_loop_filter_simple_vertical_edge_c(y_ptr + 4, y_stride, blimit);
    vp8_loop_filter_simple_vertical_edge_c(y_ptr + 8, y_stride, blimit);
    vp8_loop_filter_simple_vertical_edge_c(y_ptr + 12, y_stride, blimit);
}

static void lf_init_lut(loop_filter_info_n *lfi)
{
    int filt_lvl;

    for (filt_lvl = 0; filt_lvl <= MAX_LOOP_FILTER; filt_lvl++)
    {
        if (filt_lvl >= 40)
        {
            lfi->hev_thr_lut[KEY_FRAME][filt_lvl] = 2;
            lfi->hev_thr_lut[INTER_FRAME][filt_lvl] = 3;
        }
        else if (filt_lvl >= 20)
        {
            lfi->hev_thr_lut[KEY_FRAME][filt_lvl] = 1;
            lfi->hev_thr_lut[INTER_FRAME][filt_lvl] = 2;
        }
        else if (filt_lvl >= 15)
        {
            lfi->hev_thr_lut[KEY_FRAME][filt_lvl] = 1;
            lfi->hev_thr_lut[INTER_FRAME][filt_lvl] = 1;
        }
        else
        {
            lfi->hev_thr_lut[KEY_FRAME][filt_lvl] = 0;
            lfi->hev_thr_lut[INTER_FRAME][filt_lvl] = 0;
        }
    }

    lfi->mode_lf_lut[DC_PRED] = 1;
    lfi->mode_lf_lut[V_PRED] = 1;
    lfi->mode_lf_lut[H_PRED] = 1;
    lfi->mode_lf_lut[TM_PRED] = 1;
    lfi->mode_lf_lut[B_PRED]  = 0;

    lfi->mode_lf_lut[ZEROMV]  = 1;
    lfi->mode_lf_lut[NEARESTMV] = 2;
    lfi->mode_lf_lut[NEARMV] = 2;
    lfi->mode_lf_lut[NEWMV] = 2;
    lfi->mode_lf_lut[SPLITMV] = 3;

}

void vp8_loop_filter_update_sharpness(loop_filter_info_n *lfi,
                                      int sharpness_lvl)
{
    int i;

    /* For each possible value for the loop filter fill out limits */
    for (i = 0; i <= MAX_LOOP_FILTER; i++)
    {
        int filt_lvl = i;
        int block_inside_limit = 0;

        /* Set loop filter paramaeters that control sharpness. */
        block_inside_limit = filt_lvl >> (sharpness_lvl > 0);
        block_inside_limit = block_inside_limit >> (sharpness_lvl > 4);

        if (sharpness_lvl > 0)
        {
            if (block_inside_limit > (9 - sharpness_lvl))
                block_inside_limit = (9 - sharpness_lvl);
        }

        if (block_inside_limit < 1)
            block_inside_limit = 1;

        vpx_memset(lfi->lim[i], block_inside_limit, SIMD_WIDTH);
        vpx_memset(lfi->blim[i], (2 * filt_lvl + block_inside_limit),
                SIMD_WIDTH);
        vpx_memset(lfi->mblim[i], (2 * (filt_lvl + 2) + block_inside_limit),
                SIMD_WIDTH);
    }
}

void vp8_loop_filter_init(VP8_COMMON *cm)
{
    loop_filter_info_n *lfi = &cm->lf_info;
    int i;

    /* init limits for given sharpness*/
    vp8_loop_filter_update_sharpness(lfi, cm->sharpness_level);
    cm->last_sharpness_level = cm->sharpness_level;

    /* init LUT for lvl  and hev thr picking */
    lf_init_lut(lfi);

    /* init hev threshold const vectors */
    for(i = 0; i < 4 ; i++)
    {
        vpx_memset(lfi->hev_thr[i], i, SIMD_WIDTH);
    }
}

void vp8_loop_filter_frame_init(VP8_COMMON *cm,
                                MACROBLOCKD *mbd,
                                int default_filt_lvl)
{
    int seg,  /* segment number */
        ref,  /* index in ref_lf_deltas */
        mode; /* index in mode_lf_deltas */

    loop_filter_info_n *lfi = &cm->lf_info;

    /* update limits if sharpness has changed */
    if(cm->last_sharpness_level != cm->sharpness_level)
    {
        vp8_loop_filter_update_sharpness(lfi, cm->sharpness_level);
        cm->last_sharpness_level = cm->sharpness_level;
    }

    for(seg = 0; seg < MAX_MB_SEGMENTS; seg++)
    {
        int lvl_seg = default_filt_lvl;
        int lvl_ref, lvl_mode;

        /* Note the baseline filter values for each segment */
        if (mbd->segmentation_enabled)
        {
            /* Abs value */
            if (mbd->mb_segement_abs_delta == SEGMENT_ABSDATA)
            {
                lvl_seg = mbd->segment_feature_data[MB_LVL_ALT_LF][seg];
            }
            else  /* Delta Value */
            {
                lvl_seg += mbd->segment_feature_data[MB_LVL_ALT_LF][seg];
                lvl_seg = (lvl_seg > 0) ? ((lvl_seg > 63) ? 63: lvl_seg) : 0;
            }
        }

        if (!mbd->mode_ref_lf_delta_enabled)
        {
            /* we could get rid of this if we assume that deltas are set to
             * zero when not in use; encoder always uses deltas
             */
            vpx_memset(lfi->lvl[seg][0], lvl_seg, 4 * 4 );
            continue;
        }

        lvl_ref = lvl_seg;

        /* INTRA_FRAME */
        ref = INTRA_FRAME;

        /* Apply delta for reference frame */
        lvl_ref += mbd->ref_lf_deltas[ref];

        /* Apply delta for Intra modes */
        mode = 0; /* B_PRED */
        /* Only the split mode BPRED has a further special case */
        lvl_mode = lvl_ref +  mbd->mode_lf_deltas[mode];
        lvl_mode = (lvl_mode > 0) ? (lvl_mode > 63 ? 63 : lvl_mode) : 0; /* clamp */

        lfi->lvl[seg][ref][mode] = lvl_mode;

        mode = 1; /* all the rest of Intra modes */
        lvl_mode = (lvl_ref > 0) ? (lvl_ref > 63 ? 63 : lvl_ref)  : 0; /* clamp */
        lfi->lvl[seg][ref][mode] = lvl_mode;

        /* LAST, GOLDEN, ALT */
        for(ref = 1; ref < MAX_REF_FRAMES; ref++)
        {
            int lvl_ref = lvl_seg;

            /* Apply delta for reference frame */
            lvl_ref += mbd->ref_lf_deltas[ref];

            /* Apply delta for Inter modes */
            for (mode = 1; mode < 4; mode++)
            {
                lvl_mode = lvl_ref + mbd->mode_lf_deltas[mode];
                lvl_mode = (lvl_mode > 0) ? (lvl_mode > 63 ? 63 : lvl_mode) : 0; /* clamp */

                lfi->lvl[seg][ref][mode] = lvl_mode;
            }
        }
    }
}

void vp8_loop_filter_frame
(
    VP8_COMMON *cm,
    MACROBLOCKD *mbd
)
{
    YV12_BUFFER_CONFIG *post = cm->frame_to_show;
    loop_filter_info_n *lfi_n = &cm->lf_info;
    loop_filter_info lfi;

    FRAME_TYPE frame_type = cm->frame_type;

    int mb_row;
    int mb_col;

    int filter_level;

    unsigned char *y_ptr, *u_ptr, *v_ptr;

    /* Point at base of Mb MODE_INFO list */
    const MODE_INFO *mode_info_context = cm->mi;

    /* Initialize the loop filter for this frame. */
    vp8_loop_filter_frame_init(cm, mbd, cm->filter_level);

    /* Set up the buffer pointers */
    y_ptr = post->y_buffer;
    u_ptr = post->u_buffer;
    v_ptr = post->v_buffer;

    /* vp8_filter each macro block */
    for (mb_row = 0; mb_row < cm->mb_rows; mb_row++)
    {
        for (mb_col = 0; mb_col < cm->mb_cols; mb_col++)
        {
            int skip_lf = (mode_info_context->mbmi.mode != B_PRED &&
                            mode_info_context->mbmi.mode != SPLITMV &&
                            mode_info_context->mbmi.mb_skip_coeff);

            const int mode_index = lfi_n->mode_lf_lut[mode_info_context->mbmi.mode];
            const int seg = mode_info_context->mbmi.segment_id;
            const int ref_frame = mode_info_context->mbmi.ref_frame;

            filter_level = lfi_n->lvl[seg][ref_frame][mode_index];

            if (filter_level)
            {
                if (cm->filter_type == NORMAL_LOOPFILTER)
                {
                    const int hev_index = lfi_n->hev_thr_lut[frame_type][filter_level];
                    lfi.mblim = lfi_n->mblim[filter_level];
                    lfi.blim = lfi_n->blim[filter_level];
                    lfi.lim = lfi_n->lim[filter_level];
                    lfi.hev_thr = lfi_n->hev_thr[hev_index];

                    if (mb_col > 0)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_mb_v)
                        (y_ptr, u_ptr, v_ptr, post->y_stride, post->uv_stride, &lfi);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_b_v)
                        (y_ptr, u_ptr, v_ptr, post->y_stride, post->uv_stride, &lfi);

                    /* don't apply across umv border */
                    if (mb_row > 0)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_mb_h)
                        (y_ptr, u_ptr, v_ptr, post->y_stride, post->uv_stride, &lfi);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_b_h)
                        (y_ptr, u_ptr, v_ptr, post->y_stride, post->uv_stride, &lfi);
                }
                else
                {
                    if (mb_col > 0)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_mb_v)
                        (y_ptr, post->y_stride, lfi_n->mblim[filter_level]);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_b_v)
                        (y_ptr, post->y_stride, lfi_n->blim[filter_level]);

                    /* don't apply across umv border */
                    if (mb_row > 0)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_mb_h)
                        (y_ptr, post->y_stride, lfi_n->mblim[filter_level]);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_b_h)
                        (y_ptr, post->y_stride, lfi_n->blim[filter_level]);
                }
            }

            y_ptr += 16;
            u_ptr += 8;
            v_ptr += 8;

            mode_info_context++;     /* step to next MB */
        }

        y_ptr += post->y_stride  * 16 - post->y_width;
        u_ptr += post->uv_stride *  8 - post->uv_width;
        v_ptr += post->uv_stride *  8 - post->uv_width;

        mode_info_context++;         /* Skip border mb */
    }
}

void vp8_loop_filter_frame_yonly
(
    VP8_COMMON *cm,
    MACROBLOCKD *mbd,
    int default_filt_lvl
)
{
    YV12_BUFFER_CONFIG *post = cm->frame_to_show;

    unsigned char *y_ptr;
    int mb_row;
    int mb_col;

    loop_filter_info_n *lfi_n = &cm->lf_info;
    loop_filter_info lfi;

    int filter_level;
    FRAME_TYPE frame_type = cm->frame_type;

    /* Point at base of Mb MODE_INFO list */
    const MODE_INFO *mode_info_context = cm->mi;

#if 0
    if(default_filt_lvl == 0) /* no filter applied */
        return;
#endif

    /* Initialize the loop filter for this frame. */
    vp8_loop_filter_frame_init( cm, mbd, default_filt_lvl);

    /* Set up the buffer pointers */
    y_ptr = post->y_buffer;

    /* vp8_filter each macro block */
    for (mb_row = 0; mb_row < cm->mb_rows; mb_row++)
    {
        for (mb_col = 0; mb_col < cm->mb_cols; mb_col++)
        {
            int skip_lf = (mode_info_context->mbmi.mode != B_PRED &&
                            mode_info_context->mbmi.mode != SPLITMV &&
                            mode_info_context->mbmi.mb_skip_coeff);

            const int mode_index = lfi_n->mode_lf_lut[mode_info_context->mbmi.mode];
            const int seg = mode_info_context->mbmi.segment_id;
            const int ref_frame = mode_info_context->mbmi.ref_frame;

            filter_level = lfi_n->lvl[seg][ref_frame][mode_index];

            if (filter_level)
            {
                if (cm->filter_type == NORMAL_LOOPFILTER)
                {
                    const int hev_index = lfi_n->hev_thr_lut[frame_type][filter_level];
                    lfi.mblim = lfi_n->mblim[filter_level];
                    lfi.blim = lfi_n->blim[filter_level];
                    lfi.lim = lfi_n->lim[filter_level];
                    lfi.hev_thr = lfi_n->hev_thr[hev_index];

                    if (mb_col > 0)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_mb_v)
                        (y_ptr, 0, 0, post->y_stride, 0, &lfi);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_b_v)
                        (y_ptr, 0, 0, post->y_stride, 0, &lfi);

                    /* don't apply across umv border */
                    if (mb_row > 0)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_mb_h)
                        (y_ptr, 0, 0, post->y_stride, 0, &lfi);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_b_h)
                        (y_ptr, 0, 0, post->y_stride, 0, &lfi);
                }
                else
                {
                    if (mb_col > 0)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_mb_v)
                        (y_ptr, post->y_stride, lfi_n->mblim[filter_level]);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_b_v)
                        (y_ptr, post->y_stride, lfi_n->blim[filter_level]);

                    /* don't apply across umv border */
                    if (mb_row > 0)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_mb_h)
                        (y_ptr, post->y_stride, lfi_n->mblim[filter_level]);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_b_h)
                        (y_ptr, post->y_stride, lfi_n->blim[filter_level]);
                }
            }

            y_ptr += 16;
            mode_info_context ++;        /* step to next MB */

        }

        y_ptr += post->y_stride  * 16 - post->y_width;
        mode_info_context ++;            /* Skip border mb */
    }

}

void vp8_loop_filter_partial_frame
(
    VP8_COMMON *cm,
    MACROBLOCKD *mbd,
    int default_filt_lvl
)
{
    YV12_BUFFER_CONFIG *post = cm->frame_to_show;

    unsigned char *y_ptr;
    int mb_row;
    int mb_col;
    int mb_cols = post->y_width >> 4;
    int mb_rows = post->y_height >> 4;

    int linestocopy, i;

    loop_filter_info_n *lfi_n = &cm->lf_info;
    loop_filter_info lfi;

    int filter_level;
    int alt_flt_enabled = mbd->segmentation_enabled;
    FRAME_TYPE frame_type = cm->frame_type;

    const MODE_INFO *mode_info_context;

    int lvl_seg[MAX_MB_SEGMENTS];

    /* number of MB rows to use in partial filtering */
    linestocopy = mb_rows / PARTIAL_FRAME_FRACTION;
    linestocopy = linestocopy ? linestocopy << 4 : 16;     /* 16 lines per MB */

    /* Note the baseline filter values for each segment */
    /* See vp8_loop_filter_frame_init. Rather than call that for each change
     * to default_filt_lvl, copy the relevant calculation here.
     */
    if (alt_flt_enabled)
    {
        for (i = 0; i < MAX_MB_SEGMENTS; i++)
        {    /* Abs value */
            if (mbd->mb_segement_abs_delta == SEGMENT_ABSDATA)
            {
                lvl_seg[i] = mbd->segment_feature_data[MB_LVL_ALT_LF][i];
            }
            /* Delta Value */
            else
            {
                lvl_seg[i] = default_filt_lvl
                        + mbd->segment_feature_data[MB_LVL_ALT_LF][i];
                lvl_seg[i] = (lvl_seg[i] > 0) ?
                        ((lvl_seg[i] > 63) ? 63: lvl_seg[i]) : 0;
            }
        }
    }

    /* Set up the buffer pointers; partial image starts at ~middle of frame */
    y_ptr = post->y_buffer + ((post->y_height >> 5) * 16) * post->y_stride;
    mode_info_context = cm->mi + (post->y_height >> 5) * (mb_cols + 1);

    /* vp8_filter each macro block */
    for (mb_row = 0; mb_row<(linestocopy >> 4); mb_row++)
    {
        for (mb_col = 0; mb_col < mb_cols; mb_col++)
        {
            int skip_lf = (mode_info_context->mbmi.mode != B_PRED &&
                           mode_info_context->mbmi.mode != SPLITMV &&
                           mode_info_context->mbmi.mb_skip_coeff);

            if (alt_flt_enabled)
                filter_level = lvl_seg[mode_info_context->mbmi.segment_id];
            else
                filter_level = default_filt_lvl;

            if (filter_level)
            {
                if (cm->filter_type == NORMAL_LOOPFILTER)
                {
                    const int hev_index = lfi_n->hev_thr_lut[frame_type][filter_level];
                    lfi.mblim = lfi_n->mblim[filter_level];
                    lfi.blim = lfi_n->blim[filter_level];
                    lfi.lim = lfi_n->lim[filter_level];
                    lfi.hev_thr = lfi_n->hev_thr[hev_index];

                    if (mb_col > 0)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_mb_v)
                        (y_ptr, 0, 0, post->y_stride, 0, &lfi);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_b_v)
                        (y_ptr, 0, 0, post->y_stride, 0, &lfi);

                    LF_INVOKE(&cm->rtcd.loopfilter, normal_mb_h)
                        (y_ptr, 0, 0, post->y_stride, 0, &lfi);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, normal_b_h)
                        (y_ptr, 0, 0, post->y_stride, 0, &lfi);
                }
                else
                {
                    if (mb_col > 0)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_mb_v)
                        (y_ptr, post->y_stride, lfi_n->mblim[filter_level]);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_b_v)
                        (y_ptr, post->y_stride, lfi_n->blim[filter_level]);

                    LF_INVOKE(&cm->rtcd.loopfilter, simple_mb_h)
                        (y_ptr, post->y_stride, lfi_n->mblim[filter_level]);

                    if (!skip_lf)
                        LF_INVOKE(&cm->rtcd.loopfilter, simple_b_h)
                        (y_ptr, post->y_stride, lfi_n->blim[filter_level]);
                }
            }

            y_ptr += 16;
            mode_info_context += 1;      /* step to next MB */
        }

        y_ptr += post->y_stride  * 16 - post->y_width;
        mode_info_context += 1;          /* Skip border mb */
    }
}
