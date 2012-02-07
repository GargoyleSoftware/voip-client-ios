/*
 * ALSA input and output
 * Copyright (c) 2007 Luca Abeni ( lucabe72 email it )
 * Copyright (c) 2007 Benoit Fouet ( benoit fouet free fr )
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * ALSA input and output: common code
 * @author Luca Abeni ( lucabe72 email it )
 * @author Benoit Fouet ( benoit fouet free fr )
 * @author Nicolas George ( nicolas george normalesup org )
 */

#include <alsa/asoundlib.h>
#include "libavformat/avformat.h"

#include "alsa-audio.h"

static av_cold snd_pcm_format_t codec_id_to_pcm_format(int codec_id)
{
    switch(codec_id) {
        case CODEC_ID_PCM_S16LE: return SND_PCM_FORMAT_S16_LE;
        case CODEC_ID_PCM_S16BE: return SND_PCM_FORMAT_S16_BE;
        case CODEC_ID_PCM_S8:    return SND_PCM_FORMAT_S8;
        default:                 return SND_PCM_FORMAT_UNKNOWN;
    }
}

static void alsa_reorder_s16_out_51(const void *in_v, void *out_v, int n)
{
    const int16_t *in = in_v;
    int16_t *out = out_v;

    while (n-- > 0) {
        out[0] = in[0];
        out[1] = in[1];
        out[2] = in[4];
        out[3] = in[5];
        out[4] = in[2];
        out[5] = in[3];
        in  += 6;
        out += 6;
    }
}

static void alsa_reorder_s16_out_71(const void *in_v, void *out_v, int n)
{
    const int16_t *in = in_v;
    int16_t *out = out_v;

    while (n-- > 0) {
        out[0] = in[0];
        out[1] = in[1];
        out[2] = in[4];
        out[3] = in[5];
        out[4] = in[2];
        out[5] = in[3];
        out[6] = in[6];
        out[7] = in[7];
        in  += 8;
        out += 8;
    }
}

#define REORDER_DUMMY ((void *)1)

static av_cold ff_reorder_func find_reorder_func(int codec_id,
                                                 int64_t layout,
                                                 int out)
{
    return
    codec_id == CODEC_ID_PCM_S16LE || codec_id == CODEC_ID_PCM_S16BE ?
        layout == AV_CH_LAYOUT_QUAD ? REORDER_DUMMY :
        layout == AV_CH_LAYOUT_5POINT1_BACK ?
            out ? alsa_reorder_s16_out_51 : NULL :
        layout == AV_CH_LAYOUT_7POINT1 ?
            out ? alsa_reorder_s16_out_71 : NULL :
            NULL :
        NULL;
}

av_cold int ff_alsa_open(AVFormatContext *ctx, snd_pcm_stream_t mode,
                         unsigned int *sample_rate,
                         int channels, enum CodecID *codec_id)
{
    AlsaData *s = ctx->priv_data;
    const char *audio_device;
    int res, flags = 0;
    snd_pcm_format_t format;
    snd_pcm_t *h;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_uframes_t buffer_size, period_size;
    int64_t layout = ctx->streams[0]->codec->channel_layout;

    if (ctx->filename[0] == 0) audio_device = "default";
    else                       audio_device = ctx->filename;

    if (*codec_id == CODEC_ID_NONE)
        *codec_id = DEFAULT_CODEC_ID;
    format = codec_id_to_pcm_format(*codec_id);
    if (format == SND_PCM_FORMAT_UNKNOWN) {
        av_log(ctx, AV_LOG_ERROR, "sample format 0x%04x is not supported\n", *codec_id);
        return AVERROR(ENOSYS);
    }
    s->frame_size = av_get_bits_per_sample(*codec_id) / 8 * channels;

    if (ctx->flags & AVFMT_FLAG_NONBLOCK) {
        flags = SND_PCM_NONBLOCK;
    }
    res = snd_pcm_open(&h, audio_device, mode, flags);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot open audio device %s (%s)\n",
               audio_device, snd_strerror(res));
        return AVERROR(EIO);
    }

    res = snd_pcm_hw_params_malloc(&hw_params);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot allocate hardware parameter structure (%s)\n",
               snd_strerror(res));
        goto fail1;
    }

    res = snd_pcm_hw_params_any(h, hw_params);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot initialize hardware parameter structure (%s)\n",
               snd_strerror(res));
        goto fail;
    }

    res = snd_pcm_hw_params_set_access(h, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set access type (%s)\n",
               snd_strerror(res));
        goto fail;
    }

    res = snd_pcm_hw_params_set_format(h, hw_params, format);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set sample format 0x%04x %d (%s)\n",
               *codec_id, format, snd_strerror(res));
        goto fail;
    }

    res = snd_pcm_hw_params_set_rate_near(h, hw_params, sample_rate, 0);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set sample rate (%s)\n",
               snd_strerror(res));
        goto fail;
    }

    res = snd_pcm_hw_params_set_channels(h, hw_params, channels);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set channel count to %d (%s)\n",
               channels, snd_strerror(res));
        goto fail;
    }

    snd_pcm_hw_params_get_buffer_size_max(hw_params, &buffer_size);
    /* TODO: maybe use ctx->max_picture_buffer somehow */
    res = snd_pcm_hw_params_set_buffer_size_near(h, hw_params, &buffer_size);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set ALSA buffer size (%s)\n",
               snd_strerror(res));
        goto fail;
    }

    snd_pcm_hw_params_get_period_size_min(hw_params, &period_size, NULL);
    res = snd_pcm_hw_params_set_period_size_near(h, hw_params, &period_size, NULL);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set ALSA period size (%s)\n",
               snd_strerror(res));
        goto fail;
    }
    s->period_size = period_size;

    res = snd_pcm_hw_params(h, hw_params);
    if (res < 0) {
        av_log(ctx, AV_LOG_ERROR, "cannot set parameters (%s)\n",
               snd_strerror(res));
        goto fail;
    }

    snd_pcm_hw_params_free(hw_params);

    if (channels > 2 && layout) {
        s->reorder_func = find_reorder_func(*codec_id, layout,
                                            mode == SND_PCM_STREAM_PLAYBACK);
        if (s->reorder_func == REORDER_DUMMY) {
            s->reorder_func = NULL;
        } else if (s->reorder_func) {
            s->reorder_buf_size = buffer_size;
            s->reorder_buf = av_malloc(s->reorder_buf_size * s->frame_size);
            if (!s->reorder_buf)
                goto fail1;
        } else {
            char name[16];
            av_get_channel_layout_string(name, sizeof(name), channels, layout);
            av_log(ctx, AV_LOG_WARNING,
                   "ALSA channel layout unknown or unimplemented for %s %s.\n",
                   name,
                   mode == SND_PCM_STREAM_PLAYBACK ? "playback" : "capture");
        }
    }

    s->h = h;
    return 0;

fail:
    snd_pcm_hw_params_free(hw_params);
fail1:
    snd_pcm_close(h);
    return AVERROR(EIO);
}

av_cold int ff_alsa_close(AVFormatContext *s1)
{
    AlsaData *s = s1->priv_data;

    av_freep(s->reorder_buf);
    snd_pcm_close(s->h);
    return 0;
}

int ff_alsa_xrun_recover(AVFormatContext *s1, int err)
{
    AlsaData *s = s1->priv_data;
    snd_pcm_t *handle = s->h;

    av_log(s1, AV_LOG_WARNING, "ALSA buffer xrun.\n");
    if (err == -EPIPE) {
        err = snd_pcm_prepare(handle);
        if (err < 0) {
            av_log(s1, AV_LOG_ERROR, "cannot recover from underrun (snd_pcm_prepare failed: %s)\n", snd_strerror(err));

            return AVERROR(EIO);
        }
    } else if (err == -ESTRPIPE) {
        av_log(s1, AV_LOG_ERROR, "-ESTRPIPE... Unsupported!\n");

        return -1;
    }
    return err;
}

int ff_alsa_extend_reorder_buf(AlsaData *s, int min_size)
{
    int size = s->reorder_buf_size;
    void *r;

    while (size < min_size)
        size *= 2;
    r = av_realloc(s->reorder_buf, size * s->frame_size);
    if (!r)
        return AVERROR(ENOMEM);
    s->reorder_buf = r;
    s->reorder_buf_size = size;
    return 0;
}
