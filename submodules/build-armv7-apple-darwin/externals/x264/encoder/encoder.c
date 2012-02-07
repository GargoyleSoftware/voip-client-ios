/*****************************************************************************
 * encoder.c: top-level encoder functions
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

#include "set.h"
#include "analyse.h"
#include "ratecontrol.h"
#include "macroblock.h"
#include "me.h"

#if HAVE_VISUALIZE
#include "common/visualize.h"
#endif

//#define DEBUG_MB_TYPE

#define bs_write_ue bs_write_ue_big

static int x264_encoder_frame_end( x264_t *h, x264_t *thread_current,
                                   x264_nal_t **pp_nal, int *pi_nal,
                                   x264_picture_t *pic_out );

/****************************************************************************
 *
 ******************************* x264 libs **********************************
 *
 ****************************************************************************/
static double x264_psnr( double sqe, double size )
{
    double mse = sqe / (PIXEL_MAX*PIXEL_MAX * size);
    if( mse <= 0.0000000001 ) /* Max 100dB */
        return 100;

    return -10.0 * log10( mse );
}

static double x264_ssim( double ssim )
{
    return -10.0 * log10( 1 - ssim );
}

static void x264_frame_dump( x264_t *h )
{
    FILE *f = fopen( h->param.psz_dump_yuv, "r+b" );
    if( !f )
        return;

    /* Write the frame in display order */
    int frame_size = h->param.i_height * h->param.i_width * (3<<CHROMA444)/2 * sizeof(pixel);
    fseek( f, (uint64_t)h->fdec->i_frame * frame_size, SEEK_SET );
    for( int p = 0; p < (CHROMA444 ? 3 : 1); p++ )
        for( int y = 0; y < h->param.i_height; y++ )
            fwrite( &h->fdec->plane[p][y*h->fdec->i_stride[p]], sizeof(pixel), h->param.i_width, f );
    if( !CHROMA444 )
    {
        int cw = h->param.i_width>>1;
        int ch = h->param.i_height>>1;
        pixel *planeu = x264_malloc( (cw*ch*2+32)*sizeof(pixel) );
        pixel *planev = planeu + cw*ch + 16;
        h->mc.plane_copy_deinterleave( planeu, cw, planev, cw, h->fdec->plane[1], h->fdec->i_stride[1], cw, ch );
        fwrite( planeu, 1, cw*ch*sizeof(pixel), f );
        fwrite( planev, 1, cw*ch*sizeof(pixel), f );
        x264_free( planeu );
    }
    fclose( f );
}


/* Fill "default" values */
static void x264_slice_header_init( x264_t *h, x264_slice_header_t *sh,
                                    x264_sps_t *sps, x264_pps_t *pps,
                                    int i_idr_pic_id, int i_frame, int i_qp )
{
    x264_param_t *param = &h->param;

    /* First we fill all fields */
    sh->sps = sps;
    sh->pps = pps;

    sh->i_first_mb  = 0;
    sh->i_last_mb   = h->mb.i_mb_count - 1;
    sh->i_pps_id    = pps->i_id;

    sh->i_frame_num = i_frame;

    sh->b_mbaff = PARAM_INTERLACED;
    sh->b_field_pic = 0;    /* no field support for now */
    sh->b_bottom_field = 0; /* not yet used */

    sh->i_idr_pic_id = i_idr_pic_id;

    /* poc stuff, fixed later */
    sh->i_poc = 0;
    sh->i_delta_poc_bottom = 0;
    sh->i_delta_poc[0] = 0;
    sh->i_delta_poc[1] = 0;

    sh->i_redundant_pic_cnt = 0;

    h->mb.b_direct_auto_write = h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_AUTO
                                && h->param.i_bframe
                                && ( h->param.rc.b_stat_write || !h->param.rc.b_stat_read );

    if( !h->mb.b_direct_auto_read && sh->i_type == SLICE_TYPE_B )
    {
        if( h->fref[1][0]->i_poc_l0ref0 == h->fref[0][0]->i_poc )
        {
            if( h->mb.b_direct_auto_write )
                sh->b_direct_spatial_mv_pred = ( h->stat.i_direct_score[1] > h->stat.i_direct_score[0] );
            else
                sh->b_direct_spatial_mv_pred = ( param->analyse.i_direct_mv_pred == X264_DIRECT_PRED_SPATIAL );
        }
        else
        {
            h->mb.b_direct_auto_write = 0;
            sh->b_direct_spatial_mv_pred = 1;
        }
    }
    /* else b_direct_spatial_mv_pred was read from the 2pass statsfile */

    sh->b_num_ref_idx_override = 0;
    sh->i_num_ref_idx_l0_active = 1;
    sh->i_num_ref_idx_l1_active = 1;

    sh->b_ref_pic_list_reordering[0] = h->b_ref_reorder[0];
    sh->b_ref_pic_list_reordering[1] = h->b_ref_reorder[1];

    /* If the ref list isn't in the default order, construct reordering header */
    for( int list = 0; list < 2; list++ )
    {
        if( sh->b_ref_pic_list_reordering[list] )
        {
            int pred_frame_num = i_frame;
            for( int i = 0; i < h->i_ref[list]; i++ )
            {
                int diff = h->fref[list][i]->i_frame_num - pred_frame_num;
                sh->ref_pic_list_order[list][i].idc = ( diff > 0 );
                sh->ref_pic_list_order[list][i].arg = (abs(diff) - 1) & ((1 << sps->i_log2_max_frame_num) - 1);
                pred_frame_num = h->fref[list][i]->i_frame_num;
            }
        }
    }

    sh->i_cabac_init_idc = param->i_cabac_init_idc;

    sh->i_qp = SPEC_QP(i_qp);
    sh->i_qp_delta = sh->i_qp - pps->i_pic_init_qp;
    sh->b_sp_for_swidth = 0;
    sh->i_qs_delta = 0;

    int deblock_thresh = i_qp + 2 * X264_MIN(param->i_deblocking_filter_alphac0, param->i_deblocking_filter_beta);
    /* If effective qp <= 15, deblocking would have no effect anyway */
    if( param->b_deblocking_filter && (h->mb.b_variable_qp || 15 < deblock_thresh ) )
        sh->i_disable_deblocking_filter_idc = param->b_sliced_threads ? 2 : 0;
    else
        sh->i_disable_deblocking_filter_idc = 1;
    sh->i_alpha_c0_offset = param->i_deblocking_filter_alphac0 << 1;
    sh->i_beta_offset = param->i_deblocking_filter_beta << 1;
}

static void x264_slice_header_write( bs_t *s, x264_slice_header_t *sh, int i_nal_ref_idc )
{
    if( sh->b_mbaff )
    {
        int first_x = sh->i_first_mb % sh->sps->i_mb_width;
        int first_y = sh->i_first_mb / sh->sps->i_mb_width;
        assert( (first_y&1) == 0 );
        bs_write_ue( s, (2*first_x + sh->sps->i_mb_width*(first_y&~1) + (first_y&1)) >> 1 );
    }
    else
        bs_write_ue( s, sh->i_first_mb );

    bs_write_ue( s, sh->i_type + 5 );   /* same type things */
    bs_write_ue( s, sh->i_pps_id );
    bs_write( s, sh->sps->i_log2_max_frame_num, sh->i_frame_num & ((1<<sh->sps->i_log2_max_frame_num)-1) );

    if( !sh->sps->b_frame_mbs_only )
    {
        bs_write1( s, sh->b_field_pic );
        if( sh->b_field_pic )
            bs_write1( s, sh->b_bottom_field );
    }

    if( sh->i_idr_pic_id >= 0 ) /* NAL IDR */
        bs_write_ue( s, sh->i_idr_pic_id );

    if( sh->sps->i_poc_type == 0 )
    {
        bs_write( s, sh->sps->i_log2_max_poc_lsb, sh->i_poc & ((1<<sh->sps->i_log2_max_poc_lsb)-1) );
        if( sh->pps->b_pic_order && !sh->b_field_pic )
            bs_write_se( s, sh->i_delta_poc_bottom );
    }

    if( sh->pps->b_redundant_pic_cnt )
        bs_write_ue( s, sh->i_redundant_pic_cnt );

    if( sh->i_type == SLICE_TYPE_B )
        bs_write1( s, sh->b_direct_spatial_mv_pred );

    if( sh->i_type == SLICE_TYPE_P || sh->i_type == SLICE_TYPE_B )
    {
        bs_write1( s, sh->b_num_ref_idx_override );
        if( sh->b_num_ref_idx_override )
        {
            bs_write_ue( s, sh->i_num_ref_idx_l0_active - 1 );
            if( sh->i_type == SLICE_TYPE_B )
                bs_write_ue( s, sh->i_num_ref_idx_l1_active - 1 );
        }
    }

    /* ref pic list reordering */
    if( sh->i_type != SLICE_TYPE_I )
    {
        bs_write1( s, sh->b_ref_pic_list_reordering[0] );
        if( sh->b_ref_pic_list_reordering[0] )
        {
            for( int i = 0; i < sh->i_num_ref_idx_l0_active; i++ )
            {
                bs_write_ue( s, sh->ref_pic_list_order[0][i].idc );
                bs_write_ue( s, sh->ref_pic_list_order[0][i].arg );
            }
            bs_write_ue( s, 3 );
        }
    }
    if( sh->i_type == SLICE_TYPE_B )
    {
        bs_write1( s, sh->b_ref_pic_list_reordering[1] );
        if( sh->b_ref_pic_list_reordering[1] )
        {
            for( int i = 0; i < sh->i_num_ref_idx_l1_active; i++ )
            {
                bs_write_ue( s, sh->ref_pic_list_order[1][i].idc );
                bs_write_ue( s, sh->ref_pic_list_order[1][i].arg );
            }
            bs_write_ue( s, 3 );
        }
    }

    if( sh->pps->b_weighted_pred && sh->i_type == SLICE_TYPE_P )
    {
        /* pred_weight_table() */
        bs_write_ue( s, sh->weight[0][0].i_denom );
        bs_write_ue( s, sh->weight[0][1].i_denom );
        for( int i = 0; i < sh->i_num_ref_idx_l0_active; i++ )
        {
            int luma_weight_l0_flag = !!sh->weight[i][0].weightfn;
            int chroma_weight_l0_flag = !!sh->weight[i][1].weightfn || !!sh->weight[i][2].weightfn;
            bs_write1( s, luma_weight_l0_flag );
            if( luma_weight_l0_flag )
            {
                bs_write_se( s, sh->weight[i][0].i_scale );
                bs_write_se( s, sh->weight[i][0].i_offset );
            }
            bs_write1( s, chroma_weight_l0_flag );
            if( chroma_weight_l0_flag )
            {
                for( int j = 1; j < 3; j++ )
                {
                    bs_write_se( s, sh->weight[i][j].i_scale );
                    bs_write_se( s, sh->weight[i][j].i_offset );
                }
            }
        }
    }
    else if( sh->pps->b_weighted_bipred == 1 && sh->i_type == SLICE_TYPE_B )
    {
      /* TODO */
    }

    if( i_nal_ref_idc != 0 )
    {
        if( sh->i_idr_pic_id >= 0 )
        {
            bs_write1( s, 0 );  /* no output of prior pics flag */
            bs_write1( s, 0 );  /* long term reference flag */
        }
        else
        {
            bs_write1( s, sh->i_mmco_command_count > 0 ); /* adaptive_ref_pic_marking_mode_flag */
            if( sh->i_mmco_command_count > 0 )
            {
                for( int i = 0; i < sh->i_mmco_command_count; i++ )
                {
                    bs_write_ue( s, 1 ); /* mark short term ref as unused */
                    bs_write_ue( s, sh->mmco[i].i_difference_of_pic_nums - 1 );
                }
                bs_write_ue( s, 0 ); /* end command list */
            }
        }
    }

    if( sh->pps->b_cabac && sh->i_type != SLICE_TYPE_I )
        bs_write_ue( s, sh->i_cabac_init_idc );

    bs_write_se( s, sh->i_qp_delta );      /* slice qp delta */

    if( sh->pps->b_deblocking_filter_control )
    {
        bs_write_ue( s, sh->i_disable_deblocking_filter_idc );
        if( sh->i_disable_deblocking_filter_idc != 1 )
        {
            bs_write_se( s, sh->i_alpha_c0_offset >> 1 );
            bs_write_se( s, sh->i_beta_offset >> 1 );
        }
    }
}

/* If we are within a reasonable distance of the end of the memory allocated for the bitstream, */
/* reallocate, adding an arbitrary amount of space (100 kilobytes). */
static int x264_bitstream_check_buffer( x264_t *h )
{
    uint8_t *bs_bak = h->out.p_bitstream;
    int max_mb_size = 2500 << SLICE_MBAFF;
    if( (h->param.b_cabac && (h->cabac.p_end - h->cabac.p < max_mb_size)) ||
        (h->out.bs.p_end - h->out.bs.p < max_mb_size) )
    {
        h->out.i_bitstream += 100000;
        CHECKED_MALLOC( h->out.p_bitstream, h->out.i_bitstream );
        h->mc.memcpy_aligned( h->out.p_bitstream, bs_bak, (h->out.i_bitstream - 100000) & ~15 );
        intptr_t delta = h->out.p_bitstream - bs_bak;

        h->out.bs.p_start += delta;
        h->out.bs.p += delta;
        h->out.bs.p_end = h->out.p_bitstream + h->out.i_bitstream;

        h->cabac.p_start += delta;
        h->cabac.p += delta;
        h->cabac.p_end = h->out.p_bitstream + h->out.i_bitstream;

        for( int i = 0; i <= h->out.i_nal; i++ )
            h->out.nal[i].p_payload += delta;
        x264_free( bs_bak );
    }
    return 0;
fail:
    x264_free( bs_bak );
    return -1;
}

#if HAVE_THREAD
static void x264_encoder_thread_init( x264_t *h )
{
    if( h->param.i_sync_lookahead )
        x264_lower_thread_priority( 10 );

#if HAVE_MMX
    /* Misalign mask has to be set separately for each thread. */
    if( h->param.cpu&X264_CPU_SSE_MISALIGN )
        x264_cpu_mask_misalign_sse();
#endif
}
#endif

/****************************************************************************
 *
 ****************************************************************************
 ****************************** External API*********************************
 ****************************************************************************
 *
 ****************************************************************************/

static int x264_validate_parameters( x264_t *h, int b_open )
{
#if HAVE_MMX
#ifdef __SSE__
    if( b_open && !(x264_cpu_detect() & X264_CPU_SSE) )
    {
        x264_log( h, X264_LOG_ERROR, "your cpu does not support SSE1, but x264 was compiled with asm support\n");
#else
    if( b_open && !(x264_cpu_detect() & X264_CPU_MMX2) )
    {
        x264_log( h, X264_LOG_ERROR, "your cpu does not support MMXEXT, but x264 was compiled with asm support\n");
#endif
        x264_log( h, X264_LOG_ERROR, "to run x264, recompile without asm support (configure --disable-asm)\n");
        return -1;
    }
#endif
    if( h->param.i_width <= 0 || h->param.i_height <= 0 )
    {
        x264_log( h, X264_LOG_ERROR, "invalid width x height (%dx%d)\n",
                  h->param.i_width, h->param.i_height );
        return -1;
    }

    int i_csp = h->param.i_csp & X264_CSP_MASK;
    if( i_csp <= X264_CSP_NONE || i_csp >= X264_CSP_MAX )
    {
        x264_log( h, X264_LOG_ERROR, "invalid CSP (only I420/YV12/NV12/I444/YV24/BGR/BGRA/RGB supported)\n" );
        return -1;
    }

    if( i_csp < X264_CSP_I444 && (h->param.i_width % 2 || h->param.i_height % 2) )
    {
        x264_log( h, X264_LOG_ERROR, "width or height not divisible by 2 (%dx%d)\n",
                  h->param.i_width, h->param.i_height );
        return -1;
    }

#if HAVE_INTERLACED
    h->param.b_interlaced = !!PARAM_INTERLACED;
#else
    if( h->param.b_interlaced )
    {
        x264_log( h, X264_LOG_ERROR, "not compiled with interlaced support\n" );
        return -1;
    }
#endif

    if( (h->param.crop_rect.i_left + h->param.crop_rect.i_right ) >= h->param.i_width ||
        (h->param.crop_rect.i_top  + h->param.crop_rect.i_bottom) >= h->param.i_height )
    {
        x264_log( h, X264_LOG_ERROR, "invalid crop-rect %u,%u,%u,%u\n", h->param.crop_rect.i_left,
                  h->param.crop_rect.i_top, h->param.crop_rect.i_right,  h->param.crop_rect.i_bottom );
        return -1;
    }

    if( h->param.i_threads == X264_THREADS_AUTO )
        h->param.i_threads = x264_cpu_num_processors() * (h->param.b_sliced_threads?2:3)/2;
    h->param.i_threads = x264_clip3( h->param.i_threads, 1, X264_THREAD_MAX );
    if( h->param.i_threads > 1 )
    {
#if !HAVE_THREAD
        x264_log( h, X264_LOG_WARNING, "not compiled with thread support!\n");
        h->param.i_threads = 1;
#endif
        /* Avoid absurdly small thread slices as they can reduce performance
         * and VBV compliance.  Capped at an arbitrary 4 rows per thread. */
        if( h->param.b_sliced_threads )
        {
            int max_threads = (h->param.i_height+15)/16 / 4;
            h->param.i_threads = X264_MIN( h->param.i_threads, max_threads );
        }
    }
    else
        h->param.b_sliced_threads = 0;
    h->i_thread_frames = h->param.b_sliced_threads ? 1 : h->param.i_threads;
    if( h->i_thread_frames > 1 )
        h->param.nalu_process = NULL;

    h->param.i_keyint_max = x264_clip3( h->param.i_keyint_max, 1, X264_KEYINT_MAX_INFINITE );
    if( h->param.i_keyint_max == 1 )
    {
        h->param.b_intra_refresh = 0;
        h->param.analyse.i_weighted_pred = 0;
    }

    h->param.i_frame_packing = x264_clip3( h->param.i_frame_packing, -1, 5 );

    /* Detect default ffmpeg settings and terminate with an error. */
    if( b_open )
    {
        int score = 0;
        score += h->param.analyse.i_me_range == 0;
        score += h->param.rc.i_qp_step == 3;
        score += h->param.i_keyint_max == 12;
        score += h->param.rc.i_qp_min == 2;
        score += h->param.rc.i_qp_max == 31;
        score += h->param.rc.f_qcompress == 0.5;
        score += fabs(h->param.rc.f_ip_factor - 1.25) < 0.01;
        score += fabs(h->param.rc.f_pb_factor - 1.25) < 0.01;
        score += h->param.analyse.inter == 0 && h->param.analyse.i_subpel_refine == 8;
        if( score >= 5 )
        {
            x264_log( h, X264_LOG_ERROR, "broken ffmpeg default settings detected\n" );
            x264_log( h, X264_LOG_ERROR, "use an encoding preset (e.g. -vpre medium)\n" );
            x264_log( h, X264_LOG_ERROR, "preset usage: -vpre <speed> -vpre <profile>\n" );
            x264_log( h, X264_LOG_ERROR, "speed presets are listed in x264 --help\n" );
            x264_log( h, X264_LOG_ERROR, "profile is optional; x264 defaults to high\n" );
            return -1;
        }
    }

    if( h->param.rc.i_rc_method < 0 || h->param.rc.i_rc_method > 2 )
    {
        x264_log( h, X264_LOG_ERROR, "no ratecontrol method specified\n" );
        return -1;
    }
    h->param.rc.f_rf_constant = x264_clip3f( h->param.rc.f_rf_constant, -QP_BD_OFFSET, 51 );
    h->param.rc.f_rf_constant_max = x264_clip3f( h->param.rc.f_rf_constant_max, -QP_BD_OFFSET, 51 );
    h->param.rc.i_qp_constant = x264_clip3( h->param.rc.i_qp_constant, 0, QP_MAX );
    h->param.analyse.i_subpel_refine = x264_clip3( h->param.analyse.i_subpel_refine, 0, 11 );
    h->param.rc.f_ip_factor = X264_MAX( h->param.rc.f_ip_factor, 0.01f );
    h->param.rc.f_pb_factor = X264_MAX( h->param.rc.f_pb_factor, 0.01f );
    if( h->param.rc.i_rc_method == X264_RC_CRF )
    {
        h->param.rc.i_qp_constant = h->param.rc.f_rf_constant + QP_BD_OFFSET;
        h->param.rc.i_bitrate = 0;
    }
    if( (h->param.rc.i_rc_method == X264_RC_CQP || h->param.rc.i_rc_method == X264_RC_CRF)
        && h->param.rc.i_qp_constant == 0 )
    {
        h->mb.b_lossless = 1;
        h->param.i_cqm_preset = X264_CQM_FLAT;
        h->param.psz_cqm_file = NULL;
        h->param.rc.i_rc_method = X264_RC_CQP;
        h->param.rc.f_ip_factor = 1;
        h->param.rc.f_pb_factor = 1;
        h->param.analyse.b_psnr = 0;
        h->param.analyse.b_ssim = 0;
        h->param.analyse.i_chroma_qp_offset = 0;
        h->param.analyse.i_trellis = 0;
        h->param.analyse.b_fast_pskip = 0;
        h->param.analyse.i_noise_reduction = 0;
        h->param.analyse.b_psy = 0;
        h->param.i_bframe = 0;
        /* 8x8dct is not useful without RD in CAVLC lossless */
        if( !h->param.b_cabac && h->param.analyse.i_subpel_refine < 6 )
            h->param.analyse.b_transform_8x8 = 0;
    }
    if( h->param.rc.i_rc_method == X264_RC_CQP )
    {
        float qp_p = h->param.rc.i_qp_constant;
        float qp_i = qp_p - 6*log2f( h->param.rc.f_ip_factor );
        float qp_b = qp_p + 6*log2f( h->param.rc.f_pb_factor );
        h->param.rc.i_qp_min = x264_clip3( (int)(X264_MIN3( qp_p, qp_i, qp_b )), 0, QP_MAX );
        h->param.rc.i_qp_max = x264_clip3( (int)(X264_MAX3( qp_p, qp_i, qp_b ) + .999), 0, QP_MAX );
        h->param.rc.i_aq_mode = 0;
        h->param.rc.b_mb_tree = 0;
        h->param.rc.i_bitrate = 0;
    }
    h->param.rc.i_qp_max = x264_clip3( h->param.rc.i_qp_max, 0, QP_MAX );
    h->param.rc.i_qp_min = x264_clip3( h->param.rc.i_qp_min, 0, h->param.rc.i_qp_max );
    h->param.rc.i_qp_step = x264_clip3( h->param.rc.i_qp_step, 0, QP_MAX );
    h->param.rc.i_bitrate = x264_clip3( h->param.rc.i_bitrate, 0, 2000000 );
    h->param.rc.i_vbv_buffer_size = x264_clip3( h->param.rc.i_vbv_buffer_size, 0, 2000000 );
    h->param.rc.i_vbv_max_bitrate = x264_clip3( h->param.rc.i_vbv_max_bitrate, 0, 2000000 );
    h->param.rc.f_vbv_buffer_init = x264_clip3f( h->param.rc.f_vbv_buffer_init, 0, 2000000 );
    if( h->param.rc.i_vbv_buffer_size )
    {
        if( h->param.rc.i_rc_method == X264_RC_CQP )
        {
            x264_log( h, X264_LOG_WARNING, "VBV is incompatible with constant QP, ignored.\n" );
            h->param.rc.i_vbv_max_bitrate = 0;
            h->param.rc.i_vbv_buffer_size = 0;
        }
        else if( h->param.rc.i_vbv_max_bitrate == 0 )
        {
            if( h->param.rc.i_rc_method == X264_RC_ABR )
            {
                x264_log( h, X264_LOG_WARNING, "VBV maxrate unspecified, assuming CBR\n" );
                h->param.rc.i_vbv_max_bitrate = h->param.rc.i_bitrate;
            }
            else
            {
                x264_log( h, X264_LOG_WARNING, "VBV bufsize set but maxrate unspecified, ignored\n" );
                h->param.rc.i_vbv_buffer_size = 0;
            }
        }
        else if( h->param.rc.i_vbv_max_bitrate < h->param.rc.i_bitrate &&
                 h->param.rc.i_rc_method == X264_RC_ABR )
        {
            x264_log( h, X264_LOG_WARNING, "max bitrate less than average bitrate, assuming CBR\n" );
            h->param.rc.i_vbv_max_bitrate = h->param.rc.i_bitrate;
        }
    }
    else if( h->param.rc.i_vbv_max_bitrate )
    {
        x264_log( h, X264_LOG_WARNING, "VBV maxrate specified, but no bufsize, ignored\n" );
        h->param.rc.i_vbv_max_bitrate = 0;
    }

    h->param.i_slice_max_size = X264_MAX( h->param.i_slice_max_size, 0 );
    h->param.i_slice_max_mbs = X264_MAX( h->param.i_slice_max_mbs, 0 );

    int max_slices = (h->param.i_height+((16<<PARAM_INTERLACED)-1))/(16<<PARAM_INTERLACED);
    if( h->param.b_sliced_threads )
        h->param.i_slice_count = x264_clip3( h->param.i_threads, 0, max_slices );
    else
    {
        h->param.i_slice_count = x264_clip3( h->param.i_slice_count, 0, max_slices );
        if( h->param.i_slice_max_mbs || h->param.i_slice_max_size )
            h->param.i_slice_count = 0;
    }

    if( h->param.b_bluray_compat )
    {
        h->param.i_bframe_pyramid = X264_MIN( X264_B_PYRAMID_STRICT, h->param.i_bframe_pyramid );
        h->param.i_bframe = X264_MIN( h->param.i_bframe, 3 );
        h->param.b_aud = 1;
        h->param.i_nal_hrd = X264_MAX( h->param.i_nal_hrd, X264_NAL_HRD_VBR );
        h->param.i_slice_max_size = 0;
        h->param.i_slice_max_mbs = 0;
        h->param.b_intra_refresh = 0;
        h->param.i_frame_reference = X264_MIN( h->param.i_frame_reference, 6 );
        h->param.i_dpb_size = X264_MIN( h->param.i_dpb_size, 6 );
        /* Due to the proliferation of broken players that don't handle dupes properly. */
        h->param.analyse.i_weighted_pred = X264_MIN( h->param.analyse.i_weighted_pred, X264_WEIGHTP_SIMPLE );
        if( h->param.b_fake_interlaced )
            h->param.b_pic_struct = 1;
    }

    h->param.i_frame_reference = x264_clip3( h->param.i_frame_reference, 1, X264_REF_MAX );
    h->param.i_dpb_size = x264_clip3( h->param.i_dpb_size, 1, X264_REF_MAX );
    if( h->param.i_scenecut_threshold < 0 )
        h->param.i_scenecut_threshold = 0;
    h->param.analyse.i_direct_mv_pred = x264_clip3( h->param.analyse.i_direct_mv_pred, X264_DIRECT_PRED_NONE, X264_DIRECT_PRED_AUTO );
    if( !h->param.analyse.i_subpel_refine && h->param.analyse.i_direct_mv_pred > X264_DIRECT_PRED_SPATIAL )
    {
        x264_log( h, X264_LOG_WARNING, "subme=0 + direct=temporal is not supported\n" );
        h->param.analyse.i_direct_mv_pred = X264_DIRECT_PRED_SPATIAL;
    }
    h->param.i_bframe = x264_clip3( h->param.i_bframe, 0, X264_MIN( X264_BFRAME_MAX, h->param.i_keyint_max-1 ) );
    h->param.i_bframe_bias = x264_clip3( h->param.i_bframe_bias, -90, 100 );
    if( h->param.i_bframe <= 1 )
        h->param.i_bframe_pyramid = X264_B_PYRAMID_NONE;
    h->param.i_bframe_pyramid = x264_clip3( h->param.i_bframe_pyramid, X264_B_PYRAMID_NONE, X264_B_PYRAMID_NORMAL );
    h->param.i_bframe_adaptive = x264_clip3( h->param.i_bframe_adaptive, X264_B_ADAPT_NONE, X264_B_ADAPT_TRELLIS );
    if( !h->param.i_bframe )
    {
        h->param.i_bframe_adaptive = X264_B_ADAPT_NONE;
        h->param.analyse.i_direct_mv_pred = 0;
        h->param.analyse.b_weighted_bipred = 0;
        h->param.b_open_gop = 0;
    }
    if( h->param.b_intra_refresh && h->param.i_bframe_pyramid == X264_B_PYRAMID_NORMAL )
    {
        x264_log( h, X264_LOG_WARNING, "b-pyramid normal + intra-refresh is not supported\n" );
        h->param.i_bframe_pyramid = X264_B_PYRAMID_STRICT;
    }
    if( h->param.b_intra_refresh && (h->param.i_frame_reference > 1 || h->param.i_dpb_size > 1) )
    {
        x264_log( h, X264_LOG_WARNING, "ref > 1 + intra-refresh is not supported\n" );
        h->param.i_frame_reference = 1;
        h->param.i_dpb_size = 1;
    }
    if( h->param.b_intra_refresh && h->param.b_open_gop )
    {
        x264_log( h, X264_LOG_WARNING, "intra-refresh is not compatible with open-gop\n" );
        h->param.b_open_gop = 0;
    }
    float fps = h->param.i_fps_num > 0 && h->param.i_fps_den > 0 ? (float) h->param.i_fps_num / h->param.i_fps_den : 25.0;
    if( h->param.i_keyint_min == X264_KEYINT_MIN_AUTO )
        h->param.i_keyint_min = X264_MIN( h->param.i_keyint_max / 10, fps );
    h->param.i_keyint_min = x264_clip3( h->param.i_keyint_min, 1, h->param.i_keyint_max/2+1 );
    h->param.rc.i_lookahead = x264_clip3( h->param.rc.i_lookahead, 0, X264_LOOKAHEAD_MAX );
    {
        int maxrate = X264_MAX( h->param.rc.i_vbv_max_bitrate, h->param.rc.i_bitrate );
        float bufsize = maxrate ? (float)h->param.rc.i_vbv_buffer_size / maxrate : 0;
        h->param.rc.i_lookahead = X264_MIN( h->param.rc.i_lookahead, X264_MAX( h->param.i_keyint_max, bufsize*fps ) );
    }

    if( !h->param.i_timebase_num || !h->param.i_timebase_den || !(h->param.b_vfr_input || h->param.b_pulldown) )
    {
        h->param.i_timebase_num = h->param.i_fps_den;
        h->param.i_timebase_den = h->param.i_fps_num;
    }

    h->param.rc.f_qcompress = x264_clip3f( h->param.rc.f_qcompress, 0.0, 1.0 );
    if( h->param.i_keyint_max == 1 || h->param.rc.f_qcompress == 1 )
        h->param.rc.b_mb_tree = 0;
    if( (!h->param.b_intra_refresh && h->param.i_keyint_max != X264_KEYINT_MAX_INFINITE) &&
        !h->param.rc.i_lookahead && h->param.rc.b_mb_tree )
    {
        x264_log( h, X264_LOG_WARNING, "lookaheadless mb-tree requires intra refresh or infinite keyint\n" );
        h->param.rc.b_mb_tree = 0;
    }
    if( h->param.rc.b_stat_read )
        h->param.rc.i_lookahead = 0;
#if HAVE_THREAD
    if( h->param.i_sync_lookahead < 0 )
        h->param.i_sync_lookahead = h->param.i_bframe + 1;
    h->param.i_sync_lookahead = X264_MIN( h->param.i_sync_lookahead, X264_LOOKAHEAD_MAX );
    if( h->param.rc.b_stat_read || h->i_thread_frames == 1 )
        h->param.i_sync_lookahead = 0;
#else
    h->param.i_sync_lookahead = 0;
#endif

    h->param.i_deblocking_filter_alphac0 = x264_clip3( h->param.i_deblocking_filter_alphac0, -6, 6 );
    h->param.i_deblocking_filter_beta    = x264_clip3( h->param.i_deblocking_filter_beta, -6, 6 );
    h->param.analyse.i_luma_deadzone[0] = x264_clip3( h->param.analyse.i_luma_deadzone[0], 0, 32 );
    h->param.analyse.i_luma_deadzone[1] = x264_clip3( h->param.analyse.i_luma_deadzone[1], 0, 32 );

    h->param.i_cabac_init_idc = x264_clip3( h->param.i_cabac_init_idc, 0, 2 );

    if( h->param.i_cqm_preset < X264_CQM_FLAT || h->param.i_cqm_preset > X264_CQM_CUSTOM )
        h->param.i_cqm_preset = X264_CQM_FLAT;

    if( h->param.analyse.i_me_method < X264_ME_DIA ||
        h->param.analyse.i_me_method > X264_ME_TESA )
        h->param.analyse.i_me_method = X264_ME_HEX;
    h->param.analyse.i_me_range = x264_clip3( h->param.analyse.i_me_range, 4, 1024 );
    if( h->param.analyse.i_me_range > 16 && h->param.analyse.i_me_method <= X264_ME_HEX )
        h->param.analyse.i_me_range = 16;
    if( h->param.analyse.i_me_method == X264_ME_TESA &&
        (h->mb.b_lossless || h->param.analyse.i_subpel_refine <= 1) )
        h->param.analyse.i_me_method = X264_ME_ESA;
    h->param.analyse.b_mixed_references = h->param.analyse.b_mixed_references && h->param.i_frame_reference > 1;
    h->param.analyse.inter &= X264_ANALYSE_PSUB16x16|X264_ANALYSE_PSUB8x8|X264_ANALYSE_BSUB16x16|
                              X264_ANALYSE_I4x4|X264_ANALYSE_I8x8;
    h->param.analyse.intra &= X264_ANALYSE_I4x4|X264_ANALYSE_I8x8;
    if( !(h->param.analyse.inter & X264_ANALYSE_PSUB16x16) )
        h->param.analyse.inter &= ~X264_ANALYSE_PSUB8x8;
    if( !h->param.analyse.b_transform_8x8 )
    {
        h->param.analyse.inter &= ~X264_ANALYSE_I8x8;
        h->param.analyse.intra &= ~X264_ANALYSE_I8x8;
    }
    h->param.analyse.i_trellis = x264_clip3( h->param.analyse.i_trellis, 0, 2 );
    h->param.rc.i_aq_mode = x264_clip3( h->param.rc.i_aq_mode, 0, 2 );
    h->param.rc.f_aq_strength = x264_clip3f( h->param.rc.f_aq_strength, 0, 3 );
    if( h->param.rc.f_aq_strength == 0 )
        h->param.rc.i_aq_mode = 0;

    if( h->param.i_log_level < X264_LOG_INFO )
    {
        h->param.analyse.b_psnr = 0;
        h->param.analyse.b_ssim = 0;
    }
    /* Warn users trying to measure PSNR/SSIM with psy opts on. */
    if( b_open && (h->param.analyse.b_psnr || h->param.analyse.b_ssim) )
    {
        char *s = NULL;

        if( h->param.analyse.b_psy )
        {
            s = h->param.analyse.b_psnr ? "psnr" : "ssim";
            x264_log( h, X264_LOG_WARNING, "--%s used with psy on: results will be invalid!\n", s );
        }
        else if( !h->param.rc.i_aq_mode && h->param.analyse.b_ssim )
        {
            x264_log( h, X264_LOG_WARNING, "--ssim used with AQ off: results will be invalid!\n" );
            s = "ssim";
        }
        else if(  h->param.rc.i_aq_mode && h->param.analyse.b_psnr )
        {
            x264_log( h, X264_LOG_WARNING, "--psnr used with AQ on: results will be invalid!\n" );
            s = "psnr";
        }
        if( s )
            x264_log( h, X264_LOG_WARNING, "--tune %s should be used if attempting to benchmark %s!\n", s, s );
    }

    if( !h->param.analyse.b_psy )
    {
        h->param.analyse.f_psy_rd = 0;
        h->param.analyse.f_psy_trellis = 0;
    }
    h->param.analyse.f_psy_rd = x264_clip3f( h->param.analyse.f_psy_rd, 0, 10 );
    h->param.analyse.f_psy_trellis = x264_clip3f( h->param.analyse.f_psy_trellis, 0, 10 );
    h->mb.i_psy_rd = h->param.analyse.i_subpel_refine >= 6 ? FIX8( h->param.analyse.f_psy_rd ) : 0;
    h->mb.i_psy_trellis = h->param.analyse.i_trellis ? FIX8( h->param.analyse.f_psy_trellis / 4 ) : 0;
    h->param.analyse.i_chroma_qp_offset = x264_clip3(h->param.analyse.i_chroma_qp_offset, -32, 32);
    /* In 4:4:4 mode, chroma gets twice as much resolution, so we can halve its quality. */
    if( b_open && i_csp >= X264_CSP_I444 && i_csp < X264_CSP_BGR && h->param.analyse.b_psy )
        h->param.analyse.i_chroma_qp_offset += 6;
    /* Psy RDO increases overall quantizers to improve the quality of luma--this indirectly hurts chroma quality */
    /* so we lower the chroma QP offset to compensate */
    if( b_open && h->mb.i_psy_rd )
        h->param.analyse.i_chroma_qp_offset -= h->param.analyse.f_psy_rd < 0.25 ? 1 : 2;
    /* Psy trellis has a similar effect. */
    if( b_open && h->mb.i_psy_trellis )
        h->param.analyse.i_chroma_qp_offset -= h->param.analyse.f_psy_trellis < 0.25 ? 1 : 2;
    h->param.analyse.i_chroma_qp_offset = x264_clip3(h->param.analyse.i_chroma_qp_offset, -12, 12);
    /* MB-tree requires AQ to be on, even if the strength is zero. */
    if( !h->param.rc.i_aq_mode && h->param.rc.b_mb_tree )
    {
        h->param.rc.i_aq_mode = 1;
        h->param.rc.f_aq_strength = 0;
    }
    h->param.analyse.i_noise_reduction = x264_clip3( h->param.analyse.i_noise_reduction, 0, 1<<16 );
    if( h->param.analyse.i_subpel_refine >= 10 && (h->param.analyse.i_trellis != 2 || !h->param.rc.i_aq_mode) )
        h->param.analyse.i_subpel_refine = 9;

    {
        const x264_level_t *l = x264_levels;
        if( h->param.i_level_idc < 0 )
        {
            int maxrate_bak = h->param.rc.i_vbv_max_bitrate;
            if( h->param.rc.i_rc_method == X264_RC_ABR && h->param.rc.i_vbv_buffer_size <= 0 )
                h->param.rc.i_vbv_max_bitrate = h->param.rc.i_bitrate * 2;
            x264_sps_init( h->sps, h->param.i_sps_id, &h->param );
            do h->param.i_level_idc = l->level_idc;
                while( l[1].level_idc && x264_validate_levels( h, 0 ) && l++ );
            h->param.rc.i_vbv_max_bitrate = maxrate_bak;
        }
        else
        {
            while( l->level_idc && l->level_idc != h->param.i_level_idc )
                l++;
            if( l->level_idc == 0 )
            {
                x264_log( h, X264_LOG_ERROR, "invalid level_idc: %d\n", h->param.i_level_idc );
                return -1;
            }
        }
        if( h->param.analyse.i_mv_range <= 0 )
            h->param.analyse.i_mv_range = l->mv_range >> PARAM_INTERLACED;
        else
            h->param.analyse.i_mv_range = x264_clip3(h->param.analyse.i_mv_range, 32, 512 >> PARAM_INTERLACED);
    }

    h->param.analyse.i_weighted_pred = x264_clip3( h->param.analyse.i_weighted_pred, X264_WEIGHTP_NONE, X264_WEIGHTP_SMART );

    if( PARAM_INTERLACED )
    {
        if( h->param.analyse.i_me_method >= X264_ME_ESA )
        {
            x264_log( h, X264_LOG_WARNING, "interlace + me=esa is not implemented\n" );
            h->param.analyse.i_me_method = X264_ME_UMH;
        }
        if( h->param.analyse.i_weighted_pred > 0 )
        {
            x264_log( h, X264_LOG_WARNING, "interlace + weightp is not implemented\n" );
            h->param.analyse.i_weighted_pred = X264_WEIGHTP_NONE;
        }
    }

    if( !h->param.analyse.i_weighted_pred && h->param.rc.b_mb_tree && h->param.analyse.b_psy )
        h->param.analyse.i_weighted_pred = X264_WEIGHTP_FAKE;

    if( h->i_thread_frames > 1 )
    {
        int r = h->param.analyse.i_mv_range_thread;
        int r2;
        if( r <= 0 )
        {
            // half of the available space is reserved and divided evenly among the threads,
            // the rest is allocated to whichever thread is far enough ahead to use it.
            // reserving more space increases quality for some videos, but costs more time
            // in thread synchronization.
            int max_range = (h->param.i_height + X264_THREAD_HEIGHT) / h->i_thread_frames - X264_THREAD_HEIGHT;
            r = max_range / 2;
        }
        r = X264_MAX( r, h->param.analyse.i_me_range );
        r = X264_MIN( r, h->param.analyse.i_mv_range );
        // round up to use the whole mb row
        r2 = (r & ~15) + ((-X264_THREAD_HEIGHT) & 15);
        if( r2 < r )
            r2 += 16;
        x264_log( h, X264_LOG_DEBUG, "using mv_range_thread = %d\n", r2 );
        h->param.analyse.i_mv_range_thread = r2;
    }

    if( h->param.rc.f_rate_tolerance < 0 )
        h->param.rc.f_rate_tolerance = 0;
    if( h->param.rc.f_qblur < 0 )
        h->param.rc.f_qblur = 0;
    if( h->param.rc.f_complexity_blur < 0 )
        h->param.rc.f_complexity_blur = 0;

    h->param.i_sps_id &= 31;

    if( PARAM_INTERLACED )
        h->param.b_pic_struct = 1;

    h->param.i_nal_hrd = x264_clip3( h->param.i_nal_hrd, X264_NAL_HRD_NONE, X264_NAL_HRD_CBR );

    if( h->param.i_nal_hrd && !h->param.rc.i_vbv_buffer_size )
    {
        x264_log( h, X264_LOG_WARNING, "NAL HRD parameters require VBV parameters\n" );
        h->param.i_nal_hrd = X264_NAL_HRD_NONE;
    }

    if( h->param.i_nal_hrd == X264_NAL_HRD_CBR &&
       (h->param.rc.i_bitrate != h->param.rc.i_vbv_max_bitrate || !h->param.rc.i_vbv_max_bitrate) )
    {
        x264_log( h, X264_LOG_WARNING, "CBR HRD requires constant bitrate\n" );
        h->param.i_nal_hrd = X264_NAL_HRD_VBR;
    }

    /* ensure the booleans are 0 or 1 so they can be used in math */
#define BOOLIFY(x) h->param.x = !!h->param.x
    BOOLIFY( b_cabac );
    BOOLIFY( b_constrained_intra );
    BOOLIFY( b_deblocking_filter );
    BOOLIFY( b_deterministic );
    BOOLIFY( b_sliced_threads );
    BOOLIFY( b_interlaced );
    BOOLIFY( b_intra_refresh );
    BOOLIFY( b_visualize );
    BOOLIFY( b_aud );
    BOOLIFY( b_repeat_headers );
    BOOLIFY( b_annexb );
    BOOLIFY( b_vfr_input );
    BOOLIFY( b_pulldown );
    BOOLIFY( b_tff );
    BOOLIFY( b_pic_struct );
    BOOLIFY( b_fake_interlaced );
    BOOLIFY( b_open_gop );
    BOOLIFY( b_bluray_compat );
    BOOLIFY( analyse.b_transform_8x8 );
    BOOLIFY( analyse.b_weighted_bipred );
    BOOLIFY( analyse.b_chroma_me );
    BOOLIFY( analyse.b_mixed_references );
    BOOLIFY( analyse.b_fast_pskip );
    BOOLIFY( analyse.b_dct_decimate );
    BOOLIFY( analyse.b_psy );
    BOOLIFY( analyse.b_psnr );
    BOOLIFY( analyse.b_ssim );
    BOOLIFY( rc.b_stat_write );
    BOOLIFY( rc.b_stat_read );
    BOOLIFY( rc.b_mb_tree );
#undef BOOLIFY

    return 0;
}

static void mbcmp_init( x264_t *h )
{
    int satd = !h->mb.b_lossless && h->param.analyse.i_subpel_refine > 1;
    memcpy( h->pixf.mbcmp, satd ? h->pixf.satd : h->pixf.sad_aligned, sizeof(h->pixf.mbcmp) );
    memcpy( h->pixf.mbcmp_unaligned, satd ? h->pixf.satd : h->pixf.sad, sizeof(h->pixf.mbcmp_unaligned) );
    h->pixf.intra_mbcmp_x3_16x16 = satd ? h->pixf.intra_satd_x3_16x16 : h->pixf.intra_sad_x3_16x16;
    h->pixf.intra_mbcmp_x3_8x8c = satd ? h->pixf.intra_satd_x3_8x8c : h->pixf.intra_sad_x3_8x8c;
    h->pixf.intra_mbcmp_x3_8x8 = satd ? h->pixf.intra_sa8d_x3_8x8 : h->pixf.intra_sad_x3_8x8;
    h->pixf.intra_mbcmp_x3_4x4 = satd ? h->pixf.intra_satd_x3_4x4 : h->pixf.intra_sad_x3_4x4;
    satd &= h->param.analyse.i_me_method == X264_ME_TESA;
    memcpy( h->pixf.fpelcmp, satd ? h->pixf.satd : h->pixf.sad, sizeof(h->pixf.fpelcmp) );
    memcpy( h->pixf.fpelcmp_x3, satd ? h->pixf.satd_x3 : h->pixf.sad_x3, sizeof(h->pixf.fpelcmp_x3) );
    memcpy( h->pixf.fpelcmp_x4, satd ? h->pixf.satd_x4 : h->pixf.sad_x4, sizeof(h->pixf.fpelcmp_x4) );
}

static void x264_set_aspect_ratio( x264_t *h, x264_param_t *param, int initial )
{
    /* VUI */
    if( param->vui.i_sar_width > 0 && param->vui.i_sar_height > 0 )
    {
        uint32_t i_w = param->vui.i_sar_width;
        uint32_t i_h = param->vui.i_sar_height;
        uint32_t old_w = h->param.vui.i_sar_width;
        uint32_t old_h = h->param.vui.i_sar_height;

        x264_reduce_fraction( &i_w, &i_h );

        while( i_w > 65535 || i_h > 65535 )
        {
            i_w /= 2;
            i_h /= 2;
        }

        x264_reduce_fraction( &i_w, &i_h );

        if( i_w != old_w || i_h != old_h || initial )
        {
            h->param.vui.i_sar_width = 0;
            h->param.vui.i_sar_height = 0;
            if( i_w == 0 || i_h == 0 )
                x264_log( h, X264_LOG_WARNING, "cannot create valid sample aspect ratio\n" );
            else
            {
                x264_log( h, initial?X264_LOG_INFO:X264_LOG_DEBUG, "using SAR=%d/%d\n", i_w, i_h );
                h->param.vui.i_sar_width = i_w;
                h->param.vui.i_sar_height = i_h;
            }
            x264_sps_init( h->sps, h->param.i_sps_id, &h->param );
        }
    }
}

/****************************************************************************
 * x264_encoder_open:
 ****************************************************************************/
x264_t *x264_encoder_open( x264_param_t *param )
{
    x264_t *h;
    char buf[1000], *p;
    int qp, i_slicetype_length;

    CHECKED_MALLOCZERO( h, sizeof(x264_t) );

    /* Create a copy of param */
    memcpy( &h->param, param, sizeof(x264_param_t) );

    if( param->param_free )
        param->param_free( param );

    if( x264_threading_init() )
    {
        x264_log( h, X264_LOG_ERROR, "unable to initialize threading\n" );
        goto fail;
    }

    if( x264_validate_parameters( h, 1 ) < 0 )
        goto fail;

    if( h->param.psz_cqm_file )
        if( x264_cqm_parse_file( h, h->param.psz_cqm_file ) < 0 )
            goto fail;

    if( h->param.rc.psz_stat_out )
        h->param.rc.psz_stat_out = strdup( h->param.rc.psz_stat_out );
    if( h->param.rc.psz_stat_in )
        h->param.rc.psz_stat_in = strdup( h->param.rc.psz_stat_in );

    x264_reduce_fraction( &h->param.i_fps_num, &h->param.i_fps_den );
    x264_reduce_fraction( &h->param.i_timebase_num, &h->param.i_timebase_den );

    /* Init x264_t */
    h->i_frame = -1;
    h->i_frame_num = 0;
    h->i_idr_pic_id = 0;

    if( (uint64_t)h->param.i_timebase_den * 2 > UINT32_MAX )
    {
        x264_log( h, X264_LOG_ERROR, "Effective timebase denominator %u exceeds H.264 maximum\n", h->param.i_timebase_den );
        goto fail;
    }

    x264_sps_init( h->sps, h->param.i_sps_id, &h->param );
    x264_pps_init( h->pps, h->param.i_sps_id, &h->param, h->sps );

    x264_set_aspect_ratio( h, &h->param, 1 );

    x264_validate_levels( h, 1 );

    h->chroma_qp_table = i_chroma_qp_table + 12 + h->pps->i_chroma_qp_index_offset;

    if( x264_cqm_init( h ) < 0 )
        goto fail;

    h->mb.i_mb_width = h->sps->i_mb_width;
    h->mb.i_mb_height = h->sps->i_mb_height;
    h->mb.i_mb_count = h->mb.i_mb_width * h->mb.i_mb_height;
    /* Adaptive MBAFF and subme 0 are not supported as we require halving motion
     * vectors during prediction, resulting in hpel mvs.
     * The chosen solution is to make MBAFF non-adaptive in this case. */
    h->mb.b_adaptive_mbaff = PARAM_INTERLACED && h->param.analyse.i_subpel_refine;

    /* Init frames. */
    if( h->param.i_bframe_adaptive == X264_B_ADAPT_TRELLIS && !h->param.rc.b_stat_read )
        h->frames.i_delay = X264_MAX(h->param.i_bframe,3)*4;
    else
        h->frames.i_delay = h->param.i_bframe;
    if( h->param.rc.b_mb_tree || h->param.rc.i_vbv_buffer_size )
        h->frames.i_delay = X264_MAX( h->frames.i_delay, h->param.rc.i_lookahead );
    i_slicetype_length = h->frames.i_delay;
    h->frames.i_delay += h->i_thread_frames - 1;
    h->frames.i_delay += h->param.i_sync_lookahead;
    h->frames.i_delay += h->param.b_vfr_input;
    h->frames.i_bframe_delay = h->param.i_bframe ? (h->param.i_bframe_pyramid ? 2 : 1) : 0;

    h->frames.i_max_ref0 = h->param.i_frame_reference;
    h->frames.i_max_ref1 = X264_MIN( h->sps->vui.i_num_reorder_frames, h->param.i_frame_reference );
    h->frames.i_max_dpb  = h->sps->vui.i_max_dec_frame_buffering;
    h->frames.b_have_lowres = !h->param.rc.b_stat_read
        && ( h->param.rc.i_rc_method == X264_RC_ABR
          || h->param.rc.i_rc_method == X264_RC_CRF
          || h->param.i_bframe_adaptive
          || h->param.i_scenecut_threshold
          || h->param.rc.b_mb_tree
          || h->param.analyse.i_weighted_pred );
    h->frames.b_have_lowres |= h->param.rc.b_stat_read && h->param.rc.i_vbv_buffer_size > 0;
    h->frames.b_have_sub8x8_esa = !!(h->param.analyse.inter & X264_ANALYSE_PSUB8x8);

    h->frames.i_last_idr =
    h->frames.i_last_keyframe = - h->param.i_keyint_max;
    h->frames.i_input    = 0;
    h->frames.i_largest_pts = h->frames.i_second_largest_pts = -1;
    h->frames.i_poc_last_open_gop = -1;

    CHECKED_MALLOCZERO( h->frames.unused[0], (h->frames.i_delay + 3) * sizeof(x264_frame_t *) );
    /* Allocate room for max refs plus a few extra just in case. */
    CHECKED_MALLOCZERO( h->frames.unused[1], (h->i_thread_frames + X264_REF_MAX + 4) * sizeof(x264_frame_t *) );
    CHECKED_MALLOCZERO( h->frames.current, (h->param.i_sync_lookahead + h->param.i_bframe
                        + h->i_thread_frames + 3) * sizeof(x264_frame_t *) );
    if( h->param.analyse.i_weighted_pred > 0 )
        CHECKED_MALLOCZERO( h->frames.blank_unused, h->i_thread_frames * 4 * sizeof(x264_frame_t *) );
    h->i_ref[0] = h->i_ref[1] = 0;
    h->i_cpb_delay = h->i_coded_fields = h->i_disp_fields = 0;
    h->i_prev_duration = ((uint64_t)h->param.i_fps_den * h->sps->vui.i_time_scale) / ((uint64_t)h->param.i_fps_num * h->sps->vui.i_num_units_in_tick);
    h->i_disp_fields_last_frame = -1;
    x264_rdo_init();

    /* init CPU functions */
    x264_predict_16x16_init( h->param.cpu, h->predict_16x16 );
    x264_predict_8x8c_init( h->param.cpu, h->predict_8x8c );
    x264_predict_8x8_init( h->param.cpu, h->predict_8x8, &h->predict_8x8_filter );
    x264_predict_4x4_init( h->param.cpu, h->predict_4x4 );
    if( h->param.b_cabac )
        x264_cabac_init( h );
    else
        x264_cavlc_init();
    x264_pixel_init( h->param.cpu, &h->pixf );
    x264_dct_init( h->param.cpu, &h->dctf );
    x264_zigzag_init( h->param.cpu, &h->zigzagf_progressive, &h->zigzagf_interlaced );
    memcpy( &h->zigzagf, PARAM_INTERLACED ? &h->zigzagf_interlaced : &h->zigzagf_progressive, sizeof(h->zigzagf) );
    x264_mc_init( h->param.cpu, &h->mc );
    x264_quant_init( h, h->param.cpu, &h->quantf );
    x264_deblock_init( h->param.cpu, &h->loopf, PARAM_INTERLACED );
    x264_bitstream_init( h->param.cpu, &h->bsf );
    x264_dct_init_weights();

    mbcmp_init( h );

    p = buf + sprintf( buf, "using cpu capabilities:" );
    for( int i = 0; x264_cpu_names[i].flags; i++ )
    {
        if( !strcmp(x264_cpu_names[i].name, "SSE2")
            && h->param.cpu & (X264_CPU_SSE2_IS_FAST|X264_CPU_SSE2_IS_SLOW) )
            continue;
        if( !strcmp(x264_cpu_names[i].name, "SSE3")
            && (h->param.cpu & X264_CPU_SSSE3 || !(h->param.cpu & X264_CPU_CACHELINE_64)) )
            continue;
        if( !strcmp(x264_cpu_names[i].name, "SSE4.1")
            && (h->param.cpu & X264_CPU_SSE42) )
            continue;
        if( (h->param.cpu & x264_cpu_names[i].flags) == x264_cpu_names[i].flags
            && (!i || x264_cpu_names[i].flags != x264_cpu_names[i-1].flags) )
            p += sprintf( p, " %s", x264_cpu_names[i].name );
    }
    if( !h->param.cpu )
        p += sprintf( p, " none!" );
    x264_log( h, X264_LOG_INFO, "%s\n", buf );

    float *logs = x264_analyse_prepare_costs( h );
    if( !logs )
        goto fail;
    for( qp = X264_MIN( h->param.rc.i_qp_min, QP_MAX_SPEC ); qp <= h->param.rc.i_qp_max; qp++ )
        if( x264_analyse_init_costs( h, logs, qp ) )
            goto fail;
    if( x264_analyse_init_costs( h, logs, X264_LOOKAHEAD_QP ) )
        goto fail;
    x264_free( logs );

    static const uint16_t cost_mv_correct[7] = { 24, 47, 95, 189, 379, 757, 1515 };
    /* Checks for known miscompilation issues. */
    if( h->cost_mv[X264_LOOKAHEAD_QP][2013] != cost_mv_correct[BIT_DEPTH-8] )
    {
        x264_log( h, X264_LOG_ERROR, "MV cost test failed: x264 has been miscompiled!\n" );
        goto fail;
    }

    /* Must be volatile or else GCC will optimize it out. */
    volatile int temp = 392;
    if( x264_clz( temp ) != 23 )
    {
        x264_log( h, X264_LOG_ERROR, "CLZ test failed: x264 has been miscompiled!\n" );
#if ARCH_X86 || ARCH_X86_64
        x264_log( h, X264_LOG_ERROR, "Are you attempting to run an SSE4a-targeted build on a CPU that\n" );
        x264_log( h, X264_LOG_ERROR, "doesn't support it?\n" );
#endif
        goto fail;
    }

    h->out.i_nal = 0;
    h->out.i_bitstream = X264_MAX( 1000000, h->param.i_width * h->param.i_height * 4
        * ( h->param.rc.i_rc_method == X264_RC_ABR ? pow( 0.95, h->param.rc.i_qp_min )
          : pow( 0.95, h->param.rc.i_qp_constant ) * X264_MAX( 1, h->param.rc.f_ip_factor )));

    h->nal_buffer_size = h->out.i_bitstream * 3/2 + 4;
    CHECKED_MALLOC( h->nal_buffer, h->nal_buffer_size );

    if( h->param.i_threads > 1 &&
        x264_threadpool_init( &h->threadpool, h->param.i_threads, (void*)x264_encoder_thread_init, h ) )
        goto fail;

    h->thread[0] = h;
    for( int i = 1; i < h->param.i_threads + !!h->param.i_sync_lookahead; i++ )
        CHECKED_MALLOC( h->thread[i], sizeof(x264_t) );

    for( int i = 0; i < h->param.i_threads; i++ )
    {
        int init_nal_count = h->param.i_slice_count + 3;
        int allocate_threadlocal_data = !h->param.b_sliced_threads || !i;
        if( i > 0 )
            *h->thread[i] = *h;

        if( allocate_threadlocal_data )
        {
            h->thread[i]->fdec = x264_frame_pop_unused( h, 1 );
            if( !h->thread[i]->fdec )
                goto fail;
        }
        else
            h->thread[i]->fdec = h->thread[0]->fdec;

        CHECKED_MALLOC( h->thread[i]->out.p_bitstream, h->out.i_bitstream );
        /* Start each thread with room for init_nal_count NAL units; it'll realloc later if needed. */
        CHECKED_MALLOC( h->thread[i]->out.nal, init_nal_count*sizeof(x264_nal_t) );
        h->thread[i]->out.i_nals_allocated = init_nal_count;

        if( allocate_threadlocal_data && x264_macroblock_cache_allocate( h->thread[i] ) < 0 )
            goto fail;
    }

    if( x264_lookahead_init( h, i_slicetype_length ) )
        goto fail;

    for( int i = 0; i < h->param.i_threads; i++ )
        if( x264_macroblock_thread_allocate( h->thread[i], 0 ) < 0 )
            goto fail;

    if( x264_ratecontrol_new( h ) < 0 )
        goto fail;

    if( h->param.i_nal_hrd )
    {
        x264_log( h, X264_LOG_DEBUG, "HRD bitrate: %i bits/sec\n", h->sps->vui.hrd.i_bit_rate_unscaled );
        x264_log( h, X264_LOG_DEBUG, "CPB size: %i bits\n", h->sps->vui.hrd.i_cpb_size_unscaled );
    }

    if( h->param.psz_dump_yuv )
    {
        /* create or truncate the reconstructed video file */
        FILE *f = fopen( h->param.psz_dump_yuv, "w" );
        if( !f )
        {
            x264_log( h, X264_LOG_ERROR, "dump_yuv: can't write to %s\n", h->param.psz_dump_yuv );
            goto fail;
        }
        else if( !x264_is_regular_file( f ) )
        {
            x264_log( h, X264_LOG_ERROR, "dump_yuv: incompatible with non-regular file %s\n", h->param.psz_dump_yuv );
            goto fail;
        }
        fclose( f );
    }

    const char *profile = h->sps->i_profile_idc == PROFILE_BASELINE ? "Constrained Baseline" :
                          h->sps->i_profile_idc == PROFILE_MAIN ? "Main" :
                          h->sps->i_profile_idc == PROFILE_HIGH ? "High" :
                          h->sps->i_profile_idc == PROFILE_HIGH10 ? (h->sps->b_constraint_set3 == 1 ? "High 10 Intra" : "High 10") :
                          h->sps->b_constraint_set3 == 1 ? "High 4:4:4 Intra" : "High 4:4:4 Predictive";
    char level[4];
    snprintf( level, sizeof(level), "%d.%d", h->sps->i_level_idc/10, h->sps->i_level_idc%10 );
    if( h->sps->i_level_idc == 9 || ( h->sps->i_level_idc == 11 && h->sps->b_constraint_set3 &&
        (h->sps->i_profile_idc >= PROFILE_BASELINE && h->sps->i_profile_idc <= PROFILE_EXTENDED) ) )
        strcpy( level, "1b" );

    if( h->sps->i_profile_idc < PROFILE_HIGH10 )
    {
        x264_log( h, X264_LOG_INFO, "profile %s, level %s\n",
            profile, level );
    }
    else
    {
        x264_log( h, X264_LOG_INFO, "profile %s, level %s, %s %d-bit\n",
            profile, level, CHROMA444 ? "4:4:4" : "4:2:0", BIT_DEPTH );
    }

    return h;
fail:
    x264_free( h );
    return NULL;
}

/****************************************************************************
 * x264_encoder_reconfig:
 ****************************************************************************/
int x264_encoder_reconfig( x264_t *h, x264_param_t *param )
{
    int rc_reconfig = 0;
    h = h->thread[h->thread[0]->i_thread_phase];
    x264_set_aspect_ratio( h, param, 0 );
#define COPY(var) h->param.var = param->var
    COPY( i_frame_reference ); // but never uses more refs than initially specified
    COPY( i_bframe_bias );
    if( h->param.i_scenecut_threshold )
        COPY( i_scenecut_threshold ); // can't turn it on or off, only vary the threshold
    COPY( b_deblocking_filter );
    COPY( i_deblocking_filter_alphac0 );
    COPY( i_deblocking_filter_beta );
    COPY( i_frame_packing );
    COPY( analyse.inter );
    COPY( analyse.intra );
    COPY( analyse.i_direct_mv_pred );
    /* Scratch buffer prevents me_range from being increased for esa/tesa */
    if( h->param.analyse.i_me_method < X264_ME_ESA || param->analyse.i_me_range < h->param.analyse.i_me_range )
        COPY( analyse.i_me_range );
    COPY( analyse.i_noise_reduction );
    /* We can't switch out of subme=0 during encoding. */
    if( h->param.analyse.i_subpel_refine )
        COPY( analyse.i_subpel_refine );
    COPY( analyse.i_trellis );
    COPY( analyse.b_chroma_me );
    COPY( analyse.b_dct_decimate );
    COPY( analyse.b_fast_pskip );
    COPY( analyse.b_mixed_references );
    COPY( analyse.f_psy_rd );
    COPY( analyse.f_psy_trellis );
    COPY( crop_rect );
    // can only twiddle these if they were enabled to begin with:
    if( h->param.analyse.i_me_method >= X264_ME_ESA || param->analyse.i_me_method < X264_ME_ESA )
        COPY( analyse.i_me_method );
    if( h->param.analyse.i_me_method >= X264_ME_ESA && !h->frames.b_have_sub8x8_esa )
        h->param.analyse.inter &= ~X264_ANALYSE_PSUB8x8;
    if( h->pps->b_transform_8x8_mode )
        COPY( analyse.b_transform_8x8 );
    if( h->frames.i_max_ref1 > 1 )
        COPY( i_bframe_pyramid );
    COPY( i_slice_max_size );
    COPY( i_slice_max_mbs );
    COPY( i_slice_count );
    COPY( b_tff );

    /* VBV can't be turned on if it wasn't on to begin with */
    if( h->param.rc.i_vbv_max_bitrate > 0 && h->param.rc.i_vbv_buffer_size > 0 &&
          param->rc.i_vbv_max_bitrate > 0 &&   param->rc.i_vbv_buffer_size > 0 )
    {
        rc_reconfig |= h->param.rc.i_vbv_max_bitrate != param->rc.i_vbv_max_bitrate;
        rc_reconfig |= h->param.rc.i_vbv_buffer_size != param->rc.i_vbv_buffer_size;
        rc_reconfig |= h->param.rc.i_bitrate != param->rc.i_bitrate;
        COPY( rc.i_vbv_max_bitrate );
        COPY( rc.i_vbv_buffer_size );
        COPY( rc.i_bitrate );
    }
    rc_reconfig |= h->param.rc.f_rf_constant != param->rc.f_rf_constant;
    rc_reconfig |= h->param.rc.f_rf_constant_max != param->rc.f_rf_constant_max;
    COPY( rc.f_rf_constant );
    COPY( rc.f_rf_constant_max );
#undef COPY

    mbcmp_init( h );

    int ret = x264_validate_parameters( h, 0 );

    /* Supported reconfiguration options (1-pass only):
     * vbv-maxrate
     * vbv-bufsize
     * crf
     * bitrate (CBR only) */
    if( !ret && rc_reconfig )
        x264_ratecontrol_init_reconfigurable( h, 0 );

    return ret;
}

/****************************************************************************
 * x264_encoder_parameters:
 ****************************************************************************/
void x264_encoder_parameters( x264_t *h, x264_param_t *param )
{
    memcpy( param, &h->thread[h->i_thread_phase]->param, sizeof(x264_param_t) );
}

/* internal usage */
static void x264_nal_start( x264_t *h, int i_type, int i_ref_idc )
{
    x264_nal_t *nal = &h->out.nal[h->out.i_nal];

    nal->i_ref_idc        = i_ref_idc;
    nal->i_type           = i_type;
    nal->b_long_startcode = 1;

    nal->i_payload= 0;
    nal->p_payload= &h->out.p_bitstream[bs_pos( &h->out.bs ) / 8];
}

/* if number of allocated nals is not enough, re-allocate a larger one. */
static int x264_nal_check_buffer( x264_t *h )
{
    if( h->out.i_nal >= h->out.i_nals_allocated )
    {
        x264_nal_t *new_out = x264_malloc( sizeof(x264_nal_t) * (h->out.i_nals_allocated*2) );
        if( !new_out )
            return -1;
        memcpy( new_out, h->out.nal, sizeof(x264_nal_t) * (h->out.i_nals_allocated) );
        x264_free( h->out.nal );
        h->out.nal = new_out;
        h->out.i_nals_allocated *= 2;
    }
    return 0;
}

static int x264_nal_end( x264_t *h )
{
    x264_nal_t *nal = &h->out.nal[h->out.i_nal];
    uint8_t *end = &h->out.p_bitstream[bs_pos( &h->out.bs ) / 8];
    nal->i_payload = end - nal->p_payload;
    /* nal_escape_mmx reads past the end of the input.
     * While undefined padding wouldn't actually affect the output, it makes valgrind unhappy. */
    memset( end, 0xff, 32 );
    if( h->param.nalu_process )
        h->param.nalu_process( h, nal );
    h->out.i_nal++;

    return x264_nal_check_buffer( h );
}

static int x264_encoder_encapsulate_nals( x264_t *h, int start )
{
    int nal_size = 0, previous_nal_size = 0;

    if( h->param.nalu_process )
    {
        for( int i = start; i < h->out.i_nal; i++ )
            nal_size += h->out.nal[i].i_payload;
        return nal_size;
    }

    for( int i = 0; i < start; i++ )
        previous_nal_size += h->out.nal[i].i_payload;

    for( int i = start; i < h->out.i_nal; i++ )
        nal_size += h->out.nal[i].i_payload;

    /* Worst-case NAL unit escaping: reallocate the buffer if it's too small. */
    int necessary_size = nal_size * 3/2 + h->out.i_nal * 4;
    if( h->nal_buffer_size < necessary_size )
    {
        h->nal_buffer_size = necessary_size * 2;
        uint8_t *buf = x264_malloc( h->nal_buffer_size );
        if( !buf )
            return -1;
        if( previous_nal_size )
            memcpy( buf, h->nal_buffer, previous_nal_size );
        x264_free( h->nal_buffer );
        h->nal_buffer = buf;
    }

    uint8_t *nal_buffer = h->nal_buffer + previous_nal_size;

    for( int i = start; i < h->out.i_nal; i++ )
    {
        h->out.nal[i].b_long_startcode = !i || h->out.nal[i].i_type == NAL_SPS || h->out.nal[i].i_type == NAL_PPS;
        x264_nal_encode( h, nal_buffer, &h->out.nal[i] );
        nal_buffer += h->out.nal[i].i_payload;
    }

    x264_emms();

    return nal_buffer - (h->nal_buffer + previous_nal_size);
}

/****************************************************************************
 * x264_encoder_headers:
 ****************************************************************************/
int x264_encoder_headers( x264_t *h, x264_nal_t **pp_nal, int *pi_nal )
{
    int frame_size = 0;
    /* init bitstream context */
    h->out.i_nal = 0;
    bs_init( &h->out.bs, h->out.p_bitstream, h->out.i_bitstream );

    /* Write SEI, SPS and PPS. */

    /* generate sequence parameters */
    x264_nal_start( h, NAL_SPS, NAL_PRIORITY_HIGHEST );
    x264_sps_write( &h->out.bs, h->sps );
    if( x264_nal_end( h ) )
        return -1;

    /* generate picture parameters */
    x264_nal_start( h, NAL_PPS, NAL_PRIORITY_HIGHEST );
    x264_pps_write( &h->out.bs, h->sps, h->pps );
    if( x264_nal_end( h ) )
        return -1;

    /* identify ourselves */
    x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
    if( x264_sei_version_write( h, &h->out.bs ) )
        return -1;
    if( x264_nal_end( h ) )
        return -1;

    frame_size = x264_encoder_encapsulate_nals( h, 0 );
    if( frame_size < 0 )
        return -1;

    /* now set output*/
    *pi_nal = h->out.i_nal;
    *pp_nal = &h->out.nal[0];
    h->out.i_nal = 0;

    return frame_size;
}

/* Check to see whether we have chosen a reference list ordering different
 * from the standard's default. */
static inline void x264_reference_check_reorder( x264_t *h )
{
    /* The reorder check doesn't check for missing frames, so just
     * force a reorder if one of the reference list is corrupt. */
    for( int i = 0; h->frames.reference[i]; i++ )
        if( h->frames.reference[i]->b_corrupt )
        {
            h->b_ref_reorder[0] = 1;
            return;
        }
    for( int list = 0; list <= (h->sh.i_type == SLICE_TYPE_B); list++ )
        for( int i = 0; i < h->i_ref[list] - 1; i++ )
        {
            int framenum_diff = h->fref[list][i+1]->i_frame_num - h->fref[list][i]->i_frame_num;
            int poc_diff = h->fref[list][i+1]->i_poc - h->fref[list][i]->i_poc;
            /* P and B-frames use different default orders. */
            if( h->sh.i_type == SLICE_TYPE_P ? framenum_diff > 0 : list == 1 ? poc_diff < 0 : poc_diff > 0 )
            {
                h->b_ref_reorder[list] = 1;
                return;
            }
        }
}

/* return -1 on failure, else return the index of the new reference frame */
int x264_weighted_reference_duplicate( x264_t *h, int i_ref, const x264_weight_t *w )
{
    int i = h->i_ref[0];
    int j = 1;
    x264_frame_t *newframe;
    if( i <= 1 ) /* empty list, definitely can't duplicate frame */
        return -1;

    //Duplication is only used in X264_WEIGHTP_SMART
    if( h->param.analyse.i_weighted_pred != X264_WEIGHTP_SMART )
        return -1;

    /* Duplication is a hack to compensate for crappy rounding in motion compensation.
     * With high bit depth, it's not worth doing, so turn it off except in the case of
     * unweighted dupes. */
    if( BIT_DEPTH > 8 && w != x264_weight_none )
        return -1;

    newframe = x264_frame_pop_blank_unused( h );
    if( !newframe )
        return -1;

    //FIXME: probably don't need to copy everything
    *newframe = *h->fref[0][i_ref];
    newframe->i_reference_count = 1;
    newframe->orig = h->fref[0][i_ref];
    newframe->b_duplicate = 1;
    memcpy( h->fenc->weight[j], w, sizeof(h->fenc->weight[i]) );

    /* shift the frames to make space for the dupe. */
    h->b_ref_reorder[0] = 1;
    if( h->i_ref[0] < X264_REF_MAX )
        ++h->i_ref[0];
    h->fref[0][X264_REF_MAX-1] = NULL;
    x264_frame_unshift( &h->fref[0][j], newframe );

    return j;
}

static void x264_weighted_pred_init( x264_t *h )
{
    /* for now no analysis and set all weights to nothing */
    for( int i_ref = 0; i_ref < h->i_ref[0]; i_ref++ )
        h->fenc->weighted[i_ref] = h->fref[0][i_ref]->filtered[0][0];

    // FIXME: This only supports weighting of one reference frame
    // and duplicates of that frame.
    h->fenc->i_lines_weighted = 0;

    for( int i_ref = 0; i_ref < (h->i_ref[0] << SLICE_MBAFF); i_ref++ )
        for( int i = 0; i < 3; i++ )
            h->sh.weight[i_ref][i].weightfn = NULL;


    if( h->sh.i_type != SLICE_TYPE_P || h->param.analyse.i_weighted_pred <= 0 )
        return;

    int i_padv = PADV << PARAM_INTERLACED;
    int denom = -1;
    int weightplane[2] = { 0, 0 };
    int buffer_next = 0;
    for( int i = 0; i < 3; i++ )
    {
        for( int j = 0; j < h->i_ref[0]; j++ )
        {
            if( h->fenc->weight[j][i].weightfn )
            {
                h->sh.weight[j][i] = h->fenc->weight[j][i];
                // if weight is useless, don't write it to stream
                if( h->sh.weight[j][i].i_scale == 1<<h->sh.weight[j][i].i_denom && h->sh.weight[j][i].i_offset == 0 )
                    h->sh.weight[j][i].weightfn = NULL;
                else
                {
                    if( !weightplane[!!i] )
                    {
                        weightplane[!!i] = 1;
                        h->sh.weight[0][!!i].i_denom = denom = h->sh.weight[j][i].i_denom;
                        assert( x264_clip3( denom, 0, 7 ) == denom );
                    }

                    assert( h->sh.weight[j][i].i_denom == denom );
                    if( !i )
                    {
                        h->fenc->weighted[j] = h->mb.p_weight_buf[buffer_next++] + h->fenc->i_stride[0] * i_padv + PADH;
                        //scale full resolution frame
                        if( h->param.i_threads == 1 )
                        {
                            pixel *src = h->fref[0][j]->filtered[0][0] - h->fref[0][j]->i_stride[0]*i_padv - PADH;
                            pixel *dst = h->fenc->weighted[j] - h->fenc->i_stride[0]*i_padv - PADH;
                            int stride = h->fenc->i_stride[0];
                            int width = h->fenc->i_width[0] + PADH*2;
                            int height = h->fenc->i_lines[0] + i_padv*2;
                            x264_weight_scale_plane( h, dst, stride, src, stride, width, height, &h->sh.weight[j][0] );
                            h->fenc->i_lines_weighted = height;
                        }
                    }
                }
            }
        }
    }

    if( weightplane[1] )
        for( int i = 0; i < h->i_ref[0]; i++ )
        {
            if( h->sh.weight[i][1].weightfn && !h->sh.weight[i][2].weightfn )
            {
                h->sh.weight[i][2].i_scale = 1 << h->sh.weight[0][1].i_denom;
                h->sh.weight[i][2].i_offset = 0;
            }
            else if( h->sh.weight[i][2].weightfn && !h->sh.weight[i][1].weightfn )
            {
                h->sh.weight[i][1].i_scale = 1 << h->sh.weight[0][1].i_denom;
                h->sh.weight[i][1].i_offset = 0;
            }
        }

    if( !weightplane[0] )
        h->sh.weight[0][0].i_denom = 0;
    if( !weightplane[1] )
        h->sh.weight[0][1].i_denom = 0;
    h->sh.weight[0][2].i_denom = h->sh.weight[0][1].i_denom;
}

static inline int x264_reference_distance( x264_t *h, x264_frame_t *frame )
{
    if( h->param.i_frame_packing == 5 )
        return abs((h->fenc->i_frame&~1) - (frame->i_frame&~1)) +
                  ((h->fenc->i_frame&1) != (frame->i_frame&1));
    else
        return abs(h->fenc->i_frame - frame->i_frame);
}

static inline void x264_reference_build_list( x264_t *h, int i_poc )
{
    int b_ok;

    /* build ref list 0/1 */
    h->mb.pic.i_fref[0] = h->i_ref[0] = 0;
    h->mb.pic.i_fref[1] = h->i_ref[1] = 0;
    if( h->sh.i_type == SLICE_TYPE_I )
        return;

    for( int i = 0; h->frames.reference[i]; i++ )
    {
        if( h->frames.reference[i]->b_corrupt )
            continue;
        if( h->frames.reference[i]->i_poc < i_poc )
            h->fref[0][h->i_ref[0]++] = h->frames.reference[i];
        else if( h->frames.reference[i]->i_poc > i_poc )
            h->fref[1][h->i_ref[1]++] = h->frames.reference[i];
    }

    /* Order reference lists by distance from the current frame. */
    for( int list = 0; list < 2; list++ )
    {
        h->fref_nearest[list] = h->fref[list][0];
        do
        {
            b_ok = 1;
            for( int i = 0; i < h->i_ref[list] - 1; i++ )
            {
                if( list ? h->fref[list][i+1]->i_poc < h->fref_nearest[list]->i_poc
                         : h->fref[list][i+1]->i_poc > h->fref_nearest[list]->i_poc )
                    h->fref_nearest[list] = h->fref[list][i+1];
                if( x264_reference_distance( h, h->fref[list][i] ) > x264_reference_distance( h, h->fref[list][i+1] ) )
                {
                    XCHG( x264_frame_t*, h->fref[list][i], h->fref[list][i+1] );
                    b_ok = 0;
                    break;
                }
            }
        } while( !b_ok );
    }

    if( h->sh.i_mmco_remove_from_end )
        for( int i = h->i_ref[0]-1; i >= h->i_ref[0] - h->sh.i_mmco_remove_from_end; i-- )
        {
            int diff = h->i_frame_num - h->fref[0][i]->i_frame_num;
            h->sh.mmco[h->sh.i_mmco_command_count].i_poc = h->fref[0][i]->i_poc;
            h->sh.mmco[h->sh.i_mmco_command_count++].i_difference_of_pic_nums = diff;
        }

    x264_reference_check_reorder( h );

    h->i_ref[1] = X264_MIN( h->i_ref[1], h->frames.i_max_ref1 );
    h->i_ref[0] = X264_MIN( h->i_ref[0], h->frames.i_max_ref0 );
    h->i_ref[0] = X264_MIN( h->i_ref[0], h->param.i_frame_reference ); // if reconfig() has lowered the limit

    /* For Blu-ray compliance, don't reference frames outside of the minigop. */
    if( IS_X264_TYPE_B( h->fenc->i_type ) && h->param.b_bluray_compat )
        h->i_ref[0] = X264_MIN( h->i_ref[0], IS_X264_TYPE_B( h->fref[0][0]->i_type ) + 1 );

    /* add duplicates */
    if( h->fenc->i_type == X264_TYPE_P )
    {
        int idx = -1;
        if( h->param.analyse.i_weighted_pred >= X264_WEIGHTP_SIMPLE )
        {
            x264_weight_t w[3];
            w[1].weightfn = w[2].weightfn = NULL;
            if( h->param.rc.b_stat_read )
                x264_ratecontrol_set_weights( h, h->fenc );

            if( !h->fenc->weight[0][0].weightfn )
            {
                h->fenc->weight[0][0].i_denom = 0;
                SET_WEIGHT( w[0], 1, 1, 0, -1 );
                idx = x264_weighted_reference_duplicate( h, 0, w );
            }
            else
            {
                if( h->fenc->weight[0][0].i_scale == 1<<h->fenc->weight[0][0].i_denom )
                {
                    SET_WEIGHT( h->fenc->weight[0][0], 1, 1, 0, h->fenc->weight[0][0].i_offset );
                }
                x264_weighted_reference_duplicate( h, 0, x264_weight_none );
                if( h->fenc->weight[0][0].i_offset > -128 )
                {
                    w[0] = h->fenc->weight[0][0];
                    w[0].i_offset--;
                    h->mc.weight_cache( h, &w[0] );
                    idx = x264_weighted_reference_duplicate( h, 0, w );
                }
            }
        }
        h->mb.ref_blind_dupe = idx;
    }

    assert( h->i_ref[0] + h->i_ref[1] <= X264_REF_MAX );
    h->mb.pic.i_fref[0] = h->i_ref[0];
    h->mb.pic.i_fref[1] = h->i_ref[1];
}

static void x264_fdec_filter_row( x264_t *h, int mb_y, int b_inloop )
{
    /* mb_y is the mb to be encoded next, not the mb to be filtered here */
    int b_hpel = h->fdec->b_kept_as_ref;
    int b_deblock = h->sh.i_disable_deblocking_filter_idc != 1;
    int b_end = mb_y == h->i_threadslice_end;
    int b_measure_quality = 1;
    int min_y = mb_y - (1 << SLICE_MBAFF);
    int b_start = min_y == h->i_threadslice_start;
    /* Even in interlaced mode, deblocking never modifies more than 4 pixels
     * above each MB, as bS=4 doesn't happen for the top of interlaced mbpairs. */
    int minpix_y = min_y*16 - 4 * !b_start;
    int maxpix_y = mb_y*16 - 4 * !b_end;
    b_deblock &= b_hpel || h->param.psz_dump_yuv;
    if( h->param.b_sliced_threads && b_start && min_y && !b_inloop )
    {
        b_deblock = 0;         /* We already deblocked on the inloop pass. */
        b_measure_quality = 0; /* We already measured quality on the inloop pass. */
    }
    if( mb_y & SLICE_MBAFF )
        return;
    if( min_y < h->i_threadslice_start )
        return;

    if( b_deblock )
        for( int y = min_y; y < mb_y; y += (1 << SLICE_MBAFF) )
            x264_frame_deblock_row( h, y );

    /* FIXME: Prediction requires different borders for interlaced/progressive mc,
     * but the actual image data is equivalent. For now, maintain this
     * consistency by copying deblocked pixels between planes. */
    if( PARAM_INTERLACED )
        for( int p = 0; p < h->fdec->i_plane; p++ )
            for( int i = minpix_y>>(!CHROMA444 && p); i < maxpix_y>>(!CHROMA444 && p); i++ )
                memcpy( h->fdec->plane_fld[p] + i*h->fdec->i_stride[p],
                        h->fdec->plane[p]     + i*h->fdec->i_stride[p],
                        h->mb.i_mb_width*16*sizeof(pixel) );

    if( b_hpel )
    {
        int end = mb_y == h->mb.i_mb_height;
        x264_frame_expand_border( h, h->fdec, min_y, end );
        if( h->param.analyse.i_subpel_refine )
        {
            x264_frame_filter( h, h->fdec, min_y, end );
            x264_frame_expand_border_filtered( h, h->fdec, min_y, end );
        }
    }

    if( SLICE_MBAFF )
        for( int i = 0; i < 3; i++ )
        {
            XCHG( pixel *, h->intra_border_backup[0][i], h->intra_border_backup[3][i] );
            XCHG( pixel *, h->intra_border_backup[1][i], h->intra_border_backup[4][i] );
        }

    if( h->i_thread_frames > 1 && h->fdec->b_kept_as_ref )
        x264_frame_cond_broadcast( h->fdec, mb_y*16 + (b_end ? 10000 : -(X264_THREAD_HEIGHT << SLICE_MBAFF)) );

    if( b_measure_quality )
    {
        maxpix_y = X264_MIN( maxpix_y, h->param.i_height );
        if( h->param.analyse.b_psnr )
        {
            for( int p = 0; p < (CHROMA444 ? 3 : 1); p++ )
                h->stat.frame.i_ssd[p] += x264_pixel_ssd_wxh( &h->pixf,
                    h->fdec->plane[p] + minpix_y * h->fdec->i_stride[p], h->fdec->i_stride[p],
                    h->fenc->plane[p] + minpix_y * h->fenc->i_stride[p], h->fenc->i_stride[p],
                    h->param.i_width, maxpix_y-minpix_y );
            if( !CHROMA444 )
            {
                uint64_t ssd_u, ssd_v;
                x264_pixel_ssd_nv12( &h->pixf,
                    h->fdec->plane[1] + (minpix_y>>1) * h->fdec->i_stride[1], h->fdec->i_stride[1],
                    h->fenc->plane[1] + (minpix_y>>1) * h->fenc->i_stride[1], h->fenc->i_stride[1],
                    h->param.i_width>>1, (maxpix_y-minpix_y)>>1, &ssd_u, &ssd_v );
                h->stat.frame.i_ssd[1] += ssd_u;
                h->stat.frame.i_ssd[2] += ssd_v;
            }
        }

        if( h->param.analyse.b_ssim )
        {
            int ssim_cnt;
            x264_emms();
            /* offset by 2 pixels to avoid alignment of ssim blocks with dct blocks,
             * and overlap by 4 */
            minpix_y += b_start ? 2 : -6;
            h->stat.frame.f_ssim +=
                x264_pixel_ssim_wxh( &h->pixf,
                    h->fdec->plane[0] + 2+minpix_y*h->fdec->i_stride[0], h->fdec->i_stride[0],
                    h->fenc->plane[0] + 2+minpix_y*h->fenc->i_stride[0], h->fenc->i_stride[0],
                    h->param.i_width-2, maxpix_y-minpix_y, h->scratch_buffer, &ssim_cnt );
            h->stat.frame.i_ssim_cnt += ssim_cnt;
        }
    }
}

static inline int x264_reference_update( x264_t *h )
{
    if( !h->fdec->b_kept_as_ref )
    {
        if( h->i_thread_frames > 1 )
        {
            x264_frame_push_unused( h, h->fdec );
            h->fdec = x264_frame_pop_unused( h, 1 );
            if( !h->fdec )
                return -1;
        }
        return 0;
    }

    /* apply mmco from previous frame. */
    for( int i = 0; i < h->sh.i_mmco_command_count; i++ )
        for( int j = 0; h->frames.reference[j]; j++ )
            if( h->frames.reference[j]->i_poc == h->sh.mmco[i].i_poc )
                x264_frame_push_unused( h, x264_frame_shift( &h->frames.reference[j] ) );

    /* move frame in the buffer */
    x264_frame_push( h->frames.reference, h->fdec );
    if( h->frames.reference[h->sps->i_num_ref_frames] )
        x264_frame_push_unused( h, x264_frame_shift( h->frames.reference ) );
    h->fdec = x264_frame_pop_unused( h, 1 );
    if( !h->fdec )
        return -1;
    return 0;
}

static inline void x264_reference_reset( x264_t *h )
{
    while( h->frames.reference[0] )
        x264_frame_push_unused( h, x264_frame_pop( h->frames.reference ) );
    h->fdec->i_poc =
    h->fenc->i_poc = 0;
}

static inline void x264_reference_hierarchy_reset( x264_t *h )
{
    int ref;
    int b_hasdelayframe = 0;

    /* look for delay frames -- chain must only contain frames that are disposable */
    for( int i = 0; h->frames.current[i] && IS_DISPOSABLE( h->frames.current[i]->i_type ); i++ )
        b_hasdelayframe |= h->frames.current[i]->i_coded
                        != h->frames.current[i]->i_frame + h->sps->vui.i_num_reorder_frames;

    /* This function must handle b-pyramid and clear frames for open-gop */
    if( h->param.i_bframe_pyramid != X264_B_PYRAMID_STRICT && !b_hasdelayframe && h->frames.i_poc_last_open_gop == -1 )
        return;

    /* Remove last BREF. There will never be old BREFs in the
     * dpb during a BREF decode when pyramid == STRICT */
    for( ref = 0; h->frames.reference[ref]; ref++ )
    {
        if( ( h->param.i_bframe_pyramid == X264_B_PYRAMID_STRICT
            && h->frames.reference[ref]->i_type == X264_TYPE_BREF )
            || ( h->frames.reference[ref]->i_poc < h->frames.i_poc_last_open_gop
            && h->sh.i_type != SLICE_TYPE_B ) )
        {
            int diff = h->i_frame_num - h->frames.reference[ref]->i_frame_num;
            h->sh.mmco[h->sh.i_mmco_command_count].i_difference_of_pic_nums = diff;
            h->sh.mmco[h->sh.i_mmco_command_count++].i_poc = h->frames.reference[ref]->i_poc;
            x264_frame_push_unused( h, x264_frame_shift( &h->frames.reference[ref] ) );
            h->b_ref_reorder[0] = 1;
            ref--;
        }
    }

    /* Prepare room in the dpb for the delayed display time of the later b-frame's */
    if( h->param.i_bframe_pyramid )
        h->sh.i_mmco_remove_from_end = X264_MAX( ref + 2 - h->frames.i_max_dpb, 0 );
}

static inline void x264_slice_init( x264_t *h, int i_nal_type, int i_global_qp )
{
    /* ------------------------ Create slice header  ----------------------- */
    if( i_nal_type == NAL_SLICE_IDR )
    {
        x264_slice_header_init( h, &h->sh, h->sps, h->pps, h->i_idr_pic_id, h->i_frame_num, i_global_qp );

        /* alternate id */
        h->i_idr_pic_id ^= 1;
    }
    else
    {
        x264_slice_header_init( h, &h->sh, h->sps, h->pps, -1, h->i_frame_num, i_global_qp );

        h->sh.i_num_ref_idx_l0_active = h->i_ref[0] <= 0 ? 1 : h->i_ref[0];
        h->sh.i_num_ref_idx_l1_active = h->i_ref[1] <= 0 ? 1 : h->i_ref[1];
        if( h->sh.i_num_ref_idx_l0_active != h->pps->i_num_ref_idx_l0_default_active ||
            (h->sh.i_type == SLICE_TYPE_B && h->sh.i_num_ref_idx_l1_active != h->pps->i_num_ref_idx_l1_default_active) )
        {
            h->sh.b_num_ref_idx_override = 1;
        }
    }

    if( h->fenc->i_type == X264_TYPE_BREF && h->param.b_bluray_compat && h->sh.i_mmco_command_count )
    {
        h->b_sh_backup = 1;
        h->sh_backup = h->sh;
    }

    h->fdec->i_frame_num = h->sh.i_frame_num;

    if( h->sps->i_poc_type == 0 )
    {
        h->sh.i_poc = h->fdec->i_poc;
        if( PARAM_INTERLACED )
        {
            h->sh.i_delta_poc_bottom = h->param.b_tff ? 1 : -1;
            h->sh.i_poc += h->sh.i_delta_poc_bottom == -1;
        }
        else
            h->sh.i_delta_poc_bottom = 0;
        h->fdec->i_delta_poc[0] = h->sh.i_delta_poc_bottom == -1;
        h->fdec->i_delta_poc[1] = h->sh.i_delta_poc_bottom ==  1;
    }
    else
    {
        /* Nothing to do ? */
    }

    x264_macroblock_slice_init( h );
}

static int x264_slice_write( x264_t *h )
{
    int i_skip;
    int mb_xy, i_mb_x, i_mb_y;
    int i_skip_bak = 0; /* Shut up GCC. */
    bs_t UNINIT(bs_bak);
    x264_cabac_t cabac_bak;
    uint8_t cabac_prevbyte_bak = 0; /* Shut up GCC. */
    int mv_bits_bak = 0;
    int tex_bits_bak = 0;
    /* NALUs other than the first use a 3-byte startcode.
     * Add one extra byte for the rbsp, and one more for the final CABAC putbyte.
     * Then add an extra 5 bytes just in case, to account for random NAL escapes and
     * other inaccuracies. */
    int overhead_guess = (NALU_OVERHEAD - (h->param.b_annexb && h->out.i_nal)) + 1 + h->param.b_cabac + 5;
    int slice_max_size = h->param.i_slice_max_size > 0 ? (h->param.i_slice_max_size-overhead_guess)*8 : 0;
    int back_up_bitstream = slice_max_size || (!h->param.b_cabac && h->sps->i_profile_idc < PROFILE_HIGH);
    int starting_bits = bs_pos(&h->out.bs);
    int b_deblock = h->sh.i_disable_deblocking_filter_idc != 1;
    int b_hpel = h->fdec->b_kept_as_ref;
    uint8_t *last_emu_check;
    b_deblock &= b_hpel || h->param.psz_dump_yuv;
    bs_realign( &h->out.bs );

    /* Slice */
    x264_nal_start( h, h->i_nal_type, h->i_nal_ref_idc );
    h->out.nal[h->out.i_nal].i_first_mb = h->sh.i_first_mb;

    /* Slice header */
    x264_macroblock_thread_init( h );

    /* If this isn't the first slice in the threadslice, set the slice QP
     * equal to the last QP in the previous slice for more accurate
     * CABAC initialization. */
    if( h->sh.i_first_mb != h->i_threadslice_start * h->mb.i_mb_width )
    {
        h->sh.i_qp = h->mb.i_last_qp;
        h->sh.i_qp_delta = h->sh.i_qp - h->pps->i_pic_init_qp;
    }

    x264_slice_header_write( &h->out.bs, &h->sh, h->i_nal_ref_idc );
    if( h->param.b_cabac )
    {
        /* alignment needed */
        bs_align_1( &h->out.bs );

        /* init cabac */
        x264_cabac_context_init( h, &h->cabac, h->sh.i_type, x264_clip3( h->sh.i_qp-QP_BD_OFFSET, 0, 51 ), h->sh.i_cabac_init_idc );
        x264_cabac_encode_init ( &h->cabac, h->out.bs.p, h->out.bs.p_end );
        last_emu_check = h->cabac.p;
    }
    else
        last_emu_check = h->out.bs.p;
    h->mb.i_last_qp = h->sh.i_qp;
    h->mb.i_last_dqp = 0;
    h->mb.field_decoding_flag = 0;

    i_mb_y = h->sh.i_first_mb / h->mb.i_mb_width;
    i_mb_x = h->sh.i_first_mb % h->mb.i_mb_width;
    i_skip = 0;

    while( 1 )
    {
        mb_xy = i_mb_x + i_mb_y * h->mb.i_mb_width;
        int mb_spos = bs_pos(&h->out.bs) + x264_cabac_pos(&h->cabac);

        if( !(i_mb_y & SLICE_MBAFF) )
        {
            if( x264_bitstream_check_buffer( h ) )
                return -1;

            if( back_up_bitstream )
            {
                mv_bits_bak = h->stat.frame.i_mv_bits;
                tex_bits_bak = h->stat.frame.i_tex_bits;
                /* We don't need the contexts because flushing the CABAC encoder has no context
                 * dependency and macroblocks are only re-encoded in the case where a slice is
                 * ended (and thus the content of all contexts are thrown away). */
                if( h->param.b_cabac )
                {
                    memcpy( &cabac_bak, &h->cabac, offsetof(x264_cabac_t, f8_bits_encoded) );
                    /* x264's CABAC writer modifies the previous byte during carry, so it has to be
                     * backed up. */
                    cabac_prevbyte_bak = h->cabac.p[-1];
                }
                else
                {
                    bs_bak = h->out.bs;
                    i_skip_bak = i_skip;
                }
            }
        }

        if( i_mb_x == 0 && !h->mb.b_reencode_mb )
            x264_fdec_filter_row( h, i_mb_y, 1 );

        if( PARAM_INTERLACED )
        {
            if( h->mb.b_adaptive_mbaff )
            {
                if( !(i_mb_y&1) )
                {
                    /* FIXME: VSAD is fast but fairly poor at choosing the best interlace type. */
                    h->mb.b_interlaced = x264_field_vsad( h, i_mb_x, i_mb_y );
                    memcpy( &h->zigzagf, MB_INTERLACED ? &h->zigzagf_interlaced : &h->zigzagf_progressive, sizeof(h->zigzagf) );
                    if( !MB_INTERLACED && (i_mb_y+2) == h->mb.i_mb_height )
                        x264_expand_border_mbpair( h, i_mb_x, i_mb_y );
                }
            }
            h->mb.field[mb_xy] = MB_INTERLACED;
        }

        /* load cache */
        if( SLICE_MBAFF )
            x264_macroblock_cache_load_interlaced( h, i_mb_x, i_mb_y );
        else
            x264_macroblock_cache_load_progressive( h, i_mb_x, i_mb_y );

        x264_macroblock_analyse( h );

        /* encode this macroblock -> be careful it can change the mb type to P_SKIP if needed */
reencode:
        x264_macroblock_encode( h );

        if( h->param.b_cabac )
        {
            if( mb_xy > h->sh.i_first_mb && !(SLICE_MBAFF && (i_mb_y&1)) )
                x264_cabac_encode_terminal( &h->cabac );

            if( IS_SKIP( h->mb.i_type ) )
                x264_cabac_mb_skip( h, 1 );
            else
            {
                if( h->sh.i_type != SLICE_TYPE_I )
                    x264_cabac_mb_skip( h, 0 );
                x264_macroblock_write_cabac( h, &h->cabac );
            }
        }
        else
        {
            if( IS_SKIP( h->mb.i_type ) )
                i_skip++;
            else
            {
                if( h->sh.i_type != SLICE_TYPE_I )
                {
                    bs_write_ue( &h->out.bs, i_skip );  /* skip run */
                    i_skip = 0;
                }
                x264_macroblock_write_cavlc( h );
                /* If there was a CAVLC level code overflow, try again at a higher QP. */
                if( h->mb.b_overflow )
                {
                    h->mb.i_chroma_qp = h->chroma_qp_table[++h->mb.i_qp];
                    h->mb.i_skip_intra = 0;
                    h->mb.b_skip_mc = 0;
                    h->mb.b_overflow = 0;
                    h->out.bs = bs_bak;
                    i_skip = i_skip_bak;
                    h->stat.frame.i_mv_bits = mv_bits_bak;
                    h->stat.frame.i_tex_bits = tex_bits_bak;
                    goto reencode;
                }
            }
        }

        int total_bits = bs_pos(&h->out.bs) + x264_cabac_pos(&h->cabac);
        int mb_size = total_bits - mb_spos;

        if( slice_max_size )
        {
            /* Count the skip run, just in case. */
            if( !h->param.b_cabac )
                total_bits += bs_size_ue_big( i_skip );
            /* Check for escape bytes. */
            uint8_t *end = h->param.b_cabac ? h->cabac.p : h->out.bs.p;
            for( ; last_emu_check < end - 2; last_emu_check++ )
                if( last_emu_check[0] == 0 && last_emu_check[1] == 0 && last_emu_check[2] <= 3 )
                {
                    slice_max_size -= 8;
                    last_emu_check++;
                }
            /* We'll just re-encode this last macroblock if we go over the max slice size. */
            if( total_bits - starting_bits > slice_max_size && !h->mb.b_reencode_mb )
            {
                if( mb_xy != h->sh.i_first_mb )
                {
                    h->stat.frame.i_mv_bits = mv_bits_bak;
                    h->stat.frame.i_tex_bits = tex_bits_bak;
                    if( h->param.b_cabac )
                    {
                        memcpy( &h->cabac, &cabac_bak, offsetof(x264_cabac_t, f8_bits_encoded) );
                        h->cabac.p[-1] = cabac_prevbyte_bak;
                    }
                    else
                    {
                        h->out.bs = bs_bak;
                        i_skip = i_skip_bak;
                    }
                    h->mb.b_reencode_mb = 1;
                    if( SLICE_MBAFF )
                    {
                        // set to bottom of previous mbpair
                        if( i_mb_x )
                            h->sh.i_last_mb = mb_xy-1+h->mb.i_mb_stride*(!(i_mb_y&1));
                        else
                            h->sh.i_last_mb = (i_mb_y-2+!(i_mb_y&1))*h->mb.i_mb_stride + h->mb.i_mb_width - 1;
                    }
                    else
                        h->sh.i_last_mb = mb_xy-1;
                    break;
                }
                else
                {
                    h->sh.i_last_mb = mb_xy;
                    h->mb.b_reencode_mb = 0;
                }
            }
            else
                h->mb.b_reencode_mb = 0;
        }

#if HAVE_VISUALIZE
        if( h->param.b_visualize )
            x264_visualize_mb( h );
#endif

        /* save cache */
        x264_macroblock_cache_save( h );

        /* accumulate mb stats */
        h->stat.frame.i_mb_count[h->mb.i_type]++;

        int b_intra = IS_INTRA( h->mb.i_type );
        int b_skip = IS_SKIP( h->mb.i_type );
        if( h->param.i_log_level >= X264_LOG_INFO || h->param.rc.b_stat_write )
        {
            if( !b_intra && !b_skip && !IS_DIRECT( h->mb.i_type ) )
            {
                if( h->mb.i_partition != D_8x8 )
                        h->stat.frame.i_mb_partition[h->mb.i_partition] += 4;
                    else
                        for( int i = 0; i < 4; i++ )
                            h->stat.frame.i_mb_partition[h->mb.i_sub_partition[i]] ++;
                if( h->param.i_frame_reference > 1 )
                    for( int i_list = 0; i_list <= (h->sh.i_type == SLICE_TYPE_B); i_list++ )
                        for( int i = 0; i < 4; i++ )
                        {
                            int i_ref = h->mb.cache.ref[i_list][ x264_scan8[4*i] ];
                            if( i_ref >= 0 )
                                h->stat.frame.i_mb_count_ref[i_list][i_ref] ++;
                        }
            }
        }

        if( h->param.i_log_level >= X264_LOG_INFO )
        {
            if( h->mb.i_cbp_luma | h->mb.i_cbp_chroma )
            {
                if( CHROMA444 )
                {
                    for( int i = 0; i < 4; i++ )
                        if( h->mb.i_cbp_luma & (1 << i) )
                            for( int p = 0; p < 3; p++ )
                            {
                                int s8 = i*4+p*16;
                                int nnz8x8 = M16( &h->mb.cache.non_zero_count[x264_scan8[s8]+0] )
                                           | M16( &h->mb.cache.non_zero_count[x264_scan8[s8]+8] );
                                h->stat.frame.i_mb_cbp[!b_intra + p*2] += !!nnz8x8;
                            }
                }
                else
                {
                    int cbpsum = (h->mb.i_cbp_luma&1) + ((h->mb.i_cbp_luma>>1)&1)
                               + ((h->mb.i_cbp_luma>>2)&1) + (h->mb.i_cbp_luma>>3);
                    h->stat.frame.i_mb_cbp[!b_intra + 0] += cbpsum;
                    h->stat.frame.i_mb_cbp[!b_intra + 2] += !!h->mb.i_cbp_chroma;
                    h->stat.frame.i_mb_cbp[!b_intra + 4] += h->mb.i_cbp_chroma >> 1;
                }
            }
            if( h->mb.i_cbp_luma && !b_intra )
            {
                h->stat.frame.i_mb_count_8x8dct[0] ++;
                h->stat.frame.i_mb_count_8x8dct[1] += h->mb.b_transform_8x8;
            }
            if( b_intra && h->mb.i_type != I_PCM )
            {
                if( h->mb.i_type == I_16x16 )
                    h->stat.frame.i_mb_pred_mode[0][h->mb.i_intra16x16_pred_mode]++;
                else if( h->mb.i_type == I_8x8 )
                    for( int i = 0; i < 16; i += 4 )
                        h->stat.frame.i_mb_pred_mode[1][h->mb.cache.intra4x4_pred_mode[x264_scan8[i]]]++;
                else //if( h->mb.i_type == I_4x4 )
                    for( int i = 0; i < 16; i++ )
                        h->stat.frame.i_mb_pred_mode[2][h->mb.cache.intra4x4_pred_mode[x264_scan8[i]]]++;
                h->stat.frame.i_mb_pred_mode[3][x264_mb_pred_mode8x8c_fix[h->mb.i_chroma_pred_mode]]++;
            }
            h->stat.frame.i_mb_field[b_intra?0:b_skip?2:1] += MB_INTERLACED;
        }

        /* calculate deblock strength values (actual deblocking is done per-row along with hpel) */
        if( b_deblock )
            x264_macroblock_deblock_strength( h );

        x264_ratecontrol_mb( h, mb_size );

        if( mb_xy == h->sh.i_last_mb )
            break;

        if( SLICE_MBAFF )
        {
            i_mb_x += i_mb_y & 1;
            i_mb_y ^= i_mb_x < h->mb.i_mb_width;
        }
        else
            i_mb_x++;
        if( i_mb_x == h->mb.i_mb_width )
        {
            i_mb_y++;
            i_mb_x = 0;
        }
    }
    h->out.nal[h->out.i_nal].i_last_mb = h->sh.i_last_mb;

    if( h->param.b_cabac )
    {
        x264_cabac_encode_flush( h, &h->cabac );
        h->out.bs.p = h->cabac.p;
    }
    else
    {
        if( i_skip > 0 )
            bs_write_ue( &h->out.bs, i_skip );  /* last skip run */
        /* rbsp_slice_trailing_bits */
        bs_rbsp_trailing( &h->out.bs );
        bs_flush( &h->out.bs );
    }
    if( x264_nal_end( h ) )
        return -1;

    if( h->sh.i_last_mb == (h->i_threadslice_end * h->mb.i_mb_width - 1) )
    {
        h->stat.frame.i_misc_bits = bs_pos( &h->out.bs )
                                  + (h->out.i_nal*NALU_OVERHEAD * 8)
                                  - h->stat.frame.i_tex_bits
                                  - h->stat.frame.i_mv_bits;
        x264_fdec_filter_row( h, h->i_threadslice_end, 1 );
    }

    return 0;
}

static void x264_thread_sync_context( x264_t *dst, x264_t *src )
{
    if( dst == src )
        return;

    // reference counting
    for( x264_frame_t **f = src->frames.reference; *f; f++ )
        (*f)->i_reference_count++;
    for( x264_frame_t **f = dst->frames.reference; *f; f++ )
        x264_frame_push_unused( src, *f );
    src->fdec->i_reference_count++;
    x264_frame_push_unused( src, dst->fdec );

    // copy everything except the per-thread pointers and the constants.
    memcpy( &dst->i_frame, &src->i_frame, offsetof(x264_t, mb.type) - offsetof(x264_t, i_frame) );
    dst->param = src->param;
    dst->stat = src->stat;
    dst->pixf = src->pixf;
}

static void x264_thread_sync_stat( x264_t *dst, x264_t *src )
{
    if( dst == src )
        return;
    memcpy( &dst->stat.i_frame_count, &src->stat.i_frame_count, sizeof(dst->stat) - sizeof(dst->stat.frame) );
}

static void *x264_slices_write( x264_t *h )
{
    int i_slice_num = 0;
    int last_thread_mb = h->sh.i_last_mb;

#if HAVE_VISUALIZE
    if( h->param.b_visualize )
        if( x264_visualize_init( h ) )
            return (void *)-1;
#endif

    /* init stats */
    memset( &h->stat.frame, 0, sizeof(h->stat.frame) );
    h->mb.b_reencode_mb = 0;
    while( h->sh.i_first_mb + SLICE_MBAFF*h->mb.i_mb_stride <= last_thread_mb )
    {
        h->sh.i_last_mb = last_thread_mb;
        if( h->param.i_slice_max_mbs )
        {
            if( SLICE_MBAFF )
            {
                // convert first to mbaff form, add slice-max-mbs, then convert back to normal form
                int last_mbaff = 2*(h->sh.i_first_mb % h->mb.i_mb_width)
                    + h->mb.i_mb_width*(h->sh.i_first_mb / h->mb.i_mb_width)
                    + h->param.i_slice_max_mbs - 1;
                int last_x = (last_mbaff % (2*h->mb.i_mb_width))/2;
                int last_y = (last_mbaff / (2*h->mb.i_mb_width))*2 + 1;
                h->sh.i_last_mb = last_x + h->mb.i_mb_stride*last_y;
            }
            else
                h->sh.i_last_mb = h->sh.i_first_mb + h->param.i_slice_max_mbs - 1;
        }
        else if( h->param.i_slice_count && !h->param.b_sliced_threads )
        {
            int height = h->mb.i_mb_height >> PARAM_INTERLACED;
            int width = h->mb.i_mb_width << PARAM_INTERLACED;
            i_slice_num++;
            h->sh.i_last_mb = (height * i_slice_num + h->param.i_slice_count/2) / h->param.i_slice_count * width - 1;
        }
        h->sh.i_last_mb = X264_MIN( h->sh.i_last_mb, last_thread_mb );
        if( x264_stack_align( x264_slice_write, h ) )
            return (void *)-1;
        h->sh.i_first_mb = h->sh.i_last_mb + 1;
        // if i_first_mb is not the last mb in a row then go to the next mb in MBAFF order
        if( SLICE_MBAFF && h->sh.i_first_mb % h->mb.i_mb_width )
            h->sh.i_first_mb -= h->mb.i_mb_stride;
    }

#if HAVE_VISUALIZE
    if( h->param.b_visualize )
    {
        x264_visualize_show( h );
        x264_visualize_close( h );
    }
#endif

    return (void *)0;
}

static int x264_threaded_slices_write( x264_t *h )
{
    /* set first/last mb and sync contexts */
    for( int i = 0; i < h->param.i_threads; i++ )
    {
        x264_t *t = h->thread[i];
        if( i )
        {
            t->param = h->param;
            memcpy( &t->i_frame, &h->i_frame, offsetof(x264_t, rc) - offsetof(x264_t, i_frame) );
        }
        int height = h->mb.i_mb_height >> PARAM_INTERLACED;
        t->i_threadslice_start = ((height *  i    + h->param.i_slice_count/2) / h->param.i_threads) << PARAM_INTERLACED;
        t->i_threadslice_end   = ((height * (i+1) + h->param.i_slice_count/2) / h->param.i_threads) << PARAM_INTERLACED;
        t->sh.i_first_mb = t->i_threadslice_start * h->mb.i_mb_width;
        t->sh.i_last_mb  =   t->i_threadslice_end * h->mb.i_mb_width - 1;
    }

    x264_stack_align( x264_analyse_weight_frame, h, h->mb.i_mb_height*16 + 16 );

    x264_threads_distribute_ratecontrol( h );

    /* dispatch */
    for( int i = 0; i < h->param.i_threads; i++ )
    {
        x264_threadpool_run( h->threadpool, (void*)x264_slices_write, h->thread[i] );
        h->thread[i]->b_thread_active = 1;
    }
    for( int i = 0; i < h->param.i_threads; i++ )
    {
        h->thread[i]->b_thread_active = 0;
        if( (intptr_t)x264_threadpool_wait( h->threadpool, h->thread[i] ) )
            return -1;
    }

    /* Go back and fix up the hpel on the borders between slices. */
    for( int i = 1; i < h->param.i_threads; i++ )
    {
        x264_fdec_filter_row( h->thread[i], h->thread[i]->i_threadslice_start + 1, 0 );
        if( SLICE_MBAFF )
            x264_fdec_filter_row( h->thread[i], h->thread[i]->i_threadslice_start + 2, 0 );
    }

    x264_threads_merge_ratecontrol( h );

    for( int i = 1; i < h->param.i_threads; i++ )
    {
        x264_t *t = h->thread[i];
        for( int j = 0; j < t->out.i_nal; j++ )
        {
            h->out.nal[h->out.i_nal] = t->out.nal[j];
            h->out.i_nal++;
            x264_nal_check_buffer( h );
        }
        /* All entries in stat.frame are ints except for ssd/ssim. */
        for( int j = 0; j < (offsetof(x264_t,stat.frame.i_ssd) - offsetof(x264_t,stat.frame.i_mv_bits)) / sizeof(int); j++ )
            ((int*)&h->stat.frame)[j] += ((int*)&t->stat.frame)[j];
        for( int j = 0; j < 3; j++ )
            h->stat.frame.i_ssd[j] += t->stat.frame.i_ssd[j];
        h->stat.frame.f_ssim += t->stat.frame.f_ssim;
        h->stat.frame.i_ssim_cnt += t->stat.frame.i_ssim_cnt;
    }

    return 0;
}

void x264_encoder_intra_refresh( x264_t *h )
{
    h = h->thread[h->i_thread_phase];
    h->b_queued_intra_refresh = 1;
}

int x264_encoder_invalidate_reference( x264_t *h, int64_t pts )
{
    if( h->param.i_bframe )
    {
        x264_log( h, X264_LOG_ERROR, "x264_encoder_invalidate_reference is not supported with B-frames enabled\n" );
        return -1;
    }
    if( h->param.b_intra_refresh )
    {
        x264_log( h, X264_LOG_ERROR, "x264_encoder_invalidate_reference is not supported with intra refresh enabled\n" );
        return -1;
    }
    h = h->thread[h->i_thread_phase];
    if( pts >= h->i_last_idr_pts )
    {
        for( int i = 0; h->frames.reference[i]; i++ )
            if( pts <= h->frames.reference[i]->i_pts )
                h->frames.reference[i]->b_corrupt = 1;
        if( pts <= h->fdec->i_pts )
            h->fdec->b_corrupt = 1;
    }
    return 0;
}

/****************************************************************************
 * x264_encoder_encode:
 *  XXX: i_poc   : is the poc of the current given picture
 *       i_frame : is the number of the frame being coded
 *  ex:  type frame poc
 *       I      0   2*0
 *       P      1   2*3
 *       B      2   2*1
 *       B      3   2*2
 *       P      4   2*6
 *       B      5   2*4
 *       B      6   2*5
 ****************************************************************************/
int     x264_encoder_encode( x264_t *h,
                             x264_nal_t **pp_nal, int *pi_nal,
                             x264_picture_t *pic_in,
                             x264_picture_t *pic_out )
{
    x264_t *thread_current, *thread_prev, *thread_oldest;
    int i_nal_type, i_nal_ref_idc, i_global_qp;
    int overhead = NALU_OVERHEAD;

    if( h->i_thread_frames > 1 )
    {
        thread_prev    = h->thread[ h->i_thread_phase ];
        h->i_thread_phase = (h->i_thread_phase + 1) % h->i_thread_frames;
        thread_current = h->thread[ h->i_thread_phase ];
        thread_oldest  = h->thread[ (h->i_thread_phase + 1) % h->i_thread_frames ];
        x264_thread_sync_context( thread_current, thread_prev );
        x264_thread_sync_ratecontrol( thread_current, thread_prev, thread_oldest );
        h = thread_current;
    }
    else
    {
        thread_current =
        thread_oldest  = h;
    }
#if HAVE_MMX
    if( h->param.cpu&X264_CPU_SSE_MISALIGN )
        x264_cpu_mask_misalign_sse();
#endif

    // ok to call this before encoding any frames, since the initial values of fdec have b_kept_as_ref=0
    if( x264_reference_update( h ) )
        return -1;
    h->fdec->i_lines_completed = -1;

    /* no data out */
    *pi_nal = 0;
    *pp_nal = NULL;

    /* ------------------- Setup new frame from picture -------------------- */
    if( pic_in != NULL )
    {
        /* 1: Copy the picture to a frame and move it to a buffer */
        x264_frame_t *fenc = x264_frame_pop_unused( h, 0 );
        if( !fenc )
            return -1;

        if( x264_frame_copy_picture( h, fenc, pic_in ) < 0 )
            return -1;

        if( h->param.i_width != 16 * h->mb.i_mb_width ||
            h->param.i_height != 16 * h->mb.i_mb_height )
            x264_frame_expand_border_mod16( h, fenc );

        fenc->i_frame = h->frames.i_input++;

        if( fenc->i_frame == 0 )
            h->frames.i_first_pts = fenc->i_pts;
        if( h->frames.i_bframe_delay && fenc->i_frame == h->frames.i_bframe_delay )
            h->frames.i_bframe_delay_time = fenc->i_pts - h->frames.i_first_pts;

        if( h->param.b_vfr_input && fenc->i_pts <= h->frames.i_largest_pts )
            x264_log( h, X264_LOG_WARNING, "non-strictly-monotonic PTS\n" );

        h->frames.i_second_largest_pts = h->frames.i_largest_pts;
        h->frames.i_largest_pts = fenc->i_pts;

        if( (fenc->i_pic_struct < PIC_STRUCT_AUTO) || (fenc->i_pic_struct > PIC_STRUCT_TRIPLE) )
            fenc->i_pic_struct = PIC_STRUCT_AUTO;

        if( fenc->i_pic_struct == PIC_STRUCT_AUTO )
        {
#if HAVE_INTERLACED
            int b_interlaced = fenc->param ? fenc->param->b_interlaced : h->param.b_interlaced;
#else
            int b_interlaced = 0;
#endif
            if( b_interlaced )
            {
                int b_tff = fenc->param ? fenc->param->b_tff : h->param.b_tff;
                fenc->i_pic_struct = b_tff ? PIC_STRUCT_TOP_BOTTOM : PIC_STRUCT_BOTTOM_TOP;
            }
            else
                fenc->i_pic_struct = PIC_STRUCT_PROGRESSIVE;
        }

        if( h->param.rc.b_mb_tree && h->param.rc.b_stat_read )
        {
            if( x264_macroblock_tree_read( h, fenc, pic_in->prop.quant_offsets ) )
                return -1;
        }
        else
            x264_stack_align( x264_adaptive_quant_frame, h, fenc, pic_in->prop.quant_offsets );

        if( pic_in->prop.quant_offsets_free )
            pic_in->prop.quant_offsets_free( pic_in->prop.quant_offsets );

        if( h->frames.b_have_lowres )
            x264_frame_init_lowres( h, fenc );

        /* 2: Place the frame into the queue for its slice type decision */
        x264_lookahead_put_frame( h, fenc );

        if( h->frames.i_input <= h->frames.i_delay + 1 - h->i_thread_frames )
        {
            /* Nothing yet to encode, waiting for filling of buffers */
            pic_out->i_type = X264_TYPE_AUTO;
            return 0;
        }
    }
    else
    {
        /* signal kills for lookahead thread */
        x264_pthread_mutex_lock( &h->lookahead->ifbuf.mutex );
        h->lookahead->b_exit_thread = 1;
        x264_pthread_cond_broadcast( &h->lookahead->ifbuf.cv_fill );
        x264_pthread_mutex_unlock( &h->lookahead->ifbuf.mutex );
    }

    h->i_frame++;
    /* 3: The picture is analyzed in the lookahead */
    if( !h->frames.current[0] )
        x264_lookahead_get_frames( h );

    if( !h->frames.current[0] && x264_lookahead_is_empty( h ) )
        return x264_encoder_frame_end( thread_oldest, thread_current, pp_nal, pi_nal, pic_out );

    /* ------------------- Get frame to be encoded ------------------------- */
    /* 4: get picture to encode */
    h->fenc = x264_frame_shift( h->frames.current );
    if( h->i_frame == h->i_thread_frames - 1 )
        h->i_reordered_pts_delay = h->fenc->i_reordered_pts;
    if( h->fenc->param )
    {
        x264_encoder_reconfig( h, h->fenc->param );
        if( h->fenc->param->param_free )
            h->fenc->param->param_free( h->fenc->param );
    }

    if( !IS_X264_TYPE_I( h->fenc->i_type ) )
    {
        int valid_refs_left = 0;
        for( int i = 0; h->frames.reference[i]; i++ )
            if( !h->frames.reference[i]->b_corrupt )
                valid_refs_left++;
        /* No valid reference frames left: force an IDR. */
        if( !valid_refs_left )
        {
            h->fenc->b_keyframe = 1;
            h->fenc->i_type = X264_TYPE_IDR;
        }
    }

    if( h->fenc->b_keyframe )
    {
        h->frames.i_last_keyframe = h->fenc->i_frame;
        if( h->fenc->i_type == X264_TYPE_IDR )
        {
            h->i_frame_num = 0;
            h->frames.i_last_idr = h->fenc->i_frame;
        }
    }
    h->sh.i_mmco_command_count =
    h->sh.i_mmco_remove_from_end = 0;
    h->b_ref_reorder[0] =
    h->b_ref_reorder[1] = 0;
    h->fdec->i_poc =
    h->fenc->i_poc = 2 * ( h->fenc->i_frame - X264_MAX( h->frames.i_last_idr, 0 ) );

    /* ------------------- Setup frame context ----------------------------- */
    /* 5: Init data dependent of frame type */
    if( h->fenc->i_type == X264_TYPE_IDR )
    {
        /* reset ref pictures */
        i_nal_type    = NAL_SLICE_IDR;
        i_nal_ref_idc = NAL_PRIORITY_HIGHEST;
        h->sh.i_type = SLICE_TYPE_I;
        x264_reference_reset( h );
        h->frames.i_poc_last_open_gop = -1;
    }
    else if( h->fenc->i_type == X264_TYPE_I )
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        h->sh.i_type = SLICE_TYPE_I;
        x264_reference_hierarchy_reset( h );
        if( h->param.b_open_gop )
            h->frames.i_poc_last_open_gop = h->fenc->b_keyframe ? h->fenc->i_poc : -1;
    }
    else if( h->fenc->i_type == X264_TYPE_P )
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        h->sh.i_type = SLICE_TYPE_P;
        x264_reference_hierarchy_reset( h );
        h->frames.i_poc_last_open_gop = -1;
    }
    else if( h->fenc->i_type == X264_TYPE_BREF )
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = h->param.i_bframe_pyramid == X264_B_PYRAMID_STRICT ? NAL_PRIORITY_LOW : NAL_PRIORITY_HIGH;
        h->sh.i_type = SLICE_TYPE_B;
        x264_reference_hierarchy_reset( h );
    }
    else    /* B frame */
    {
        i_nal_type    = NAL_SLICE;
        i_nal_ref_idc = NAL_PRIORITY_DISPOSABLE;
        h->sh.i_type = SLICE_TYPE_B;
    }

    h->fdec->i_type = h->fenc->i_type;
    h->fdec->i_frame = h->fenc->i_frame;
    h->fenc->b_kept_as_ref =
    h->fdec->b_kept_as_ref = i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE && h->param.i_keyint_max > 1;

    h->fdec->i_pts = h->fenc->i_pts;
    if( h->frames.i_bframe_delay )
    {
        int64_t *prev_reordered_pts = thread_current->frames.i_prev_reordered_pts;
        h->fdec->i_dts = h->i_frame > h->frames.i_bframe_delay
                       ? prev_reordered_pts[ (h->i_frame - h->frames.i_bframe_delay) % h->frames.i_bframe_delay ]
                       : h->fenc->i_reordered_pts - h->frames.i_bframe_delay_time;
        prev_reordered_pts[ h->i_frame % h->frames.i_bframe_delay ] = h->fenc->i_reordered_pts;
    }
    else
        h->fdec->i_dts = h->fenc->i_reordered_pts;
    if( h->fenc->i_type == X264_TYPE_IDR )
        h->i_last_idr_pts = h->fdec->i_pts;

    /* ------------------- Init                ----------------------------- */
    /* build ref list 0/1 */
    x264_reference_build_list( h, h->fdec->i_poc );

    /* ---------------------- Write the bitstream -------------------------- */
    /* Init bitstream context */
    if( h->param.b_sliced_threads )
    {
        for( int i = 0; i < h->param.i_threads; i++ )
        {
            bs_init( &h->thread[i]->out.bs, h->thread[i]->out.p_bitstream, h->thread[i]->out.i_bitstream );
            h->thread[i]->out.i_nal = 0;
        }
    }
    else
    {
        bs_init( &h->out.bs, h->out.p_bitstream, h->out.i_bitstream );
        h->out.i_nal = 0;
    }

    if( h->param.b_aud )
    {
        int pic_type;

        if( h->sh.i_type == SLICE_TYPE_I )
            pic_type = 0;
        else if( h->sh.i_type == SLICE_TYPE_P )
            pic_type = 1;
        else if( h->sh.i_type == SLICE_TYPE_B )
            pic_type = 2;
        else
            pic_type = 7;

        x264_nal_start( h, NAL_AUD, NAL_PRIORITY_DISPOSABLE );
        bs_write( &h->out.bs, 3, pic_type );
        bs_rbsp_trailing( &h->out.bs );
        if( x264_nal_end( h ) )
            return -1;
        overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD - (h->param.b_annexb && h->out.i_nal-1);
    }

    h->i_nal_type = i_nal_type;
    h->i_nal_ref_idc = i_nal_ref_idc;

    if( h->param.b_intra_refresh )
    {
        if( IS_X264_TYPE_I( h->fenc->i_type ) )
        {
            h->fdec->i_frames_since_pir = 0;
            h->b_queued_intra_refresh = 0;
            /* PIR is currently only supported with ref == 1, so any intra frame effectively refreshes
             * the whole frame and counts as an intra refresh. */
            h->fdec->f_pir_position = h->mb.i_mb_width;
        }
        else if( h->fenc->i_type == X264_TYPE_P )
        {
            int pocdiff = (h->fdec->i_poc - h->fref[0][0]->i_poc)/2;
            float increment = X264_MAX( ((float)h->mb.i_mb_width-1) / h->param.i_keyint_max, 1 );
            h->fdec->f_pir_position = h->fref[0][0]->f_pir_position;
            h->fdec->i_frames_since_pir = h->fref[0][0]->i_frames_since_pir + pocdiff;
            if( h->fdec->i_frames_since_pir >= h->param.i_keyint_max ||
                (h->b_queued_intra_refresh && h->fdec->f_pir_position + 0.5 >= h->mb.i_mb_width) )
            {
                h->fdec->f_pir_position = 0;
                h->fdec->i_frames_since_pir = 0;
                h->b_queued_intra_refresh = 0;
                h->fenc->b_keyframe = 1;
            }
            h->fdec->i_pir_start_col = h->fdec->f_pir_position+0.5;
            h->fdec->f_pir_position += increment * pocdiff;
            h->fdec->i_pir_end_col = h->fdec->f_pir_position+0.5;
            /* If our intra refresh has reached the right side of the frame, we're done. */
            if( h->fdec->i_pir_end_col >= h->mb.i_mb_width - 1 )
                h->fdec->f_pir_position = h->mb.i_mb_width;
        }
    }

    if( h->fenc->b_keyframe )
    {
        /* Write SPS and PPS */
        if( h->param.b_repeat_headers )
        {
            /* generate sequence parameters */
            x264_nal_start( h, NAL_SPS, NAL_PRIORITY_HIGHEST );
            x264_sps_write( &h->out.bs, h->sps );
            if( x264_nal_end( h ) )
                return -1;
            overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD;

            /* generate picture parameters */
            x264_nal_start( h, NAL_PPS, NAL_PRIORITY_HIGHEST );
            x264_pps_write( &h->out.bs, h->sps, h->pps );
            if( x264_nal_end( h ) )
                return -1;
            overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD;
        }

        /* when frame threading is used, buffering period sei is written in x264_encoder_frame_end */
        if( h->i_thread_frames == 1 && h->sps->vui.b_nal_hrd_parameters_present )
        {
            x264_hrd_fullness( h );
            x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
            x264_sei_buffering_period_write( h, &h->out.bs );
            if( x264_nal_end( h ) )
               return -1;
            overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD;
        }
    }

    /* write extra sei */
    for( int i = 0; i < h->fenc->extra_sei.num_payloads; i++ )
    {
        x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
        x264_sei_write( &h->out.bs, h->fenc->extra_sei.payloads[i].payload, h->fenc->extra_sei.payloads[i].payload_size,
                        h->fenc->extra_sei.payloads[i].payload_type );
        if( x264_nal_end( h ) )
            return -1;
        overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD - (h->param.b_annexb && h->out.i_nal-1);
        if( h->fenc->extra_sei.sei_free && h->fenc->extra_sei.payloads[i].payload )
            h->fenc->extra_sei.sei_free( h->fenc->extra_sei.payloads[i].payload );
    }

    if( h->fenc->extra_sei.sei_free && h->fenc->extra_sei.payloads )
        h->fenc->extra_sei.sei_free( h->fenc->extra_sei.payloads );

    if( h->fenc->b_keyframe )
    {
        if( h->param.b_repeat_headers && h->fenc->i_frame == 0 )
        {
            /* identify ourself */
            x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
            if( x264_sei_version_write( h, &h->out.bs ) )
                return -1;
            if( x264_nal_end( h ) )
                return -1;
            overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD - (h->param.b_annexb && h->out.i_nal-1);
        }

        if( h->fenc->i_type != X264_TYPE_IDR )
        {
            int time_to_recovery = h->param.b_open_gop ? 0 : X264_MIN( h->mb.i_mb_width - 1, h->param.i_keyint_max ) + h->param.i_bframe - 1;
            x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
            x264_sei_recovery_point_write( h, &h->out.bs, time_to_recovery );
            if( x264_nal_end( h ) )
                return -1;
            overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD - (h->param.b_annexb && h->out.i_nal-1);
        }

        if ( h->param.i_frame_packing >= 0 )
        {
            x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
            x264_sei_frame_packing_write( h, &h->out.bs );
            if( x264_nal_end( h ) )
                return -1;
            overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD - (h->param.b_annexb && h->out.i_nal-1);
        }
    }

    /* generate sei pic timing */
    if( h->sps->vui.b_pic_struct_present || h->sps->vui.b_nal_hrd_parameters_present )
    {
        x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
        x264_sei_pic_timing_write( h, &h->out.bs );
        if( x264_nal_end( h ) )
            return -1;
        overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD - (h->param.b_annexb && h->out.i_nal-1);
    }

    /* As required by Blu-ray. */
    if( !IS_X264_TYPE_B( h->fenc->i_type ) && h->b_sh_backup )
    {
        h->b_sh_backup = 0;
        x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
        x264_sei_dec_ref_pic_marking_write( h, &h->out.bs );
        if( x264_nal_end( h ) )
            return -1;
        overhead += h->out.nal[h->out.i_nal-1].i_payload + NALU_OVERHEAD - (h->param.b_annexb && h->out.i_nal-1);
    }

    if( h->fenc->b_keyframe && h->param.b_intra_refresh )
        h->i_cpb_delay_pir_offset = h->fenc->i_cpb_delay;

    /* Init the rate control */
    /* FIXME: Include slice header bit cost. */
    x264_ratecontrol_start( h, h->fenc->i_qpplus1, overhead*8 );
    i_global_qp = x264_ratecontrol_qp( h );

    pic_out->i_qpplus1 =
    h->fdec->i_qpplus1 = i_global_qp + 1;

    if( h->param.rc.b_stat_read && h->sh.i_type != SLICE_TYPE_I )
    {
        x264_reference_build_list_optimal( h );
        x264_reference_check_reorder( h );
    }

    if( h->i_ref[0] )
        h->fdec->i_poc_l0ref0 = h->fref[0][0]->i_poc;

    /* ------------------------ Create slice header  ----------------------- */
    x264_slice_init( h, i_nal_type, i_global_qp );

    /*------------------------- Weights -------------------------------------*/
    if( h->sh.i_type == SLICE_TYPE_B )
        x264_macroblock_bipred_init( h );

    x264_weighted_pred_init( h );

    if( i_nal_ref_idc != NAL_PRIORITY_DISPOSABLE )
        h->i_frame_num++;

    /* Write frame */
    h->i_threadslice_start = 0;
    h->i_threadslice_end = h->mb.i_mb_height;
    if( h->i_thread_frames > 1 )
    {
        x264_threadpool_run( h->threadpool, (void*)x264_slices_write, h );
        h->b_thread_active = 1;
    }
    else if( h->param.b_sliced_threads )
    {
        if( x264_threaded_slices_write( h ) )
            return -1;
    }
    else
        if( (intptr_t)x264_slices_write( h ) )
            return -1;

    return x264_encoder_frame_end( thread_oldest, thread_current, pp_nal, pi_nal, pic_out );
}

static int x264_encoder_frame_end( x264_t *h, x264_t *thread_current,
                                   x264_nal_t **pp_nal, int *pi_nal,
                                   x264_picture_t *pic_out )
{
    char psz_message[80];

    if( h->b_thread_active )
    {
        h->b_thread_active = 0;
        if( (intptr_t)x264_threadpool_wait( h->threadpool, h ) )
            return -1;
    }
    if( !h->out.i_nal )
    {
        pic_out->i_type = X264_TYPE_AUTO;
        return 0;
    }

    x264_emms();
    /* generate buffering period sei and insert it into place */
    if( h->i_thread_frames > 1 && h->fenc->b_keyframe && h->sps->vui.b_nal_hrd_parameters_present )
    {
        x264_hrd_fullness( h );
        x264_nal_start( h, NAL_SEI, NAL_PRIORITY_DISPOSABLE );
        x264_sei_buffering_period_write( h, &h->out.bs );
        if( x264_nal_end( h ) )
           return -1;
        /* buffering period sei must follow AUD, SPS and PPS and precede all other SEIs */
        int idx = 0;
        while( h->out.nal[idx].i_type == NAL_AUD ||
               h->out.nal[idx].i_type == NAL_SPS ||
               h->out.nal[idx].i_type == NAL_PPS )
            idx++;
        x264_nal_t nal_tmp = h->out.nal[h->out.i_nal-1];
        memmove( &h->out.nal[idx+1], &h->out.nal[idx], (h->out.i_nal-idx-1)*sizeof(x264_nal_t) );
        h->out.nal[idx] = nal_tmp;
    }

    int frame_size = x264_encoder_encapsulate_nals( h, 0 );
    if( frame_size < 0 )
        return -1;

    /* Set output picture properties */
    pic_out->i_type = h->fenc->i_type;

    pic_out->b_keyframe = h->fenc->b_keyframe;
    pic_out->i_pic_struct = h->fenc->i_pic_struct;

    pic_out->i_pts = h->fdec->i_pts;
    pic_out->i_dts = h->fdec->i_dts;

    if( pic_out->i_pts < pic_out->i_dts )
        x264_log( h, X264_LOG_WARNING, "invalid DTS: PTS is less than DTS\n" );

    pic_out->img.i_csp = h->fdec->i_csp;
#if HIGH_BIT_DEPTH
    pic_out->img.i_csp |= X264_CSP_HIGH_DEPTH;
#endif
    pic_out->img.i_plane = h->fdec->i_plane;
    for( int i = 0; i < pic_out->img.i_plane; i++ )
    {
        pic_out->img.i_stride[i] = h->fdec->i_stride[i] * sizeof(pixel);
        pic_out->img.plane[i] = (uint8_t*)h->fdec->plane[i];
    }

    x264_frame_push_unused( thread_current, h->fenc );

    /* ---------------------- Update encoder state ------------------------- */

    /* update rc */
    int filler = 0;
    if( x264_ratecontrol_end( h, frame_size * 8, &filler ) < 0 )
        return -1;

    pic_out->hrd_timing = h->fenc->hrd_timing;

    while( filler > 0 )
    {
        int f, overhead;
        overhead = (FILLER_OVERHEAD - h->param.b_annexb);
        if( h->param.i_slice_max_size && filler > h->param.i_slice_max_size )
        {
            int next_size = filler - h->param.i_slice_max_size;
            int overflow = X264_MAX( overhead - next_size, 0 );
            f = h->param.i_slice_max_size - overhead - overflow;
        }
        else
            f = X264_MAX( 0, filler - overhead );

        x264_nal_start( h, NAL_FILLER, NAL_PRIORITY_DISPOSABLE );
        x264_filler_write( h, &h->out.bs, f );
        if( x264_nal_end( h ) )
            return -1;
        int total_size = x264_encoder_encapsulate_nals( h, h->out.i_nal-1 );
        if( total_size < 0 )
            return -1;
        frame_size += total_size;
        filler -= total_size;
    }

    /* End bitstream, set output  */
    *pi_nal = h->out.i_nal;
    *pp_nal = h->out.nal;

    h->out.i_nal = 0;

    x264_noise_reduction_update( h );

    /* ---------------------- Compute/Print statistics --------------------- */
    x264_thread_sync_stat( h, h->thread[0] );

    /* Slice stat */
    h->stat.i_frame_count[h->sh.i_type]++;
    h->stat.i_frame_size[h->sh.i_type] += frame_size;
    h->stat.f_frame_qp[h->sh.i_type] += h->fdec->f_qp_avg_aq;

    for( int i = 0; i < X264_MBTYPE_MAX; i++ )
        h->stat.i_mb_count[h->sh.i_type][i] += h->stat.frame.i_mb_count[i];
    for( int i = 0; i < X264_PARTTYPE_MAX; i++ )
        h->stat.i_mb_partition[h->sh.i_type][i] += h->stat.frame.i_mb_partition[i];
    for( int i = 0; i < 2; i++ )
        h->stat.i_mb_count_8x8dct[i] += h->stat.frame.i_mb_count_8x8dct[i];
    for( int i = 0; i < 6; i++ )
        h->stat.i_mb_cbp[i] += h->stat.frame.i_mb_cbp[i];
    for( int i = 0; i < 4; i++ )
        for( int j = 0; j < 13; j++ )
            h->stat.i_mb_pred_mode[i][j] += h->stat.frame.i_mb_pred_mode[i][j];
    if( h->sh.i_type != SLICE_TYPE_I )
        for( int i_list = 0; i_list < 2; i_list++ )
            for( int i = 0; i < X264_REF_MAX*2; i++ )
                h->stat.i_mb_count_ref[h->sh.i_type][i_list][i] += h->stat.frame.i_mb_count_ref[i_list][i];
    for( int i = 0; i < 3; i++ )
        h->stat.i_mb_field[i] += h->stat.frame.i_mb_field[i];
    if( h->sh.i_type == SLICE_TYPE_P && h->param.analyse.i_weighted_pred >= X264_WEIGHTP_SIMPLE )
    {
        h->stat.i_wpred[0] += !!h->sh.weight[0][0].weightfn;
        h->stat.i_wpred[1] += !!h->sh.weight[0][1].weightfn || !!h->sh.weight[0][2].weightfn;
    }
    if( h->sh.i_type == SLICE_TYPE_B )
    {
        h->stat.i_direct_frames[ h->sh.b_direct_spatial_mv_pred ] ++;
        if( h->mb.b_direct_auto_write )
        {
            //FIXME somewhat arbitrary time constants
            if( h->stat.i_direct_score[0] + h->stat.i_direct_score[1] > h->mb.i_mb_count )
                for( int i = 0; i < 2; i++ )
                    h->stat.i_direct_score[i] = h->stat.i_direct_score[i] * 9/10;
            for( int i = 0; i < 2; i++ )
                h->stat.i_direct_score[i] += h->stat.frame.i_direct_score[i];
        }
    }
    else
        h->stat.i_consecutive_bframes[h->fenc->i_bframes]++;

    psz_message[0] = '\0';
    double dur = h->fenc->f_duration;
    h->stat.f_frame_duration[h->sh.i_type] += dur;
    if( h->param.analyse.b_psnr )
    {
        int64_t ssd[3] =
        {
            h->stat.frame.i_ssd[0],
            h->stat.frame.i_ssd[1],
            h->stat.frame.i_ssd[2],
        };
        int luma_size = h->param.i_width * h->param.i_height;
        int chroma_size = h->param.i_width * h->param.i_height >> (!CHROMA444 * 2);
        double psnr_y = x264_psnr( ssd[0], luma_size );
        double psnr_u = x264_psnr( ssd[1], chroma_size );
        double psnr_v = x264_psnr( ssd[2], chroma_size );

        h->stat.f_ssd_global[h->sh.i_type]   += dur * (ssd[0] + ssd[1] + ssd[2]);
        h->stat.f_psnr_average[h->sh.i_type] += dur * x264_psnr( ssd[0] + ssd[1] + ssd[2], luma_size + chroma_size*2 );
        h->stat.f_psnr_mean_y[h->sh.i_type]  += dur * psnr_y;
        h->stat.f_psnr_mean_u[h->sh.i_type]  += dur * psnr_u;
        h->stat.f_psnr_mean_v[h->sh.i_type]  += dur * psnr_v;

        snprintf( psz_message, 80, " PSNR Y:%5.2f U:%5.2f V:%5.2f", psnr_y, psnr_u, psnr_v );
    }

    if( h->param.analyse.b_ssim )
    {
        double ssim_y = h->stat.frame.f_ssim
                      / h->stat.frame.i_ssim_cnt;
        h->stat.f_ssim_mean_y[h->sh.i_type] += ssim_y * dur;
        snprintf( psz_message + strlen(psz_message), 80 - strlen(psz_message),
                  " SSIM Y:%.5f", ssim_y );
    }
    psz_message[79] = '\0';

    x264_log( h, X264_LOG_DEBUG,
                  "frame=%4d QP=%.2f NAL=%d Slice:%c Poc:%-3d I:%-4d P:%-4d SKIP:%-4d size=%d bytes%s\n",
              h->i_frame,
              h->fdec->f_qp_avg_aq,
              h->i_nal_ref_idc,
              h->sh.i_type == SLICE_TYPE_I ? 'I' : (h->sh.i_type == SLICE_TYPE_P ? 'P' : 'B' ),
              h->fdec->i_poc,
              h->stat.frame.i_mb_count_i,
              h->stat.frame.i_mb_count_p,
              h->stat.frame.i_mb_count_skip,
              frame_size,
              psz_message );

    // keep stats all in one place
    x264_thread_sync_stat( h->thread[0], h );
    // for the use of the next frame
    x264_thread_sync_stat( thread_current, h );

#ifdef DEBUG_MB_TYPE
{
    static const char mb_chars[] = { 'i', 'i', 'I', 'C', 'P', '8', 'S',
        'D', '<', 'X', 'B', 'X', '>', 'B', 'B', 'B', 'B', '8', 'S' };
    for( int mb_xy = 0; mb_xy < h->mb.i_mb_width * h->mb.i_mb_height; mb_xy++ )
    {
        if( h->mb.type[mb_xy] < X264_MBTYPE_MAX && h->mb.type[mb_xy] >= 0 )
            fprintf( stderr, "%c ", mb_chars[ h->mb.type[mb_xy] ] );
        else
            fprintf( stderr, "? " );

        if( (mb_xy+1) % h->mb.i_mb_width == 0 )
            fprintf( stderr, "\n" );
    }
}
#endif

    /* Remove duplicates, must be done near the end as breaks h->fref0 array
     * by freeing some of its pointers. */
    for( int i = 0; i < h->i_ref[0]; i++ )
        if( h->fref[0][i] && h->fref[0][i]->b_duplicate )
        {
            x264_frame_push_blank_unused( h, h->fref[0][i] );
            h->fref[0][i] = 0;
        }

    if( h->param.psz_dump_yuv )
        x264_frame_dump( h );
    x264_emms();

    return frame_size;
}

static void x264_print_intra( int64_t *i_mb_count, double i_count, int b_print_pcm, char *intra )
{
    intra += sprintf( intra, "I16..4%s: %4.1f%% %4.1f%% %4.1f%%",
        b_print_pcm ? "..PCM" : "",
        i_mb_count[I_16x16]/ i_count,
        i_mb_count[I_8x8]  / i_count,
        i_mb_count[I_4x4]  / i_count );
    if( b_print_pcm )
        sprintf( intra, " %4.1f%%", i_mb_count[I_PCM]  / i_count );
}

/****************************************************************************
 * x264_encoder_close:
 ****************************************************************************/
void    x264_encoder_close  ( x264_t *h )
{
    int luma_size = h->param.i_width * h->param.i_height;
    int chroma_size = h->param.i_width * h->param.i_height >> (!CHROMA444 * 2);
    int64_t i_yuv_size = luma_size + chroma_size * 2;
    int64_t i_mb_count_size[2][7] = {{0}};
    char buf[200];
    int b_print_pcm = h->stat.i_mb_count[SLICE_TYPE_I][I_PCM]
                   || h->stat.i_mb_count[SLICE_TYPE_P][I_PCM]
                   || h->stat.i_mb_count[SLICE_TYPE_B][I_PCM];

    x264_lookahead_delete( h );

    if( h->param.i_threads > 1 )
        x264_threadpool_delete( h->threadpool );
    if( h->i_thread_frames > 1 )
    {
        for( int i = 0; i < h->i_thread_frames; i++ )
            if( h->thread[i]->b_thread_active )
            {
                assert( h->thread[i]->fenc->i_reference_count == 1 );
                x264_frame_delete( h->thread[i]->fenc );
            }

        x264_t *thread_prev = h->thread[h->i_thread_phase];
        x264_thread_sync_ratecontrol( h, thread_prev, h );
        x264_thread_sync_ratecontrol( thread_prev, thread_prev, h );
        h->i_frame = thread_prev->i_frame + 1 - h->i_thread_frames;
    }
    h->i_frame++;

    /* Slices used and PSNR */
    for( int i = 0; i < 3; i++ )
    {
        static const uint8_t slice_order[] = { SLICE_TYPE_I, SLICE_TYPE_P, SLICE_TYPE_B };
        int i_slice = slice_order[i];

        if( h->stat.i_frame_count[i_slice] > 0 )
        {
            int i_count = h->stat.i_frame_count[i_slice];
            double dur =  h->stat.f_frame_duration[i_slice];
            if( h->param.analyse.b_psnr )
            {
                x264_log( h, X264_LOG_INFO,
                          "frame %c:%-5d Avg QP:%5.2f  size:%6.0f  PSNR Mean Y:%5.2f U:%5.2f V:%5.2f Avg:%5.2f Global:%5.2f\n",
                          slice_type_to_char[i_slice],
                          i_count,
                          h->stat.f_frame_qp[i_slice] / i_count,
                          (double)h->stat.i_frame_size[i_slice] / i_count,
                          h->stat.f_psnr_mean_y[i_slice] / dur, h->stat.f_psnr_mean_u[i_slice] / dur, h->stat.f_psnr_mean_v[i_slice] / dur,
                          h->stat.f_psnr_average[i_slice] / dur,
                          x264_psnr( h->stat.f_ssd_global[i_slice], dur * i_yuv_size ) );
            }
            else
            {
                x264_log( h, X264_LOG_INFO,
                          "frame %c:%-5d Avg QP:%5.2f  size:%6.0f\n",
                          slice_type_to_char[i_slice],
                          i_count,
                          h->stat.f_frame_qp[i_slice] / i_count,
                          (double)h->stat.i_frame_size[i_slice] / i_count );
            }
        }
    }
    if( h->param.i_bframe && h->stat.i_frame_count[SLICE_TYPE_B] )
    {
        char *p = buf;
        int den = 0;
        // weight by number of frames (including the I/P-frames) that are in a sequence of N B-frames
        for( int i = 0; i <= h->param.i_bframe; i++ )
            den += (i+1) * h->stat.i_consecutive_bframes[i];
        for( int i = 0; i <= h->param.i_bframe; i++ )
            p += sprintf( p, " %4.1f%%", 100. * (i+1) * h->stat.i_consecutive_bframes[i] / den );
        x264_log( h, X264_LOG_INFO, "consecutive B-frames:%s\n", buf );
    }

    for( int i_type = 0; i_type < 2; i_type++ )
        for( int i = 0; i < X264_PARTTYPE_MAX; i++ )
        {
            if( i == D_DIRECT_8x8 ) continue; /* direct is counted as its own type */
            i_mb_count_size[i_type][x264_mb_partition_pixel_table[i]] += h->stat.i_mb_partition[i_type][i];
        }

    /* MB types used */
    if( h->stat.i_frame_count[SLICE_TYPE_I] > 0 )
    {
        int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_I];
        double i_count = h->stat.i_frame_count[SLICE_TYPE_I] * h->mb.i_mb_count / 100.0;
        x264_print_intra( i_mb_count, i_count, b_print_pcm, buf );
        x264_log( h, X264_LOG_INFO, "mb I  %s\n", buf );
    }
    if( h->stat.i_frame_count[SLICE_TYPE_P] > 0 )
    {
        int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_P];
        double i_count = h->stat.i_frame_count[SLICE_TYPE_P] * h->mb.i_mb_count / 100.0;
        int64_t *i_mb_size = i_mb_count_size[SLICE_TYPE_P];
        x264_print_intra( i_mb_count, i_count, b_print_pcm, buf );
        x264_log( h, X264_LOG_INFO,
                  "mb P  %s  P16..4: %4.1f%% %4.1f%% %4.1f%% %4.1f%% %4.1f%%    skip:%4.1f%%\n",
                  buf,
                  i_mb_size[PIXEL_16x16] / (i_count*4),
                  (i_mb_size[PIXEL_16x8] + i_mb_size[PIXEL_8x16]) / (i_count*4),
                  i_mb_size[PIXEL_8x8] / (i_count*4),
                  (i_mb_size[PIXEL_8x4] + i_mb_size[PIXEL_4x8]) / (i_count*4),
                  i_mb_size[PIXEL_4x4] / (i_count*4),
                  i_mb_count[P_SKIP] / i_count );
    }
    if( h->stat.i_frame_count[SLICE_TYPE_B] > 0 )
    {
        int64_t *i_mb_count = h->stat.i_mb_count[SLICE_TYPE_B];
        double i_count = h->stat.i_frame_count[SLICE_TYPE_B] * h->mb.i_mb_count / 100.0;
        double i_mb_list_count;
        int64_t *i_mb_size = i_mb_count_size[SLICE_TYPE_B];
        int64_t list_count[3] = {0}; /* 0 == L0, 1 == L1, 2 == BI */
        x264_print_intra( i_mb_count, i_count, b_print_pcm, buf );
        for( int i = 0; i < X264_PARTTYPE_MAX; i++ )
            for( int j = 0; j < 2; j++ )
            {
                int l0 = x264_mb_type_list_table[i][0][j];
                int l1 = x264_mb_type_list_table[i][1][j];
                if( l0 || l1 )
                    list_count[l1+l0*l1] += h->stat.i_mb_count[SLICE_TYPE_B][i] * 2;
            }
        list_count[0] += h->stat.i_mb_partition[SLICE_TYPE_B][D_L0_8x8];
        list_count[1] += h->stat.i_mb_partition[SLICE_TYPE_B][D_L1_8x8];
        list_count[2] += h->stat.i_mb_partition[SLICE_TYPE_B][D_BI_8x8];
        i_mb_count[B_DIRECT] += (h->stat.i_mb_partition[SLICE_TYPE_B][D_DIRECT_8x8]+2)/4;
        i_mb_list_count = (list_count[0] + list_count[1] + list_count[2]) / 100.0;
        sprintf( buf + strlen(buf), "  B16..8: %4.1f%% %4.1f%% %4.1f%%  direct:%4.1f%%  skip:%4.1f%%",
                 i_mb_size[PIXEL_16x16] / (i_count*4),
                 (i_mb_size[PIXEL_16x8] + i_mb_size[PIXEL_8x16]) / (i_count*4),
                 i_mb_size[PIXEL_8x8] / (i_count*4),
                 i_mb_count[B_DIRECT] / i_count,
                 i_mb_count[B_SKIP]   / i_count );
        if( i_mb_list_count != 0 )
            sprintf( buf + strlen(buf), "  L0:%4.1f%% L1:%4.1f%% BI:%4.1f%%",
                     list_count[0] / i_mb_list_count,
                     list_count[1] / i_mb_list_count,
                     list_count[2] / i_mb_list_count );
        x264_log( h, X264_LOG_INFO, "mb B  %s\n", buf );
    }

    x264_ratecontrol_summary( h );

    if( h->stat.i_frame_count[SLICE_TYPE_I] + h->stat.i_frame_count[SLICE_TYPE_P] + h->stat.i_frame_count[SLICE_TYPE_B] > 0 )
    {
#define SUM3(p) (p[SLICE_TYPE_I] + p[SLICE_TYPE_P] + p[SLICE_TYPE_B])
#define SUM3b(p,o) (p[SLICE_TYPE_I][o] + p[SLICE_TYPE_P][o] + p[SLICE_TYPE_B][o])
        int64_t i_i8x8 = SUM3b( h->stat.i_mb_count, I_8x8 );
        int64_t i_intra = i_i8x8 + SUM3b( h->stat.i_mb_count, I_4x4 )
                                 + SUM3b( h->stat.i_mb_count, I_16x16 );
        int64_t i_all_intra = i_intra + SUM3b( h->stat.i_mb_count, I_PCM);
        int64_t i_skip = SUM3b( h->stat.i_mb_count, P_SKIP )
                       + SUM3b( h->stat.i_mb_count, B_SKIP );
        const int i_count = h->stat.i_frame_count[SLICE_TYPE_I] +
                            h->stat.i_frame_count[SLICE_TYPE_P] +
                            h->stat.i_frame_count[SLICE_TYPE_B];
        int64_t i_mb_count = (int64_t)i_count * h->mb.i_mb_count;
        int64_t i_inter = i_mb_count - i_skip - i_intra;
        const double duration = h->stat.f_frame_duration[SLICE_TYPE_I] +
                                h->stat.f_frame_duration[SLICE_TYPE_P] +
                                h->stat.f_frame_duration[SLICE_TYPE_B];
        float f_bitrate = SUM3(h->stat.i_frame_size) / duration / 125;

        if( PARAM_INTERLACED )
        {
            char *fieldstats = buf;
            fieldstats[0] = 0;
            if( i_inter )
                fieldstats += sprintf( fieldstats, " inter:%.1f%%", h->stat.i_mb_field[1] * 100.0 / i_inter );
            if( i_skip )
                fieldstats += sprintf( fieldstats, " skip:%.1f%%", h->stat.i_mb_field[2] * 100.0 / i_skip );
            x264_log( h, X264_LOG_INFO, "field mbs: intra: %.1f%%%s\n",
                      h->stat.i_mb_field[0] * 100.0 / i_intra, buf );
        }

        if( h->pps->b_transform_8x8_mode )
        {
            buf[0] = 0;
            if( h->stat.i_mb_count_8x8dct[0] )
                sprintf( buf, " inter:%.1f%%", 100. * h->stat.i_mb_count_8x8dct[1] / h->stat.i_mb_count_8x8dct[0] );
            x264_log( h, X264_LOG_INFO, "8x8 transform intra:%.1f%%%s\n", 100. * i_i8x8 / i_intra, buf );
        }

        if( (h->param.analyse.i_direct_mv_pred == X264_DIRECT_PRED_AUTO ||
            (h->stat.i_direct_frames[0] && h->stat.i_direct_frames[1]))
            && h->stat.i_frame_count[SLICE_TYPE_B] )
        {
            x264_log( h, X264_LOG_INFO, "direct mvs  spatial:%.1f%% temporal:%.1f%%\n",
                      h->stat.i_direct_frames[1] * 100. / h->stat.i_frame_count[SLICE_TYPE_B],
                      h->stat.i_direct_frames[0] * 100. / h->stat.i_frame_count[SLICE_TYPE_B] );
        }

        buf[0] = 0;
        int csize = CHROMA444 ? 4 : 1;
        if( i_mb_count != i_all_intra )
            sprintf( buf, " inter: %.1f%% %.1f%% %.1f%%",
                     h->stat.i_mb_cbp[1] * 100.0 / ((i_mb_count - i_all_intra)*4),
                     h->stat.i_mb_cbp[3] * 100.0 / ((i_mb_count - i_all_intra)*csize),
                     h->stat.i_mb_cbp[5] * 100.0 / ((i_mb_count - i_all_intra)*csize) );
        x264_log( h, X264_LOG_INFO, "coded y,%s,%s intra: %.1f%% %.1f%% %.1f%%%s\n",
                  CHROMA444?"u":"uvDC", CHROMA444?"v":"uvAC",
                  h->stat.i_mb_cbp[0] * 100.0 / (i_all_intra*4),
                  h->stat.i_mb_cbp[2] * 100.0 / (i_all_intra*csize),
                  h->stat.i_mb_cbp[4] * 100.0 / (i_all_intra*csize), buf );

        int64_t fixed_pred_modes[4][9] = {{0}};
        int64_t sum_pred_modes[4] = {0};
        for( int i = 0; i <= I_PRED_16x16_DC_128; i++ )
        {
            fixed_pred_modes[0][x264_mb_pred_mode16x16_fix[i]] += h->stat.i_mb_pred_mode[0][i];
            sum_pred_modes[0] += h->stat.i_mb_pred_mode[0][i];
        }
        if( sum_pred_modes[0] )
            x264_log( h, X264_LOG_INFO, "i16 v,h,dc,p: %2.0f%% %2.0f%% %2.0f%% %2.0f%%\n",
                      fixed_pred_modes[0][0] * 100.0 / sum_pred_modes[0],
                      fixed_pred_modes[0][1] * 100.0 / sum_pred_modes[0],
                      fixed_pred_modes[0][2] * 100.0 / sum_pred_modes[0],
                      fixed_pred_modes[0][3] * 100.0 / sum_pred_modes[0] );
        for( int i = 1; i <= 2; i++ )
        {
            for( int j = 0; j <= I_PRED_8x8_DC_128; j++ )
            {
                fixed_pred_modes[i][x264_mb_pred_mode4x4_fix(j)] += h->stat.i_mb_pred_mode[i][j];
                sum_pred_modes[i] += h->stat.i_mb_pred_mode[i][j];
            }
            if( sum_pred_modes[i] )
                x264_log( h, X264_LOG_INFO, "i%d v,h,dc,ddl,ddr,vr,hd,vl,hu: %2.0f%% %2.0f%% %2.0f%% %2.0f%% %2.0f%% %2.0f%% %2.0f%% %2.0f%% %2.0f%%\n", (3-i)*4,
                          fixed_pred_modes[i][0] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][1] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][2] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][3] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][4] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][5] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][6] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][7] * 100.0 / sum_pred_modes[i],
                          fixed_pred_modes[i][8] * 100.0 / sum_pred_modes[i] );
        }
        for( int i = 0; i <= I_PRED_CHROMA_DC_128; i++ )
        {
            fixed_pred_modes[3][x264_mb_pred_mode8x8c_fix[i]] += h->stat.i_mb_pred_mode[3][i];
            sum_pred_modes[3] += h->stat.i_mb_pred_mode[3][i];
        }
        if( sum_pred_modes[3] && !CHROMA444 )
            x264_log( h, X264_LOG_INFO, "i8c dc,h,v,p: %2.0f%% %2.0f%% %2.0f%% %2.0f%%\n",
                      fixed_pred_modes[3][0] * 100.0 / sum_pred_modes[3],
                      fixed_pred_modes[3][1] * 100.0 / sum_pred_modes[3],
                      fixed_pred_modes[3][2] * 100.0 / sum_pred_modes[3],
                      fixed_pred_modes[3][3] * 100.0 / sum_pred_modes[3] );

        if( h->param.analyse.i_weighted_pred >= X264_WEIGHTP_SIMPLE && h->stat.i_frame_count[SLICE_TYPE_P] > 0 )
            x264_log( h, X264_LOG_INFO, "Weighted P-Frames: Y:%.1f%% UV:%.1f%%\n",
                      h->stat.i_wpred[0] * 100.0 / h->stat.i_frame_count[SLICE_TYPE_P],
                      h->stat.i_wpred[1] * 100.0 / h->stat.i_frame_count[SLICE_TYPE_P] );

        for( int i_list = 0; i_list < 2; i_list++ )
            for( int i_slice = 0; i_slice < 2; i_slice++ )
            {
                char *p = buf;
                int64_t i_den = 0;
                int i_max = 0;
                for( int i = 0; i < X264_REF_MAX*2; i++ )
                    if( h->stat.i_mb_count_ref[i_slice][i_list][i] )
                    {
                        i_den += h->stat.i_mb_count_ref[i_slice][i_list][i];
                        i_max = i;
                    }
                if( i_max == 0 )
                    continue;
                for( int i = 0; i <= i_max; i++ )
                    p += sprintf( p, " %4.1f%%", 100. * h->stat.i_mb_count_ref[i_slice][i_list][i] / i_den );
                x264_log( h, X264_LOG_INFO, "ref %c L%d:%s\n", "PB"[i_slice], i_list, buf );
            }

        if( h->param.analyse.b_ssim )
        {
            float ssim = SUM3( h->stat.f_ssim_mean_y ) / duration;
            x264_log( h, X264_LOG_INFO, "SSIM Mean Y:%.7f (%6.3fdb)\n", ssim, x264_ssim( ssim ) );
        }
        if( h->param.analyse.b_psnr )
        {
            x264_log( h, X264_LOG_INFO,
                      "PSNR Mean Y:%6.3f U:%6.3f V:%6.3f Avg:%6.3f Global:%6.3f kb/s:%.2f\n",
                      SUM3( h->stat.f_psnr_mean_y ) / duration,
                      SUM3( h->stat.f_psnr_mean_u ) / duration,
                      SUM3( h->stat.f_psnr_mean_v ) / duration,
                      SUM3( h->stat.f_psnr_average ) / duration,
                      x264_psnr( SUM3( h->stat.f_ssd_global ), duration * i_yuv_size ),
                      f_bitrate );
        }
        else
            x264_log( h, X264_LOG_INFO, "kb/s:%.2f\n", f_bitrate );
    }

    /* rc */
    x264_ratecontrol_delete( h );

    /* param */
    if( h->param.rc.psz_stat_out )
        free( h->param.rc.psz_stat_out );
    if( h->param.rc.psz_stat_in )
        free( h->param.rc.psz_stat_in );

    x264_cqm_delete( h );
    x264_free( h->nal_buffer );
    x264_analyse_free_costs( h );

    if( h->i_thread_frames > 1)
        h = h->thread[h->i_thread_phase];

    /* frames */
    x264_frame_delete_list( h->frames.unused[0] );
    x264_frame_delete_list( h->frames.unused[1] );
    x264_frame_delete_list( h->frames.current );
    x264_frame_delete_list( h->frames.blank_unused );

    h = h->thread[0];

    for( int i = 0; i < h->i_thread_frames; i++ )
        if( h->thread[i]->b_thread_active )
            for( int j = 0; j < h->thread[i]->i_ref[0]; j++ )
                if( h->thread[i]->fref[0][j] && h->thread[i]->fref[0][j]->b_duplicate )
                    x264_frame_delete( h->thread[i]->fref[0][j] );

    for( int i = h->param.i_threads - 1; i >= 0; i-- )
    {
        x264_frame_t **frame;

        if( !h->param.b_sliced_threads || i == 0 )
        {
            for( frame = h->thread[i]->frames.reference; *frame; frame++ )
            {
                assert( (*frame)->i_reference_count > 0 );
                (*frame)->i_reference_count--;
                if( (*frame)->i_reference_count == 0 )
                    x264_frame_delete( *frame );
            }
            frame = &h->thread[i]->fdec;
            if( *frame )
            {
                assert( (*frame)->i_reference_count > 0 );
                (*frame)->i_reference_count--;
                if( (*frame)->i_reference_count == 0 )
                    x264_frame_delete( *frame );
            }
            x264_macroblock_cache_free( h->thread[i] );
        }
        x264_macroblock_thread_free( h->thread[i], 0 );
        x264_free( h->thread[i]->out.p_bitstream );
        x264_free( h->thread[i]->out.nal);
        x264_free( h->thread[i] );
    }
}

int x264_encoder_delayed_frames( x264_t *h )
{
    int delayed_frames = 0;
    if( h->i_thread_frames > 1 )
    {
        for( int i = 0; i < h->i_thread_frames; i++ )
            delayed_frames += h->thread[i]->b_thread_active;
        h = h->thread[h->i_thread_phase];
    }
    for( int i = 0; h->frames.current[i]; i++ )
        delayed_frames++;
    x264_pthread_mutex_lock( &h->lookahead->ofbuf.mutex );
    x264_pthread_mutex_lock( &h->lookahead->ifbuf.mutex );
    x264_pthread_mutex_lock( &h->lookahead->next.mutex );
    delayed_frames += h->lookahead->ifbuf.i_size + h->lookahead->next.i_size + h->lookahead->ofbuf.i_size;
    x264_pthread_mutex_unlock( &h->lookahead->next.mutex );
    x264_pthread_mutex_unlock( &h->lookahead->ifbuf.mutex );
    x264_pthread_mutex_unlock( &h->lookahead->ofbuf.mutex );
    return delayed_frames;
}

int x264_encoder_maximum_delayed_frames( x264_t *h )
{
    return h->frames.i_delay;
}
